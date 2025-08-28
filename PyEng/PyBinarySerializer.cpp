#include "PyBinarySerializer.h"
#include <cstring>
#include <cassert>
#include <sstream>

// Assign via generic mapping: iterate items() and setattr
static bool assign_via_mapping(PyObject* inst, PyObject* mapping) {
    if (!PyMapping_Check(mapping)) return false;
    PyObject* items = PyMapping_Items(mapping); // returns list of (k,v)
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
        // keys must be strings to setattr; otherwise write to __dict__ if possible
        if (PyUnicode_Check(k)) {
            if (PyObject_SetAttr(inst, k, v) < 0) { Py_DECREF(items); return false; }
        }
        else {
            // try __dict__ update for non-string keys
            PyObject* idict = PyObject_GetAttrString(inst, "__dict__");
            if (!idict || !PyDict_Check(idict) || PyDict_SetItem(idict, k, v) < 0) {
                Py_XDECREF(idict);
                Py_DECREF(items);
                return false;
            }
            Py_DECREF(idict);
        }
    }
    Py_DECREF(items);
    return true;
}



// ===== Writer =====
PyBinarySerializer::Writer::Writer(std::string& out, const PySerOptions& opt)
    : buf(out), opt(opt) {
    // Magic + version
    const char magic[4] = { 'P','Y','B',1 };
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
void PyBinarySerializer::Writer::writeVarI(int64_t v) {
    writeVarU(PyBinarySerializer::zigzag(v));
}
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
void PyBinarySerializer::Writer::writeString(const char* s, size_t n) {
    writeBytes(s, n);
}
void PyBinarySerializer::Writer::writeTag(Tag t) {
    writeByte(static_cast<uint8_t>(t));
}

// ===== Reader =====
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
int64_t PyBinarySerializer::Reader::readVarI() {
    uint64_t u = readVarU();
    return PyBinarySerializer::unzigzag(u);
}
double PyBinarySerializer::Reader::readDouble() {
    uint64_t u = 0;
    for (int i = 0; i < 8; ++i) u |= static_cast<uint64_t>(readByte()) << (8 * i);
    double d;
    std::memcpy(&d, &u, 8);
    return d;
}
void PyBinarySerializer::Reader::readBytes(void* dst, size_t n) {
    uint64_t want = readVarU();
    if (want != n) {
        if (n == 0) { if (want != 0) { PyErr_SetString(PyExc_ValueError, "Length mismatch"); throw std::runtime_error("len"); } return; }
        if (dst == nullptr) { // skip
            if (static_cast<size_t>(end - cur) < want) { PyErr_SetString(PyExc_EOFError, "Truncated bytes"); throw std::runtime_error("eof"); }
            cur += want;
            return;
        }
    }
    if (static_cast<size_t>(end - cur) < want) { PyErr_SetString(PyExc_EOFError, "Truncated bytes"); throw std::runtime_error("eof"); }
    std::memcpy(dst, cur, static_cast<size_t>(want));
    cur += want;
}
std::string PyBinarySerializer::Reader::readString() {
    uint64_t n = readVarU();
    if (static_cast<size_t>(end - cur) < n) { PyErr_SetString(PyExc_EOFError, "Truncated string"); throw std::runtime_error("eof"); }
    std::string s(cur, cur + n);
    cur += n;
    return s;
}
PyBinarySerializer::Tag PyBinarySerializer::Reader::readTag() {
    return static_cast<Tag>(readByte());
}

// ===== Utilities =====
inline uint64_t PyBinarySerializer::zigzag(int64_t v) {
    return (static_cast<uint64_t>(v) << 1) ^ static_cast<uint64_t>(v >> 63);
}
inline int64_t PyBinarySerializer::unzigzag(uint64_t u) {
    return static_cast<int64_t>((u >> 1) ^ (~(u & 1) + 1));
}
bool PyBinarySerializer::py_is_true(PyObject* o) {
    int r = PyObject_IsTrue(o);
    return r > 0;
}

// ===== LoadState =====
PyBinarySerializer::LoadState::~LoadState() {
    // If an exception occurred and we unwind before returning, decref all tracked objects.
    if (PyErr_Occurred()) {
        for (auto& kv : id_to_obj) Py_XDECREF(kv.second);
        id_to_obj.clear();
    }
}
void PyBinarySerializer::LoadState::track(uint64_t id, PyObject* o) {
    id_to_obj.emplace(id, o); // store NEW ref
}

// ===== Public API =====
bool PyBinarySerializer::Dump(PyObject* obj, std::string& out, const PySerOptions& opt) {
    MGil gil; // Ensure we own the MGil (safe even if already held)
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
        PyObject* v = load_value(L); // NEW ref
        // success: keep id_to_obj alive? Not needed; we transfer ownership to caller.
        return v;
    }
    catch (const std::exception&) {
        // Python exception already set
        return nullptr;
    }
}

// ===== Referenceability =====
bool PyBinarySerializer::is_referenceable(PyObject* o) {
    if (o == Py_None) return false;
    if (PyBool_Check(o) || PyLong_Check(o) || PyFloat_Check(o) ||
        PyUnicode_Check(o) || PyBytes_Check(o) || PyByteArray_Check(o)) {
        return false;
    }
    // Containers & all user objects
    return PyList_Check(o) || PyTuple_Check(o) || PyDict_Check(o) ||
        PySet_Check(o) || PyFrozenSet_Check(o) ||
        Py_TYPE(o) != &PyType_Type; // instances, not types
}

uint64_t PyBinarySerializer::ensure_id(PyObject* o, DumpState& S, bool& is_new) {
    auto it = S.memo.find(o);
    if (it != S.memo.end()) { is_new = false; return it->second; }
    uint64_t id = S.next_id++;
    S.memo.emplace(o, id);
    is_new = true;
    return id;
}

// ===== Dump primitives/containers =====
bool PyBinarySerializer::dump_value(PyObject* o, DumpState& S) {
    if (++S.depth > S.opt.max_depth) {
        PyErr_SetString(PyExc_RecursionError, "Max depth");
        return false;
    }

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
    else                                     ok = dump_object(o, S);

    --S.depth;
    return ok;
}

bool PyBinarySerializer::dump_none(DumpState& S) {
    S.w.writeTag(TAG_NONE);
    return true;
}
bool PyBinarySerializer::dump_bool(bool v, DumpState& S) {
    S.w.writeTag(v ? TAG_TRUE : TAG_FALSE);
    return true;
}
bool PyBinarySerializer::dump_int(PyObject* o, DumpState& S) {
    int overflow = 0;
    long long v = PyLong_AsLongLongAndOverflow(o, &overflow);
    if (!overflow) {
        S.w.writeTag(TAG_INT);
        S.w.writeVarI(static_cast<int64_t>(v));
        return true;
    }
    // Portable big-int path: decimal UTF-8 string
    S.w.writeTag(TAG_BIGINT);
    PyObject* s = PyObject_Str(o); // NEW ref
    if (!s) return false;
    Py_ssize_t n = 0;
    const char* p = PyUnicode_AsUTF8AndSize(s, &n);
    if (!p) { Py_DECREF(s); return false; }
    S.w.writeString(p, static_cast<size_t>(n));
    Py_DECREF(s);
    return true;
}

bool PyBinarySerializer::dump_float(PyObject* o, DumpState& S) {
    S.w.writeTag(TAG_FLOAT);
    S.w.writeDouble(PyFloat_AsDouble(o));
    return true;
}
bool PyBinarySerializer::dump_str(PyObject* o, DumpState& S) {
    Py_ssize_t n = 0;
    const char* p = PyUnicode_AsUTF8AndSize(o, &n);
    if (!p) return false;
    S.w.writeTag(TAG_STR);
    S.w.writeString(p, static_cast<size_t>(n));
    return true;
}
bool PyBinarySerializer::dump_bytes_like(PyObject* o, DumpState& S) {
    S.w.writeTag(TAG_BYTES);
    char* p = nullptr; Py_ssize_t n = 0;
    if (PyBytes_Check(o)) {
        if (PyBytes_AsStringAndSize(o, &p, &n) < 0) return false;
        S.w.writeBytes(p, static_cast<size_t>(n));
        return true;
    }
    else {
        n = PyByteArray_Size(o);
        p = PyByteArray_AsString(o);
        if (!p) return false;
        S.w.writeBytes(p, static_cast<size_t>(n));
        return true;
    }
}
bool PyBinarySerializer::dump_list(PyObject* o, DumpState& S) {
    bool is_new = false; uint64_t id = ensure_id(o, S, is_new);
    if (!is_new) { S.w.writeTag(TAG_REF); S.w.writeVarU(id); return true; }
    S.w.writeTag(TAG_LIST);
    S.w.writeVarU(id);
    Py_ssize_t n = PyList_GET_SIZE(o);
    S.w.writeVarU(static_cast<uint64_t>(n));
    for (Py_ssize_t i = 0; i < n; ++i) {
        PyObject* it = PyList_GET_ITEM(o, i); // borrowed
        if (!dump_value(it, S)) return false;
    }
    return true;
}
bool PyBinarySerializer::dump_tuple(PyObject* o, DumpState& S) {
    bool is_new = false; uint64_t id = ensure_id(o, S, is_new);
    if (!is_new) { S.w.writeTag(TAG_REF); S.w.writeVarU(id); return true; }
    S.w.writeTag(TAG_TUPLE);
    S.w.writeVarU(id);
    Py_ssize_t n = PyTuple_GET_SIZE(o);
    S.w.writeVarU(static_cast<uint64_t>(n));
    for (Py_ssize_t i = 0; i < n; ++i) {
        PyObject* it = PyTuple_GET_ITEM(o, i);
        if (!dump_value(it, S)) return false;
    }
    return true;
}
bool PyBinarySerializer::dump_dict(PyObject* o, DumpState& S) {
    bool is_new = false; uint64_t id = ensure_id(o, S, is_new);
    if (!is_new) { S.w.writeTag(TAG_REF); S.w.writeVarU(id); return true; }
    S.w.writeTag(TAG_DICT);
    S.w.writeVarU(id);
    Py_ssize_t pos = 0; PyObject* k; PyObject* v;
    uint64_t count = static_cast<uint64_t>(PyDict_GET_SIZE(o));
    S.w.writeVarU(count);
    while (PyDict_Next(o, &pos, &k, &v)) {
        if (!dump_value(k, S)) return false;
        if (!dump_value(v, S)) return false;
    }
    return true;
}
bool PyBinarySerializer::dump_set(PyObject* o, DumpState& S) {
    bool is_new = false; uint64_t id = ensure_id(o, S, is_new);
    if (!is_new) { S.w.writeTag(TAG_REF); S.w.writeVarU(id); return true; }
    S.w.writeTag(TAG_SET);
    S.w.writeVarU(id);
    uint64_t count = static_cast<uint64_t>(PySet_GET_SIZE(o));
    S.w.writeVarU(count);
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
    uint64_t count = static_cast<uint64_t>(PySet_GET_SIZE(o));
    S.w.writeVarU(count);
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

bool PyBinarySerializer::dump_object(PyObject* o, DumpState& S) {
    bool is_new = false; uint64_t id = ensure_id(o, S, is_new);
    if (!is_new) { S.w.writeTag(TAG_REF); S.w.writeVarU(id); return true; }

    PyObject* tp = (PyObject*)Py_TYPE(o);
#if 0
    std::string module = get_str_attr(tp, "__module__");
    std::string qualname = get_str_attr(tp, "__qualname__");
    if (module.empty() || qualname.empty()) {
        PyErr_SetString(PyExc_TypeError, "Object has no module/qualname");
        return false;
    }
#else
    std::string module = safe_type_attr_str(tp, "__module__", "builtins");
    std::string qualname = safe_type_attr_str(tp, "__qualname__", "");
    if (qualname.empty()) {
        // fall back to __name__ or raw tp_name
        qualname = safe_type_attr_str(tp, "__name__", "");
        if (qualname.empty()) {
            PyTypeObject* t = reinterpret_cast<PyTypeObject*>(tp);
            qualname = t && t->tp_name ? t->tp_name : "UnknownType";
        }
    }
#endif

    S.w.writeTag(TAG_OBJECT);
    S.w.writeVarU(id);
    S.w.writeString(module.data(), module.size());
    S.w.writeString(qualname.data(), qualname.size());

    // Strategy selection: getstate, reduce, dict
    PyObject* state = nullptr;
    if (S.opt.allow_getstate && try_getstate(o, &state)) {
        S.w.writeByte(static_cast<uint8_t>(OBJ_STATE_GETSTATE));
        bool ok = dump_value(state, S);
        Py_XDECREF(state);
        return ok;
    }

    PyObject* red = nullptr;
    if (S.opt.allow_reduce && try_reduce(o, &red)) {
        S.w.writeByte(static_cast<uint8_t>(OBJ_STATE_REDUCE));
        bool ok = dump_value(red, S); // store the raw reduce tuple; we'll replay at load
        Py_XDECREF(red);
        return ok;
    }

    PyObject* d = nullptr;
    if (S.opt.snapshot_dict && try_dict_snapshot(o, &d)) {
        S.w.writeByte(static_cast<uint8_t>(OBJ_STATE_DICT));
        bool ok = dump_value(d, S);
        Py_XDECREF(d);
        return ok;
    }

    // Last resort: empty state
    S.w.writeByte(static_cast<uint8_t>(OBJ_STATE_EMPTY));
    return true;
}

bool PyBinarySerializer::try_getstate(PyObject* o, PyObject** out_state) {
    if (!PyObject_HasAttrString(o, "__getstate__")) return false;
    PyObject* f = PyObject_GetAttrString(o, "__getstate__");
    if (!f) return false;
    PyObject* st = PyObject_CallObject(f, nullptr);
    Py_DECREF(f);
    if (!st) return false;
    *out_state = st; // NEW ref
    return true;
}

bool PyBinarySerializer::try_reduce(PyObject* o, PyObject** out_reduce_tuple) {
    PyObject* res = nullptr;
    // Prefer __reduce_ex__(4)
    if (PyObject_HasAttrString(o, "__reduce_ex__")) {
        PyObject* f = PyObject_GetAttrString(o, "__reduce_ex__");
        if (!f) return false;
        PyObject* arg = PyLong_FromLong(4);
        res = PyObject_CallFunctionObjArgs(f, arg, nullptr);
        Py_DECREF(arg);
        Py_DECREF(f);
    }
    if (!res && PyObject_HasAttrString(o, "__reduce__")) {
        PyObject* f = PyObject_GetAttrString(o, "__reduce__");
        if (!f) return false;
        res = PyObject_CallObject(f, nullptr);
        Py_DECREF(f);
    }
    if (!res) return false;
    if (!PyTuple_Check(res)) { Py_DECREF(res); return false; }
    *out_reduce_tuple = res; // NEW ref
    return true;
}

bool PyBinarySerializer::try_dict_snapshot(PyObject* o, PyObject** out_dict) {
    if (!PyObject_HasAttrString(o, "__dict__")) return false;
    PyObject* d = PyObject_GetAttrString(o, "__dict__");
    if (!d) return false;
    if (!PyDict_Check(d)) { Py_DECREF(d); return false; }
    *out_dict = d; // NEW ref
    return true;
}

// ===== Load primitives/containers =====
PyObject* PyBinarySerializer::load_value(LoadState& L) {
    if (++L.depth > L.opt.max_depth) {
        PyErr_SetString(PyExc_RecursionError, "Max depth");
        throw std::runtime_error("depth");
    }
    Tag t = L.r.readTag();

    PyObject* out = nullptr;
    switch (t) {
    case TAG_NONE: out = Py_None; Py_INCREF(out); break;
    case TAG_FALSE: out = PyBool_FromLong(0); break;
    case TAG_TRUE: out = PyBool_FromLong(1); break;
    case TAG_INT: {
        int64_t v = L.r.readVarI();
        out = PyLong_FromLongLong(v);
        break;
    }
    case TAG_BIGINT: {
        std::string dec = L.r.readString();
        PyObject* us = PyUnicode_DecodeUTF8(dec.data(), dec.size(), nullptr);
        if (!us) throw std::runtime_error("unicode");
        PyObject* v = PyLong_FromUnicodeObject(us, 10);
        Py_DECREF(us);
        if (!v) throw std::runtime_error("bigint");
        out = v;
        break;
    }

    default: break;
    }

    if (!out) switch (t) {
    case TAG_FLOAT: {
        double d = L.r.readDouble();
        out = PyFloat_FromDouble(d);
        break;
    }
    case TAG_STR: {
        std::string s = L.r.readString();
        out = PyUnicode_DecodeUTF8(s.data(), s.size(), nullptr);
        break;
    }
    case TAG_BYTES: {
        std::string s = L.r.readString();
        out = PyBytes_FromStringAndSize(s.data(), s.size());
        break;
    }
    case TAG_REF: {
        uint64_t id = L.r.readVarU();
        auto it = L.id_to_obj.find(id);
        if (it == L.id_to_obj.end()) {
            PyErr_SetString(PyExc_ValueError, "Invalid REF id");
            throw std::runtime_error("ref");
        }
        out = it->second; Py_INCREF(out);
        break;
    }
    case TAG_LIST: out = load_list(L); break;
    case TAG_TUPLE: out = load_tuple(L); break;
    case TAG_DICT: out = load_dict(L); break;
    case TAG_SET: out = load_set(L); break;
    case TAG_FROZENSET: out = load_frozenset(L); break;
    case TAG_OBJECT: out = load_object(L); break;
    default:
        PyErr_SetString(PyExc_ValueError, "Unknown tag");
        throw std::runtime_error("tag");
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
    L.track(id, lst); // store NEW ref
    for (uint64_t i = 0; i < n; ++i) {
        PyObject* it = load_value(L); // NEW
        PyList_SET_ITEM(lst, (Py_ssize_t)i, it); // steals ref
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
        PyTuple_SET_ITEM(tup, (Py_ssize_t)i, it); // steals ref
    }
    return tup;
}

PyObject* PyBinarySerializer::load_dict(LoadState& L) {
    uint64_t id = L.r.readVarU();
    uint64_t n = L.r.readVarU();
    PyObject* d = PyDict_New();
    if (!d) throw std::runtime_error("oom");
    L.track(id, d);
    for (uint64_t i = 0; i < n; ++i) {
        PyObject* k = load_value(L);
        PyObject* v = load_value(L);
        if (PyDict_SetItem(d, k, v) < 0) {
            Py_DECREF(k); Py_DECREF(v); throw std::runtime_error("dict set");
        }
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

// Resolve module + qualname ("A.B.C") possibly nested
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
    return cur; // NEW ref on class object
}

PyObject* PyBinarySerializer::new_without_init(PyObject* cls) {
    if (!PyType_Check(cls)) { PyErr_SetString(PyExc_TypeError, "Not a type"); return nullptr; }
    PyTypeObject* t = reinterpret_cast<PyTypeObject*>(cls);
    if (!t->tp_new) { PyErr_SetString(PyExc_TypeError, "Type has no tp_new"); return nullptr; }
    PyObject* empty_tuple = PyTuple_New(0);
    if (!empty_tuple) return nullptr;
    PyObject* inst = t->tp_new(t, empty_tuple, nullptr);
    Py_DECREF(empty_tuple);
    return inst; // NEW ref
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
    // Expect (callable, args [, state [, listitems [, dictitems]]])
    Py_ssize_t n = PyTuple_GET_SIZE(tpl);
    if (n < 2) { PyErr_SetString(PyExc_ValueError, "__reduce__ tuple too short"); return nullptr; }

    PyObject* callable = PyTuple_GET_ITEM(tpl, 0);
    PyObject* args = PyTuple_GET_ITEM(tpl, 1);
    if (!PyCallable_Check(callable) || !PyTuple_Check(args)) {
        PyErr_SetString(PyExc_TypeError, "Bad reduce tuple");
        return nullptr;
    }
    if (!opt.allow_reduce) {
        PyErr_SetString(PyExc_RuntimeError, "Reduce is disabled by policy");
        return nullptr;
    }

    PyObject* inst = PyObject_CallObject(callable, args); // NEW
    if (!inst) return nullptr;

    // Optional tail
    PyObject* state = (n >= 3) ? PyTuple_GET_ITEM(tpl, 2) : Py_None;
    PyObject* listitems = (n >= 4) ? PyTuple_GET_ITEM(tpl, 3) : Py_None;
    PyObject* dictitems = (n >= 5) ? PyTuple_GET_ITEM(tpl, 4) : Py_None;

    // Apply state
    if (state != Py_None) {
        if (!set_via_setstate(inst, state)) {
            // Fallback: if state is a dict, merge into __dict__
            if (PyDict_Check(state)) {
                if (!assign_via_dict(inst, state)) { Py_DECREF(inst); return nullptr; }
            }
        }
    }

    // Apply items
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
        // dictitems may be iterable of pairs
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

PyObject* PyBinarySerializer::load_object(LoadState& L) {
    uint64_t id = L.r.readVarU();
    std::string module = L.r.readString();
    std::string qual = L.r.readString();
    ObjStrategy strat = static_cast<ObjStrategy>(L.r.readByte());

    // Reconstruction path
    PyObject* cls = import_qualified(module, qual);
    if (!cls) return nullptr;

    PyObject* inst = nullptr;
    if (strat == OBJ_STATE_REDUCE) {
        // We stored the raw reduce tuple; first load it, then apply
        PyObject* tpl = load_value(L);
        if (!tpl) { Py_DECREF(cls); return nullptr; }
        inst = apply_reduce_tuple(tpl, L.opt);
        Py_DECREF(tpl);
        if (!inst) { Py_DECREF(cls); return nullptr; }
        L.track(id, inst);
        Py_DECREF(cls);
        return inst;
    }

    // Else: create blank instance without calling __init__
    inst = new_without_init(cls);
    if (!inst) { Py_DECREF(cls); return nullptr; }
    L.track(id, inst); // track immediately to allow cycles

    if (strat == OBJ_STATE_GETSTATE) {
        PyObject* state = load_value(L);
        if (!state) { Py_DECREF(cls); return nullptr; }

        bool ok = false;

        // Prefer __setstate__ when present
        if (L.opt.prefer_setstate && PyObject_HasAttrString(inst, "__setstate__"))
            ok = set_via_setstate(inst, state);

        // Fallbacks when __setstate__ missing or returned false:
        if (!ok) {
            if (PyDict_Check(state)) {
                ok = assign_via_dict(inst, state);
            }
            else if (PyMapping_Check(state)) {
                ok = assign_via_mapping(inst, state);
            }
            else {
                // last-resort: if state is a tuple of (dict, slotstate) (some libs do this),
                // try to apply the first element if it is a dict.
                if (PyTuple_Check(state) && PyTuple_GET_SIZE(state) >= 1) {
                    PyObject* first = PyTuple_GET_ITEM(state, 0);
                    if (PyDict_Check(first)) ok = assign_via_dict(inst, first);
                    else if (PyMapping_Check(first)) ok = assign_via_mapping(inst, first);
                }
            }
        }

        Py_DECREF(state);
        Py_DECREF(cls);
        if (!ok) { Py_DECREF(inst); return nullptr; }
        return inst;
    }
    else if (strat == OBJ_STATE_DICT) {
        PyObject* d = load_value(L);           // decoded state dict
        if (!d) { Py_DECREF(cls); return nullptr; }

        bool ok = false;
        // If class provides __setstate__, prefer it (lets class run post-logic)
        if (L.opt.prefer_setstate && PyObject_HasAttrString(inst, "__setstate__")) {
            ok = set_via_setstate(inst, d);
        }
        if (!ok) {
            ok = assign_via_dict(inst, d);     // fallback: shallow __dict__ update
        }

        Py_DECREF(d);
        Py_DECREF(cls);
        if (!ok) { Py_DECREF(inst); return nullptr; }
        return inst;
    }
    else { // EMPTY
        Py_DECREF(cls);
        return inst;
    }
}
