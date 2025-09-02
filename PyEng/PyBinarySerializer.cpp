#include "PyBinarySerializer.h"
#include <cstring>
#include <cassert>
#include <sstream>
#include <algorithm>
#include <marshal.h>

#if defined(_WIN32)
#include <io.h>
#include <direct.h>
#define getcwd _getcwd
#else
#include <unistd.h>
#endif

// ---------- forward helpers used only here ----------
static bool assign_via_mapping(PyObject* inst, PyObject* mapping);

// ================= Writer =================
PyBinarySerializer::Writer::Writer(std::string& out, const PySerOptions& opt)
    : buf(out), opt(opt) {
    const char magic[4] = { 'P','Y','B',1 }; // format v1
    buf.append(magic, 4);
}
void PyBinarySerializer::Writer::writeByte(uint8_t b) {
    if (buf.size() >= opt.max_bytes_out) throw std::runtime_error("size limit");
    buf.push_back(static_cast<char>(b));
}
void PyBinarySerializer::Writer::writeVarU(uint64_t v) {
    while (true) {
        uint8_t byte = v & 0x7F;
        v >>= 7;
        if (v) { writeByte(byte | 0x80); }
        else { writeByte(byte); break; }
    }
}
void PyBinarySerializer::Writer::writeVarI(int64_t v) { writeVarU(PyBinarySerializer::zigzag(v)); }
void PyBinarySerializer::Writer::writeDouble(double d) {
    uint64_t u;
    static_assert(sizeof(double) == 8, "double size");
    std::memcpy(&u, &d, 8);
    for (int i = 0; i < 8; ++i) writeByte((u >> (8 * i)) & 0xFF);
}
void PyBinarySerializer::Writer::writeBytes(const void* p, size_t n) {
    writeVarU(static_cast<uint64_t>(n));
    if (n == 0) return;
    if (buf.size() + n > opt.max_bytes_out) throw std::runtime_error("size limit");
    buf.append(static_cast<const char*>(p), n);
}
void PyBinarySerializer::Writer::writeString(const char* s, size_t n) { writeBytes(s, n); }
void PyBinarySerializer::Writer::writeTag(Tag t) { writeByte(static_cast<uint8_t>(t)); }

// ================= Reader =================
PyBinarySerializer::Reader::Reader(const char* p, size_t n, const PyDeserOptions& opt)
    : cur(p), end(p + n), opt(opt) {
    if (n < 4 || cur[0] != 'P' || cur[1] != 'Y' || cur[2] != 'B' || static_cast<uint8_t>(cur[3]) != 1) {
        PyErr_SetString(PyExc_ValueError, "Invalid header/magic");
        throw std::runtime_error("bad header");
    }
    cur += 4;
}
uint8_t PyBinarySerializer::Reader::readByte() {
    if (cur >= end) { PyErr_SetString(PyExc_EOFError, "Read past end"); throw std::runtime_error("eof"); }
    return static_cast<uint8_t>(*cur++);
}
uint64_t PyBinarySerializer::Reader::readVarU() {
    uint64_t v = 0, shift = 0;
    for (int i = 0; i < 10; ++i) {
        uint8_t b = readByte();
        v |= static_cast<uint64_t>(b & 0x7F) << shift;
        if (!(b & 0x80)) return v;
        shift += 7;
    }
    PyErr_SetString(PyExc_ValueError, "Varint too long");
    throw std::runtime_error("varint");
}
int64_t PyBinarySerializer::Reader::readVarI() { return PyBinarySerializer::unzigzag(readVarU()); }
double PyBinarySerializer::Reader::readDouble() {
    uint64_t u = 0;
    for (int i = 0; i < 8; ++i) u |= static_cast<uint64_t>(readByte()) << (8 * i);
    double d;
    std::memcpy(&d, &u, 8);
    return d;
}
void PyBinarySerializer::Reader::readBytes(void* dst, size_t n) {
    uint64_t want = readVarU();
    if (dst == nullptr) {
        // skip exactly 'want' bytes
        if ((size_t)(end - cur) < want) { PyErr_SetString(PyExc_EOFError, "Truncated bytes"); throw std::runtime_error("eof"); }
        cur += want;
        return;
    }
    if (want != n) { PyErr_SetString(PyExc_ValueError, "Length mismatch"); throw std::runtime_error("len"); }
    if ((size_t)(end - cur) < want) { PyErr_SetString(PyExc_EOFError, "Truncated bytes"); throw std::runtime_error("eof"); }
    std::memcpy(dst, cur, (size_t)want);
    cur += want;
}
std::string PyBinarySerializer::Reader::readString() {
    uint64_t n = readVarU();
    if ((size_t)(end - cur) < n) { PyErr_SetString(PyExc_EOFError, "Truncated string"); throw std::runtime_error("eof"); }
    std::string s(cur, cur + n);
    cur += n;
    return s;
}
PyBinarySerializer::Tag PyBinarySerializer::Reader::readTag() { return static_cast<Tag>(readByte()); }

// ================= Utilities =================
inline uint64_t PyBinarySerializer::zigzag(int64_t v) { return (static_cast<uint64_t>(v) << 1) ^ static_cast<uint64_t>(v >> 63); }
inline int64_t PyBinarySerializer::unzigzag(uint64_t u) { return static_cast<int64_t>((u >> 1) ^ (~(u & 1) + 1)); }
bool PyBinarySerializer::py_is_true(PyObject* o) { int r = PyObject_IsTrue(o); return r > 0; }

// ================= LoadState =================
PyBinarySerializer::LoadState::~LoadState() {
    if (PyErr_Occurred()) {
        for (auto& kv : id_to_obj) Py_XDECREF(kv.second);
        id_to_obj.clear();
    }
}
void PyBinarySerializer::LoadState::track(uint64_t id, PyObject* o) { id_to_obj.emplace(id, o); }

// ================= Public API =================
bool PyBinarySerializer::Dump(PyObject* obj, std::string& out, const PySerOptions& opt) {
    MGil gil;
    try {
        Writer w(out, opt);
        DumpState S{ w, opt };
        if (!dump_value(obj, S)) return false;
        return true;
    }
    catch (const std::exception& e) {
        if (!PyErr_Occurred()) PyErr_SetString(PyExc_RuntimeError, e.what());
        return false;
    }
}
PyObject* PyBinarySerializer::Load(const char* data, size_t n, const PyDeserOptions& opt) {
    MGil gil;
    try {
        Reader r(data, n, opt);
        LoadState L{ r, opt };
        PyObject* v = load_value(L);
        return v;
    }
    catch (const std::exception&) {
        return nullptr;
    }
}

// ================= Referenceability =================
bool PyBinarySerializer::is_referenceable(PyObject* o) {
    if (o == Py_None) return false;
    if (PyBool_Check(o) || PyLong_Check(o) || PyFloat_Check(o) ||
        PyUnicode_Check(o) || PyBytes_Check(o) || PyByteArray_Check(o)) {
        return false;
    }
    // containers, objects, functions, modules, types are all referenceable
    if (PyList_Check(o) || PyTuple_Check(o) || PyDict_Check(o) ||
        PySet_Check(o) || PyFrozenSet_Check(o) ||
        PyFunction_Check(o) || PyModule_Check(o) || PyType_Check(o)) {
        return true;
    }
    return Py_TYPE(o) != &PyType_Type;
}
uint64_t PyBinarySerializer::ensure_id(PyObject* o, DumpState& S, bool& is_new) {
    auto it = S.memo.find(o);
    if (it != S.memo.end()) { is_new = false; return it->second; }
    uint64_t id = S.next_id++;
    S.memo.emplace(o, id);
    is_new = true;
    return id;
}

// ================= Dump: primitives/containers =================
static std::string get_str_attr(PyObject* obj, const char* name) {
    PyObject* v = PyObject_GetAttrString(obj, name);
    if (!v) return {};
    Py_ssize_t n = 0; const char* s = PyUnicode_AsUTF8AndSize(v, &n);
    std::string out;
    if (s) out.assign(s, s + n);
    Py_DECREF(v);
    return out;
}
static std::string safe_type_attr_str(PyObject* tp, const char* name, const char* fallback = "") {
    PyObject* v = PyObject_GetAttrString(tp, name);
    if (!v) { PyErr_Clear(); return fallback; }
    Py_ssize_t n = 0; const char* s = PyUnicode_AsUTF8AndSize(v, &n);
    std::string out = s ? std::string(s, s + n) : std::string(fallback);
    Py_DECREF(v);
    return out;
}

bool PyBinarySerializer::dump_value(PyObject* o, DumpState& S) {
    if (++S.depth > S.opt.max_depth) { PyErr_SetString(PyExc_RecursionError, "Max depth"); return false; }

    bool ok = false;
    if (o == Py_None)                       ok = dump_none(S);
    else if (PyBool_Check(o))               ok = dump_bool(py_is_true(o), S);
    else if (PyLong_Check(o))               ok = dump_int(o, S);
    else if (PyFloat_Check(o))              ok = dump_float(o, S);
    else if (PyUnicode_Check(o))            ok = dump_str(o, S);
    else if (PyBytes_Check(o) || PyByteArray_Check(o)) ok = dump_bytes_like(o, S);
    else if (PyList_Check(o))               ok = dump_list(o, S);
    else if (PyTuple_Check(o))              ok = dump_tuple(o, S);
    else if (PyDict_Check(o))               ok = dump_dict(o, S);
    else if (PySet_Check(o))                ok = dump_set(o, S);
    else if (PyFrozenSet_Check(o))          ok = dump_frozenset(o, S);
    else if (PyFunction_Check(o))           ok = dump_function(o, S);
    else if (PyModule_Check(o))             ok = dump_module(o, S);
    else if (PyType_Check(o))               ok = dump_type(o, S);
    else                                    ok = dump_object(o, S);

    --S.depth;
    return ok;
}

bool PyBinarySerializer::dump_none(DumpState& S) { S.w.writeTag(TAG_NONE); return true; }
bool PyBinarySerializer::dump_bool(bool v, DumpState& S) { S.w.writeTag(v ? TAG_TRUE : TAG_FALSE); return true; }

bool PyBinarySerializer::dump_int(PyObject* o, DumpState& S) {
    int overflow = 0;
    long long v = PyLong_AsLongLongAndOverflow(o, &overflow);
    if (!overflow) {
        S.w.writeTag(TAG_INT);
        S.w.writeVarI(static_cast<int64_t>(v));
        return true;
    }
    S.w.writeTag(TAG_BIGINT);
    PyObject* s = PyObject_Str(o);
    if (!s) return false;
    Py_ssize_t n = 0;
    const char* p = PyUnicode_AsUTF8AndSize(s, &n);
    if (!p) { Py_DECREF(s); return false; }
    S.w.writeString(p, (size_t)n);
    Py_DECREF(s);
    return true;
}
bool PyBinarySerializer::dump_float(PyObject* o, DumpState& S) { S.w.writeTag(TAG_FLOAT); S.w.writeDouble(PyFloat_AsDouble(o)); return true; }
bool PyBinarySerializer::dump_str(PyObject* o, DumpState& S) {
    Py_ssize_t n = 0; const char* p = PyUnicode_AsUTF8AndSize(o, &n);
    if (!p) return false;
    S.w.writeTag(TAG_STR);
    S.w.writeString(p, (size_t)n);
    return true;
}
bool PyBinarySerializer::dump_bytes_like(PyObject* o, DumpState& S) {
    S.w.writeTag(TAG_BYTES);
    char* p = nullptr; Py_ssize_t n = 0;
    if (PyBytes_Check(o)) {
        if (PyBytes_AsStringAndSize(o, &p, &n) < 0) return false;
    }
    else {
        n = PyByteArray_Size(o);
        p = PyByteArray_AsString(o);
        if (!p) return false;
    }
    S.w.writeBytes(p, (size_t)n);
    return true;
}
bool PyBinarySerializer::dump_list(PyObject* o, DumpState& S) {
    bool is_new = false; uint64_t id = ensure_id(o, S, is_new);
    if (!is_new) { S.w.writeTag(TAG_REF); S.w.writeVarU(id); return true; }
    S.w.writeTag(TAG_LIST); S.w.writeVarU(id);
    Py_ssize_t n = PyList_GET_SIZE(o);
    S.w.writeVarU((uint64_t)n);
    for (Py_ssize_t i = 0; i < n; ++i) {
        PyObject* it = PyList_GET_ITEM(o, i);
        if (!dump_value(it, S)) return false;
    }
    return true;
}
bool PyBinarySerializer::dump_tuple(PyObject* o, DumpState& S) {
    bool is_new = false; uint64_t id = ensure_id(o, S, is_new);
    if (!is_new) { S.w.writeTag(TAG_REF); S.w.writeVarU(id); return true; }
    S.w.writeTag(TAG_TUPLE); S.w.writeVarU(id);
    Py_ssize_t n = PyTuple_GET_SIZE(o);
    S.w.writeVarU((uint64_t)n);
    for (Py_ssize_t i = 0; i < n; ++i) {
        PyObject* it = PyTuple_GET_ITEM(o, i);
        if (!dump_value(it, S)) return false;
    }
    return true;
}
bool PyBinarySerializer::dump_dict(PyObject* o, DumpState& S) {
    bool is_new = false; uint64_t id = ensure_id(o, S, is_new);
    if (!is_new) { S.w.writeTag(TAG_REF); S.w.writeVarU(id); return true; }
    S.w.writeTag(TAG_DICT); S.w.writeVarU(id);
    Py_ssize_t pos = 0; PyObject* k; PyObject* v;
    S.w.writeVarU((uint64_t)PyDict_GET_SIZE(o));
    while (PyDict_Next(o, &pos, &k, &v)) {
        if (!dump_value(k, S)) return false;
        if (!dump_value(v, S)) return false;
    }
    return true;
}
bool PyBinarySerializer::dump_set(PyObject* o, DumpState& S) {
    bool is_new = false; uint64_t id = ensure_id(o, S, is_new);
    if (!is_new) { S.w.writeTag(TAG_REF); S.w.writeVarU(id); return true; }
    S.w.writeTag(TAG_SET); S.w.writeVarU(id);
    S.w.writeVarU((uint64_t)PySet_GET_SIZE(o));
    PyObject* it = PyObject_GetIter(o);
    if (!it) return false;
    PyObject* item;
    while ((item = PyIter_Next(it))) {
        bool ok = dump_value(item, S);
        Py_DECREF(item);
        if (!ok) { Py_DECREF(it); return false; }
    }
    Py_DECREF(it);
    if (PyErr_Occurred()) return false;
    return true;
}
bool PyBinarySerializer::dump_frozenset(PyObject* o, DumpState& S) {
    S.w.writeTag(TAG_FROZENSET);
    S.w.writeVarU((uint64_t)PySet_GET_SIZE(o));
    PyObject* it = PyObject_GetIter(o);
    if (!it) return false;
    PyObject* item;
    while ((item = PyIter_Next(it))) {
        bool ok = dump_value(item, S);
        Py_DECREF(item);
        if (!ok) { Py_DECREF(it); return false; }
    }
    Py_DECREF(it);
    if (PyErr_Occurred()) return false;
    return true;
}

// ================= Dump: OBJECT (instances) =================
bool PyBinarySerializer::dump_object(PyObject* o, DumpState& S) {
    bool is_new = false; uint64_t id = ensure_id(o, S, is_new);
    if (!is_new) { S.w.writeTag(TAG_REF); S.w.writeVarU(id); return true; }

    PyObject* tp = (PyObject*)Py_TYPE(o);
    std::string module = safe_type_attr_str(tp, "__module__", "builtins");
    std::string qualname = safe_type_attr_str(tp, "__qualname__", "");
    if (qualname.empty()) {
        qualname = safe_type_attr_str(tp, "__name__", "");
        if (qualname.empty()) {
            PyTypeObject* t = reinterpret_cast<PyTypeObject*>(tp);
            qualname = t && t->tp_name ? t->tp_name : "UnknownType";
        }
    }

    // Decide class rebuild strategy without importing anything:
    // Try sys.modules[module].__file__ and check base_dir
    RebuildStrategy classStrat = RBY_IMPORT;
    std::string srcText;

    {
        std::string base = S.opt.base_dir.empty() ? get_default_base_dir() : S.opt.base_dir;

        PyObject* modules = PyImport_GetModuleDict(); // borrowed
        PyObject* mod = modules ? PyDict_GetItemString(modules, module.c_str()) : nullptr; // borrowed
        if (mod) {
            std::string file_path;
            if (get_module_file(mod, file_path) && is_path_under_base(file_path, base)) {
                if (read_text_file(file_path, srcText)) {
                    classStrat = RBY_SOURCE;
                }
            }
        }
        // If module not present or has no __file__, leave as RBY_IMPORT
    }

    // Emit header
    S.w.writeTag(TAG_OBJECT);
    S.w.writeVarU(id);
    S.w.writeString(module);
    S.w.writeString(qualname);

    // New (v2): class rebuild strategy + optional source
    S.w.writeByte((uint8_t)classStrat);
    if (classStrat == RBY_SOURCE) {
        S.w.writeString(srcText);
    }

    // ---- Existing instance state strategy selection ----
    PyObject* state = nullptr;
    if (S.opt.allow_getstate && try_getstate(o, &state)) {
        S.w.writeByte((uint8_t)OBJ_STATE_GETSTATE);
        bool ok = dump_value(state, S);
        Py_XDECREF(state);
        return ok;
    }
    PyObject* red = nullptr;
    if (S.opt.allow_reduce && try_reduce(o, &red)) {
        S.w.writeByte((uint8_t)OBJ_STATE_REDUCE);
        bool ok = dump_value(red, S); // store raw reduce tuple
        Py_XDECREF(red);
        return ok;
    }
    PyObject* d = nullptr;
    if (S.opt.snapshot_dict && try_dict_snapshot(o, &d)) {
        S.w.writeByte((uint8_t)OBJ_STATE_DICT);
        bool ok = dump_value(d, S);
        Py_XDECREF(d);
        return ok;
    }

    S.w.writeByte((uint8_t)OBJ_STATE_EMPTY);
    return true;
}


// ================= Dump: FUNCTION/MODULE/TYPE =================
bool PyBinarySerializer::get_module_name_qual(PyObject* obj, std::string& module, std::string& qual) {
    PyObject* m = PyObject_GetAttrString(obj, "__module__");
    if (!m) { PyErr_Clear(); return false; }
    PyObject* q = PyObject_GetAttrString(obj, "__qualname__");
    if (!q) { Py_DECREF(m); PyErr_Clear(); return false; }
    Py_ssize_t n1 = 0, n2 = 0;
    const char* s1 = PyUnicode_AsUTF8AndSize(m, &n1);
    const char* s2 = PyUnicode_AsUTF8AndSize(q, &n2);
    if (!s1 || !s2) { Py_DECREF(m); Py_DECREF(q); return false; }
    module.assign(s1, s1 + n1);
    qual.assign(s2, s2 + n2);
    Py_DECREF(m); Py_DECREF(q);
    return true;
}
bool PyBinarySerializer::get_module_file(PyObject* mod, std::string& file_path) {
    if (!PyModule_Check(mod)) return false;
    PyObject* f = PyObject_GetAttrString(mod, "__file__");
    if (!f) { PyErr_Clear(); return false; }
    Py_ssize_t n = 0; const char* s = PyUnicode_AsUTF8AndSize(f, &n);
    if (!s) { Py_DECREF(f); return false; }
    file_path.assign(s, s + n);
    Py_DECREF(f);
    return true;
}
static std::string to_lower(std::string s) { std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) {return (char)std::tolower(c); }); return s; }
static std::string norm_sep(std::string p) { std::replace(p.begin(), p.end(), '\\', '/'); return p; }

bool PyBinarySerializer::is_path_under_base(const std::string& path, const std::string& base_dir) {
    if (base_dir.empty()) return false;
    std::string p = to_lower(norm_sep(path));
    std::string b = to_lower(norm_sep(base_dir));
    if (!b.empty() && (b.back() == '/')) { /* ok */ }
    else { /* append slash */ }
    if (!b.empty() && b.back() != '/') b.push_back('/');
    return p.rfind(b, 0) == 0; // starts with
}
std::string PyBinarySerializer::get_default_base_dir() {
    char buf[4096];
    if (getcwd(buf, sizeof(buf))) return std::string(buf);
    return std::string();
}
bool PyBinarySerializer::read_text_file(const std::string& path, std::string& out_text) {
#ifdef _MSC_VER
    FILE* fp = nullptr;
    if (fopen_s(&fp, path.c_str(), "rb") != 0) {
        return false;
    }
#else
    FILE* fp = fopen(path.c_str(), "rb");
    if (!fp) return false;
#endif
    fseek(fp, 0, SEEK_END);
    long sz = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    if (sz < 0) { fclose(fp); return false; }
    out_text.resize((size_t)sz);
    size_t rd = fread(&out_text[0], 1, (size_t)sz, fp);
    fclose(fp);
    return rd == (size_t)sz;
}
bool PyBinarySerializer::marshal_code_object(PyObject* code, std::string& out_bytes) {
    // Use CPython marshal for code objects
    PyObject* b = PyMarshal_WriteObjectToString(code, Py_MARSHAL_VERSION);
    if (!b) return false;
    char* p = nullptr; Py_ssize_t n = 0;
    int rc = PyBytes_AsStringAndSize(b, &p, &n);
    if (rc < 0) { Py_DECREF(b); return false; }
    out_bytes.assign(p, p + n);
    Py_DECREF(b);
    return true;
}
PyObject* PyBinarySerializer::unmarshal_code_object(const std::string& bytes) {
    return PyMarshal_ReadObjectFromString(bytes.data(), (Py_ssize_t)bytes.size());
}
bool PyBinarySerializer::is_local_entity(PyObject* owner_module, const PySerOptions& opt) {
    std::string base = opt.base_dir.empty() ? get_default_base_dir() : opt.base_dir;
    if (base.empty()) return false;
    std::string file_path;
    if (!get_module_file(owner_module, file_path)) return false;
    return is_path_under_base(file_path, base);
}
PyObject* PyBinarySerializer::ensure_module_built_from_source(const std::string& module_name,
    const std::string& source_text) {
    // Create a new module object and exec code into it; insert into sys.modules
    PyObject* sys = PyImport_ImportModule("sys");
    if (!sys) return nullptr;
    PyObject* modules = PyObject_GetAttrString(sys, "modules");
    if (!modules) { Py_DECREF(sys); return nullptr; }

    PyObject* existing = PyDict_GetItemString(modules, module_name.c_str()); // borrowed
    if (existing) { Py_INCREF(existing); Py_DECREF(modules); Py_DECREF(sys); return existing; }

    PyObject* mod = PyModule_New(module_name.c_str());
    if (!mod) { Py_DECREF(modules); Py_DECREF(sys); return nullptr; }

    PyObject* dict = PyModule_GetDict(mod); // borrowed
    // compile then eval
    PyObject* code = Py_CompileStringExFlags(source_text.c_str(), module_name.c_str(), Py_file_input, nullptr, -1);
    if (!code) { Py_DECREF(mod); Py_DECREF(modules); Py_DECREF(sys); return nullptr; }
    PyObject* r = PyEval_EvalCode((PyObject*)code, dict, dict);
    Py_DECREF(code);
    if (!r) { Py_DECREF(mod); Py_DECREF(modules); Py_DECREF(sys); return nullptr; }
    Py_DECREF(r);

    // sys.modules[module_name] = mod
    if (PyDict_SetItemString(modules, module_name.c_str(), mod) < 0) { Py_DECREF(mod); Py_DECREF(modules); Py_DECREF(sys); return nullptr; }

    Py_DECREF(modules); Py_DECREF(sys);
    return mod; // NEW ref
}
PyObject* PyBinarySerializer::get_types_FunctionType() {
    PyObject* types = PyImport_ImportModule("types");
    if (!types) return nullptr;
    PyObject* ft = PyObject_GetAttrString(types, "FunctionType");
    Py_DECREF(types);
    return ft; // NEW ref
}
PyObject* PyBinarySerializer::build_function_from_bits(PyObject* code_obj,
    PyObject* globals_dict,
    const std::string& name,
    PyObject* defaults_tuple,
    PyObject* kwdefaults_dict,
    PyObject* closure_tuple) {
    PyObject* ft = get_types_FunctionType();
    if (!ft) return nullptr;

    // args: (code, globals[, name[, argdefs[, closure]]])
    PyObject* args = PyTuple_New(5);
    if (!args) { Py_DECREF(ft); return nullptr; }

    Py_INCREF(code_obj);
    Py_INCREF(globals_dict);
    PyTuple_SET_ITEM(args, 0, code_obj);
    PyTuple_SET_ITEM(args, 1, globals_dict);

    // name
    PyObject* pyName = PyUnicode_FromString(name.c_str());
    if (!pyName) { Py_DECREF(ft); Py_DECREF(args); return nullptr; }
    PyTuple_SET_ITEM(args, 2, pyName);

    if (defaults_tuple) { Py_INCREF(defaults_tuple); }
    else { defaults_tuple = Py_None; Py_INCREF(Py_None); }
    PyTuple_SET_ITEM(args, 3, defaults_tuple);

    if (closure_tuple) { Py_INCREF(closure_tuple); }
    else { closure_tuple = Py_None; Py_INCREF(Py_None); }
    PyTuple_SET_ITEM(args, 4, closure_tuple);

    PyObject* func = PyObject_CallObject(ft, args);
    Py_DECREF(ft);
    Py_DECREF(args);
    if (!func) return nullptr;

    // Set kwdefaults if provided
    if (kwdefaults_dict) {
        if (PyObject_SetAttrString(func, "__kwdefaults__", kwdefaults_dict) < 0) {
            Py_DECREF(func);
            return nullptr;
        }
    }
    return func; // NEW
}

// ----- Assign via mapping (used by instance state fallback) -----
static bool assign_via_mapping(PyObject* inst, PyObject* mapping) {
    if (!PyMapping_Check(mapping)) return false;
    PyObject* items = PyMapping_Items(mapping);
    if (!items) return false;
    Py_ssize_t n = PyList_GET_SIZE(items);
    for (Py_ssize_t i = 0; i < n; ++i) {
        PyObject* pair = PyList_GET_ITEM(items, i); // borrowed
        if (!PyTuple_Check(pair) || PyTuple_GET_SIZE(pair) != 2) {
            Py_DECREF(items);
            PyErr_SetString(PyExc_TypeError, "mapping items must be (k,v)");
            return false;
        }
        PyObject* k = PyTuple_GET_ITEM(pair, 0);
        PyObject* v = PyTuple_GET_ITEM(pair, 1);
        if (PyUnicode_Check(k)) {
            if (PyObject_SetAttr(inst, k, v) < 0) { Py_DECREF(items); return false; }
        }
        else {
            PyObject* idict = PyObject_GetAttrString(inst, "__dict__");
            if (!idict || !PyDict_Check(idict) || PyDict_SetItem(idict, k, v) < 0) {
                Py_XDECREF(idict); Py_DECREF(items); return false;
            }
            Py_DECREF(idict);
        }
    }
    Py_DECREF(items);
    return true;
}

// ================= Callable/Module/Type DUMP =================
bool PyBinarySerializer::dump_function(PyObject* func, DumpState& S) {
    bool is_new = false; uint64_t id = ensure_id(func, S, is_new);
    if (!is_new) { S.w.writeTag(TAG_REF); S.w.writeVarU(id); return true; }

    // Identify owner module + local/external
    std::string module, qual;
    if (!get_module_name_qual(func, module, qual)) {
        PyErr_SetString(PyExc_TypeError, "Function missing module/qualname");
        return false;
    }
    PyObject* owner_mod = PyImport_ImportModule(module.c_str());
    if (!owner_mod) return false;

    // Strategy
    RebuildStrategy strat = RBY_IMPORT; // default
    if (is_local_entity(owner_mod, S.opt)) strat = RBY_CODE;
    Py_DECREF(owner_mod);

    // Start record
    S.w.writeTag(TAG_FUNCTION);
    S.w.writeVarU(id);
    S.w.writeString(module);
    S.w.writeString(qual);
    S.w.writeByte((uint8_t)strat);

    if (strat == RBY_IMPORT) {
        // nothing else needed
        return true;
    }

    // RBX_CODE: capture code/defaults/kwdefaults/closure; also ship module source if the module is local
    // name
    std::string name = get_str_attr(func, "__name__");
    if (name.empty()) name = qual;

    // code
    PyObject* code = PyObject_GetAttrString(func, "__code__");
    if (!code) return false;
    std::string code_bytes;
    bool ok = marshal_code_object(code, code_bytes);
    Py_DECREF(code);
    if (!ok) return false;

    // defaults
    PyObject* defs = PyObject_GetAttrString(func, "__defaults__");     // may be NULL
    // kwdefaults
    PyObject* kwdefs = PyObject_GetAttrString(func, "__kwdefaults__"); // may be NULL
    // closure
    PyObject* closure = PyObject_GetAttrString(func, "__closure__");   // may be NULL

    // Optionally embed module source if under base_dir (gives proper globals without requiring installed pkg)
    std::string mod_src;
    PyObject* mod_for_src = PyImport_ImportModule(module.c_str());
    if (mod_for_src) {
        std::string file_path;
        if (get_module_file(mod_for_src, file_path) && is_path_under_base(file_path, S.opt.base_dir.empty() ? get_default_base_dir() : S.opt.base_dir)) {
            read_text_file(file_path, mod_src);
        }
        Py_DECREF(mod_for_src);
    }

    // emit payload:
    // name, code_bytes, has_source?, source_text, defaults, kwdefaults, closure(cell values)
    S.w.writeString(name);
    S.w.writeString(code_bytes);

    if (!mod_src.empty()) {
        S.w.writeByte(1);
        S.w.writeString(mod_src);
    }
    else {
        S.w.writeByte(0);
    }

    // defaults
    if (defs) { S.w.writeByte(1); ok = dump_value(defs, S); Py_DECREF(defs); if (!ok) return false; }
    else { S.w.writeByte(0); }

    // kwdefaults
    if (kwdefs) { S.w.writeByte(1); ok = dump_value(kwdefs, S); Py_DECREF(kwdefs); if (!ok) return false; }
    else { S.w.writeByte(0); }

    // closure (tuple of cells -> serialize cell contents)
    if (closure && closure != Py_None && PyTuple_Check(closure) && PyTuple_GET_SIZE(closure) > 0) {
        S.w.writeByte(1);
        Py_ssize_t n = PyTuple_GET_SIZE(closure);
        S.w.writeVarU((uint64_t)n);
        for (Py_ssize_t i = 0; i < n; ++i) {
            PyObject* cell = PyTuple_GET_ITEM(closure, i);
            PyObject* val = PyCell_Get(cell); // borrowed NEW?
            // PyCell_Get returns BORROWED reference (NULL on empty)
            if (val) { Py_INCREF(val); }
            else { val = Py_None; Py_INCREF(Py_None); }
            ok = dump_value(val, S);
            Py_DECREF(val);
            if (!ok) return false;
        }
        Py_DECREF(closure);
    }
    else {
        if (closure) Py_DECREF(closure);
        S.w.writeByte(0);
    }
    return true;
}

bool PyBinarySerializer::dump_module(PyObject* mod, DumpState& S) {
    bool is_new = false; uint64_t id = ensure_id(mod, S, is_new);
    if (!is_new) { S.w.writeTag(TAG_REF); S.w.writeVarU(id); return true; }

    std::string module_name = get_str_attr(mod, "__name__");
    std::string file_path;
    bool have_file = get_module_file(mod, file_path);

    RebuildStrategy strat = RBY_IMPORT;
    std::string src;
    if (have_file && is_path_under_base(file_path, S.opt.base_dir.empty() ? get_default_base_dir() : S.opt.base_dir)) {
        if (read_text_file(file_path, src)) strat = RBY_SOURCE;
    }

    S.w.writeTag(TAG_MODULE);
    S.w.writeVarU(id);
    S.w.writeString(module_name);
    S.w.writeByte((uint8_t)strat);
    if (strat == RBY_SOURCE) {
        S.w.writeString(src);
    }
    return true;
}

bool PyBinarySerializer::dump_type(PyObject* type, DumpState& S) {
    bool is_new = false; uint64_t id = ensure_id(type, S, is_new);
    if (!is_new) { S.w.writeTag(TAG_REF); S.w.writeVarU(id); return true; }

    std::string module, qual;
    if (!get_module_name_qual(type, module, qual)) {
        PyErr_SetString(PyExc_TypeError, "Type missing module/qualname");
        return false;
    }

    // decide source vs import
    PyObject* mod = PyImport_ImportModule(module.c_str());
    if (!mod) return false;
    std::string file_path;
    RebuildStrategy strat = RBY_IMPORT;
    std::string src;
    if (get_module_file(mod, file_path) &&
        is_path_under_base(file_path, S.opt.base_dir.empty() ? get_default_base_dir() : S.opt.base_dir) &&
        read_text_file(file_path, src)) {
        strat = RBY_SOURCE;
    }
    Py_DECREF(mod);

    S.w.writeTag(TAG_TYPE);
    S.w.writeVarU(id);
    S.w.writeString(module);
    S.w.writeString(qual);
    S.w.writeByte((uint8_t)strat);
    if (strat == RBY_SOURCE) {
        S.w.writeString(src);
    }
    return true;
}

// ================= Load: primitives/containers/objects =================
PyObject* PyBinarySerializer::load_value(LoadState& L) {
    if (++L.depth > L.opt.max_depth) { PyErr_SetString(PyExc_RecursionError, "Max depth"); throw std::runtime_error("depth"); }
    Tag t = L.r.readTag();
    PyObject* out = nullptr;

    switch (t) {
    case TAG_NONE: out = Py_None; Py_INCREF(out); break;
    case TAG_FALSE: out = PyBool_FromLong(0); break;
    case TAG_TRUE: out = PyBool_FromLong(1); break;
    case TAG_INT: { int64_t v = L.r.readVarI(); out = PyLong_FromLongLong(v); break; }
    case TAG_BIGINT: {
        std::string dec = L.r.readString();
        PyObject* us = PyUnicode_DecodeUTF8(dec.data(), dec.size(), nullptr);
        if (!us) throw std::runtime_error("unicode");
        PyObject* v = PyLong_FromUnicodeObject(us, 10);
        Py_DECREF(us);
        if (!v) throw std::runtime_error("bigint");
        out = v; break;
    }
    default: break;
    }

    if (!out) switch (t) {
    case TAG_FLOAT: { double d = L.r.readDouble(); out = PyFloat_FromDouble(d); break; }
    case TAG_STR: { std::string s = L.r.readString(); out = PyUnicode_DecodeUTF8(s.data(), s.size(), nullptr); break; }
    case TAG_BYTES: { std::string s = L.r.readString(); out = PyBytes_FromStringAndSize(s.data(), s.size()); break; }
    case TAG_REF: {
        uint64_t id = L.r.readVarU();
        auto it = L.id_to_obj.find(id);
        if (it == L.id_to_obj.end()) { PyErr_SetString(PyExc_ValueError, "Invalid REF id"); throw std::runtime_error("ref"); }
        out = it->second; Py_INCREF(out); break;
    }
    case TAG_LIST: out = load_list(L); break;
    case TAG_TUPLE: out = load_tuple(L); break;
    case TAG_DICT: out = load_dict(L); break;
    case TAG_SET: out = load_set(L); break;
    case TAG_FROZENSET: out = load_frozenset(L); break;
    case TAG_OBJECT: out = load_object(L); break;
    case TAG_FUNCTION: out = load_function(L); break;
    case TAG_MODULE: out = load_module(L); break;
    case TAG_TYPE: out = load_type(L); break;
    default: PyErr_SetString(PyExc_ValueError, "Unknown tag"); throw std::runtime_error("tag");
    }

    --L.depth;
    if (!out) throw std::runtime_error("alloc");
    return out;
}

PyObject* PyBinarySerializer::load_list(LoadState& L) {
    uint64_t id = L.r.readVarU();
    uint64_t n = L.r.readVarU();
    PyObject* lst = PyList_New((Py_ssize_t)n);
    if (!lst) throw std::runtime_error("oom");
    L.track(id, lst);
    for (uint64_t i = 0; i < n; ++i) {
        PyObject* it = load_value(L);
        PyList_SET_ITEM(lst, (Py_ssize_t)i, it);
    }
    return lst;
}
PyObject* PyBinarySerializer::load_tuple(LoadState& L) {
    uint64_t id = L.r.readVarU();
    uint64_t n = L.r.readVarU();
    PyObject* tup = PyTuple_New((Py_ssize_t)n);
    if (!tup) throw std::runtime_error("oom");
    L.track(id, tup);
    for (uint64_t i = 0; i < n; ++i) {
        PyObject* it = load_value(L);
        PyTuple_SET_ITEM(tup, (Py_ssize_t)i, it);
    }
    return tup;
}

static void print_pyobj_to_stderr(const std::string& indent, PyObject* obj)
{
    PyObject* repr = PyObject_Repr(obj);
    if (!repr) return;                     // repr failed – nothing to print

    const char* s = PyUnicode_AsUTF8(repr);
    if (s) {
        fprintf(stderr, "%s%s\n", indent.c_str(), s);
    }
    Py_DECREF(repr);
}


PyObject* PyBinarySerializer::load_dict(LoadState& L) {
    uint64_t id = L.r.readVarU();
    uint64_t n = L.r.readVarU();
    PyObject* d = PyDict_New();
    if (!d) throw std::runtime_error("oom");
    L.track(id, d);

    /* --- indentation based on current depth --------------------------------- */
    std::string indent(L.depth, '\t');          // one tab per depth level
    /* ------------------------------------------------------------------------ */

    for (uint64_t i = 0; i < n; ++i) {
        PyObject* k = load_value(L);
        /* ---------- DEBUG: print the key ---------- */
        if (k) {
            if (PyUnicode_Check(k)) {
                const char* s = PyUnicode_AsUTF8(k);
                if (s) {
                    fprintf(stderr, "%s  key[%zu]: %s\n", indent.c_str(), i, s);
                }
            }
            else {
                // Non‑string key – fallback to repr
                print_pyobj_to_stderr(indent + "  key[", k);
            }
        }
        /* ------------------------------------------- */
        PyObject* v = load_value(L);
        if (PyDict_SetItem(d, k, v) < 0) { Py_DECREF(k); Py_DECREF(v); throw std::runtime_error("dict set"); }
        Py_DECREF(k); Py_DECREF(v);
    }
    return d;
}
PyObject* PyBinarySerializer::load_set(LoadState& L) {
    uint64_t id = L.r.readVarU();
    uint64_t n = L.r.readVarU();
    PyObject* s = PySet_New(nullptr);
    if (!s) throw std::runtime_error("oom");
    L.track(id, s);
    for (uint64_t i = 0; i < n; ++i) {
        PyObject* it = load_value(L);
        if (PySet_Add(s, it) < 0) { Py_DECREF(it); throw std::runtime_error("set add"); }
        Py_DECREF(it);
    }
    return s;
}
PyObject* PyBinarySerializer::load_frozenset(LoadState& L) {
    uint64_t n = L.r.readVarU();
    PyObject* s = PySet_New(nullptr);
    if (!s) throw std::runtime_error("oom");
    for (uint64_t i = 0; i < n; ++i) {
        PyObject* it = load_value(L);
        if (PySet_Add(s, it) < 0) { Py_DECREF(it); Py_DECREF(s); throw std::runtime_error("fs add"); }
        Py_DECREF(it);
    }
    PyObject* fs = PyFrozenSet_New(s);
    Py_DECREF(s);
    if (!fs) throw std::runtime_error("oom");
    return fs;
}

// ----- Existing instance/object loader -----
PyObject* PyBinarySerializer::load_object(LoadState& L) {
    uint64_t id = L.r.readVarU();
    std::string module = L.r.readString();
    std::string qual = L.r.readString();

    // New (v2): how to obtain the class
    RebuildStrategy classStrat = static_cast<RebuildStrategy>(L.r.readByte());
    std::string src;
    if (classStrat == RBY_SOURCE) {
        src = L.r.readString();
    }

    // Existing: object state strategy follows
    ObjStrategy strat = static_cast<ObjStrategy>(L.r.readByte());

    // Materialize the class according to classStrat
    PyObject* cls = nullptr;
    if (classStrat == RBY_SOURCE) {
        PyObject* mod = ensure_module_built_from_source(module, src);
        if (!mod) return nullptr;
        Py_DECREF(mod);
        cls = import_qualified(module, qual); // now it resolves within the just-built module
    }
    else {
        // RBY_IMPORT: normal path (this may import if not already loaded)
        cls = import_qualified(module, qual);
    }
    if (!cls) return nullptr;

    // If __reduce__ payload is used, construct via reduce callable (which may return a ready instance)
    if (strat == OBJ_STATE_REDUCE) {
        PyObject* tpl = load_value(L);
        if (!tpl) { Py_DECREF(cls); return nullptr; }
        PyObject* inst = apply_reduce_tuple(tpl, L.opt);
        Py_DECREF(tpl);
        if (!inst) { Py_DECREF(cls); return nullptr; }
        L.track(id, inst);
        Py_DECREF(cls);
        return inst;
    }

    // Otherwise, allocate a blank instance, then apply state
    PyObject* inst = new_without_init(cls);
    if (!inst) { Py_DECREF(cls); return nullptr; }
    L.track(id, inst);

    if (strat == OBJ_STATE_GETSTATE) {
        PyObject* state = load_value(L);
        if (!state) { Py_DECREF(cls); return nullptr; }
        bool ok = false;
        if (L.opt.prefer_setstate && PyObject_HasAttrString(inst, "__setstate__"))
            ok = set_via_setstate(inst, state);
        if (!ok) {
            if (PyDict_Check(state)) ok = assign_via_dict(inst, state);
            else if (PyMapping_Check(state)) ok = assign_via_mapping(inst, state);
            else if (PyTuple_Check(state) && PyTuple_GET_SIZE(state) >= 1) {
                PyObject* first = PyTuple_GET_ITEM(state, 0);
                if (PyDict_Check(first)) ok = assign_via_dict(inst, first);
                else if (PyMapping_Check(first)) ok = assign_via_mapping(inst, first);
            }
        }
        Py_DECREF(state);
        Py_DECREF(cls);
        if (!ok) { Py_DECREF(inst); return nullptr; }
        return inst;
    }
    else if (strat == OBJ_STATE_DICT) {
        PyObject* d = load_value(L);
        if (!d) { Py_DECREF(cls); return nullptr; }
        bool ok = false;
        if (L.opt.prefer_setstate && PyObject_HasAttrString(inst, "__setstate__"))
            ok = set_via_setstate(inst, d);
        if (!ok) ok = assign_via_dict(inst, d);
        Py_DECREF(d);
        Py_DECREF(cls);
        if (!ok) { Py_DECREF(inst); return nullptr; }
        return inst;
    }
    else {
        Py_DECREF(cls);
        return inst;
    }
}

// ================= Load: FUNCTION/MODULE/TYPE =================
PyObject* PyBinarySerializer::load_module(LoadState& L) {
    uint64_t id = L.r.readVarU();
    std::string module_name = L.r.readString();
    RebuildStrategy strat = (RebuildStrategy)L.r.readByte();

    PyObject* mod = nullptr;
    if (strat == RBY_IMPORT) {
        mod = PyImport_ImportModule(module_name.c_str());
        if (!mod) return nullptr;
    }
    else {
        std::string src = L.r.readString();
        mod = ensure_module_built_from_source(module_name, src);
        if (!mod) return nullptr;
    }
    L.track(id, mod);
    return mod;
}

PyObject* PyBinarySerializer::load_type(LoadState& L) {
    uint64_t id = L.r.readVarU();
    std::string module = L.r.readString();
    std::string qual = L.r.readString();
    RebuildStrategy strat = (RebuildStrategy)L.r.readByte();

    PyObject* cls = nullptr;
    if (strat == RBY_IMPORT) {
        cls = import_qualified(module, qual);
        if (!cls) return nullptr;
    }
    else {
        std::string src = L.r.readString();
        PyObject* mod = ensure_module_built_from_source(module, src);
        if (!mod) return nullptr;
        Py_DECREF(mod);
        cls = import_qualified(module, qual);
        if (!cls) return nullptr;
    }
    L.track(id, cls);
    return cls;
}

PyObject* PyBinarySerializer::load_function(LoadState& L) {
    uint64_t id = L.r.readVarU();
    std::string module = L.r.readString();
    std::string qual = L.r.readString();
    RebuildStrategy strat = (RebuildStrategy)L.r.readByte();

    if (strat == RBY_IMPORT) {
        PyObject* f = import_qualified(module, qual);
        if (!f) return nullptr;
        L.track(id, f);
        return f;
    }

    // RBY_CODE: read payload
    std::string name = L.r.readString();
    std::string code_bytes = L.r.readString();
    uint8_t has_source = L.r.readByte();

    std::string src;
    if (has_source) { src = L.r.readString(); }

    // defaults
    PyObject* defaults_tuple = nullptr;
    if (L.r.readByte()) {
        defaults_tuple = load_value(L); // may be any sequence; we'll rely on FunctionType semantics (expects tuple or None)
        if (!defaults_tuple) return nullptr;
    }
    // kwdefaults
    PyObject* kwdefaults_dict = nullptr;
    if (L.r.readByte()) {
        kwdefaults_dict = load_value(L); // expect dict or None
        if (!kwdefaults_dict) { Py_XDECREF(defaults_tuple); return nullptr; }
    }
    // closure
    PyObject* closure_tuple = nullptr;
    if (L.r.readByte()) {
        uint64_t n = L.r.readVarU();
        closure_tuple = PyTuple_New((Py_ssize_t)n);
        if (!closure_tuple) { Py_XDECREF(defaults_tuple); Py_XDECREF(kwdefaults_dict); return nullptr; }
        for (uint64_t i = 0; i < n; ++i) {
            PyObject* val = load_value(L);
            if (!val) { Py_DECREF(closure_tuple); Py_XDECREF(defaults_tuple); Py_XDECREF(kwdefaults_dict); return nullptr; }
            PyObject* cell = PyCell_New(val);
            Py_DECREF(val);
            if (!cell) { Py_DECREF(closure_tuple); Py_XDECREF(defaults_tuple); Py_XDECREF(kwdefaults_dict); return nullptr; }
            PyTuple_SET_ITEM(closure_tuple, (Py_ssize_t)i, cell); // steals
        }
    }

    // Rebuild module/globals
    PyObject* globals_mod = nullptr;
    if (!src.empty()) {
        globals_mod = ensure_module_built_from_source(module, src);
        if (!globals_mod) { Py_XDECREF(defaults_tuple); Py_XDECREF(kwdefaults_dict); Py_XDECREF(closure_tuple); return nullptr; }
    }
    else {
        globals_mod = PyImport_ImportModule(module.c_str());
        if (!globals_mod) { Py_XDECREF(defaults_tuple); Py_XDECREF(kwdefaults_dict); Py_XDECREF(closure_tuple); return nullptr; }
    }
    PyObject* globals_dict = PyModule_GetDict(globals_mod); // borrowed

    // Unmarshal code
    PyObject* code_obj = unmarshal_code_object(code_bytes);
    if (!code_obj) { Py_DECREF(globals_mod); Py_XDECREF(defaults_tuple); Py_XDECREF(kwdefaults_dict); Py_XDECREF(closure_tuple); return nullptr; }

    PyObject* func = build_function_from_bits(code_obj, globals_dict, name, defaults_tuple, kwdefaults_dict, closure_tuple);
    Py_DECREF(code_obj);
    Py_DECREF(globals_mod);
    Py_XDECREF(defaults_tuple);
    Py_XDECREF(kwdefaults_dict);
    Py_XDECREF(closure_tuple);
    if (!func) return nullptr;

    L.track(id, func);
    return func;
}

// ================= Import/Construct/State Helpers =================
PyObject* PyBinarySerializer::import_qualified(const std::string& module, const std::string& qualname) {
    PyObject* mod = PyImport_ImportModule(module.c_str());
    if (!mod) return nullptr;
    PyObject* cur = mod;
    Py_INCREF(cur);
    Py_DECREF(mod);
    size_t start = 0;
    while (start < qualname.size()) {
        size_t dot = qualname.find('.', start);
        std::string name = (dot == std::string::npos) ? qualname.substr(start)
            : qualname.substr(start, dot - start);
        PyObject* next = PyObject_GetAttrString(cur, name.c_str());
        Py_DECREF(cur);
        if (!next) return nullptr;
        cur = next;
        if (dot == std::string::npos) break;
        start = dot + 1;
    }
    return cur;
}
PyObject* PyBinarySerializer::new_without_init(PyObject* cls) {
    if (!PyType_Check(cls)) { PyErr_SetString(PyExc_TypeError, "Not a type"); return nullptr; }
    PyTypeObject* t = reinterpret_cast<PyTypeObject*>(cls);
    if (!t->tp_new) { PyErr_SetString(PyExc_TypeError, "Type has no tp_new"); return nullptr; }
    PyObject* empty_tuple = PyTuple_New(0);
    if (!empty_tuple) return nullptr;
    PyObject* inst = t->tp_new(t, empty_tuple, nullptr);
    Py_DECREF(empty_tuple);
    return inst;
}
bool PyBinarySerializer::set_via_setstate(PyObject* inst, PyObject* state) {
    if (!PyObject_HasAttrString(inst, "__setstate__")) return false;
    PyObject* f = PyObject_GetAttrString(inst, "__setstate__");
    if (!f) return false;
    PyObject* r = PyObject_CallFunctionObjArgs(f, state, nullptr);
    Py_DECREF(f);
    if (!r) return false;
    Py_DECREF(r);
    return true;
}
bool PyBinarySerializer::assign_via_dict(PyObject* inst, PyObject* d) {
    if (!PyDict_Check(d)) return false;
    PyObject* idict = PyObject_GetAttrString(inst, "__dict__");
    if (!idict) return false;
    int rc = PyDict_Update(idict, d);
    Py_DECREF(idict);
    return rc == 0;
}
PyObject* PyBinarySerializer::apply_reduce_tuple(PyObject* tpl, const PyDeserOptions& opt) {
    // (callable, args [, state [, listitems [, dictitems]]])
    Py_ssize_t n = PyTuple_GET_SIZE(tpl);
    if (n < 2) { PyErr_SetString(PyExc_ValueError, "__reduce__ tuple too short"); return nullptr; }
    PyObject* callable = PyTuple_GET_ITEM(tpl, 0);
    PyObject* args = PyTuple_GET_ITEM(tpl, 1);
    if (!PyCallable_Check(callable) || !PyTuple_Check(args)) { PyErr_SetString(PyExc_TypeError, "Bad reduce tuple"); return nullptr; }
    if (!opt.allow_reduce) { PyErr_SetString(PyExc_RuntimeError, "Reduce is disabled by policy"); return nullptr; }

    PyObject* inst = PyObject_CallObject(callable, args);
    if (!inst) return nullptr;

    PyObject* state = (n >= 3) ? PyTuple_GET_ITEM(tpl, 2) : Py_None;
    PyObject* listitems = (n >= 4) ? PyTuple_GET_ITEM(tpl, 3) : Py_None;
    PyObject* dictitems = (n >= 5) ? PyTuple_GET_ITEM(tpl, 4) : Py_None;

    if (state != Py_None) {
        if (!set_via_setstate(inst, state)) {
            if (PyDict_Check(state)) {
                if (!assign_via_dict(inst, state)) { Py_DECREF(inst); return nullptr; }
            }
        }
    }
    if (listitems != Py_None) {
        PyObject* it = PyObject_GetIter(listitems);
        if (it) {
            PyObject* x;
            while ((x = PyIter_Next(it))) {
                if (PyList_Check(inst)) {
                    if (PyList_Append(inst, x) < 0) { Py_DECREF(x); Py_DECREF(it); Py_DECREF(inst); return nullptr; }
                }
                Py_DECREF(x);
            }
            Py_DECREF(it);
            if (PyErr_Occurred()) { Py_DECREF(inst); return nullptr; }
        }
    }
    if (dictitems != Py_None && PyDict_Check(inst)) {
        PyObject* it = PyObject_GetIter(dictitems);
        if (it) {
            PyObject* pair;
            while ((pair = PyIter_Next(it))) {
                if (PyTuple_Check(pair) && PyTuple_GET_SIZE(pair) == 2) {
                    PyObject* k = PyTuple_GET_ITEM(pair, 0);
                    PyObject* v = PyTuple_GET_ITEM(pair, 1);
                    if (PyDict_SetItem(inst, k, v) < 0) { Py_DECREF(pair); Py_DECREF(it); Py_DECREF(inst); return nullptr; }
                }
                Py_DECREF(pair);
            }
            Py_DECREF(it);
            if (PyErr_Occurred()) { Py_DECREF(inst); return nullptr; }
        }
    }
    return inst;
}
// === Missing helpers: try_getstate / try_reduce / try_dict_snapshot ===
bool PyBinarySerializer::try_getstate(PyObject* o, PyObject** out_state) {
    if (!PyObject_HasAttrString(o, "__getstate__")) {
        PyErr_Clear();
        return false;
    }
    PyObject* f = PyObject_GetAttrString(o, "__getstate__");
    if (!f) { PyErr_Clear(); return false; }

    PyObject* state = PyObject_CallFunctionObjArgs(f, nullptr);
    Py_DECREF(f);
    if (!state) { PyErr_Clear(); return false; }

    *out_state = state;  // NEW ref
    return true;
}

bool PyBinarySerializer::try_reduce(PyObject* o, PyObject** out_reduce_tuple) {
    PyObject* reduce_tuple = nullptr;

    // Prefer __reduce_ex__(highest_protocol), then fall back to __reduce__.
    int protocol = 5;  // Python 3.8+ supports 5; earlier Pythons will ignore it anyway.
#if PY_MAJOR_VERSION == 3 && PY_MINOR_VERSION < 8
    protocol = 4;
#endif

    PyObject* rex = PyObject_GetAttrString(o, "__reduce_ex__");
    if (rex) {
        reduce_tuple = PyObject_CallFunction(rex, "i", protocol);
        Py_DECREF(rex);
    }
    else {
        PyErr_Clear();
        PyObject* r = PyObject_GetAttrString(o, "__reduce__");
        if (r) {
            reduce_tuple = PyObject_CallFunctionObjArgs(r, nullptr);
            Py_DECREF(r);
        }
        else {
            PyErr_Clear();
        }
    }

    if (!reduce_tuple) return false;
    if (!PyTuple_Check(reduce_tuple)) { Py_DECREF(reduce_tuple); return false; }

    *out_reduce_tuple = reduce_tuple;  // NEW ref
    return true;
}

bool PyBinarySerializer::try_dict_snapshot(PyObject* o, PyObject** out_dict) {
    PyObject* d = PyObject_GetAttrString(o, "__dict__");
    if (!d) { PyErr_Clear(); return false; }
    if (!PyDict_Check(d)) { Py_DECREF(d); return false; }

    PyObject* copy = PyDict_Copy(d);
    Py_DECREF(d);
    if (!copy) return false;

    *out_dict = copy;  // NEW ref
    return true;
}
