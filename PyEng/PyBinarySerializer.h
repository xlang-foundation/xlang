// cloudpickle-style variant: function-global overrides supported (added by ChatGPT)
#pragma once
#include "PyGILState.h"
#include <string>
#include <unordered_map>
#include <vector>
#include <cstdint>
#include <limits>
#include <stdexcept>

// ================== Options ==================
struct PySerOptions {
    // Limits to avoid pathological inputs
    size_t max_depth = 1'000;
    size_t max_nodes = 10'000'000;
    size_t max_bytes_out = (size_t)-1;

    bool   allow_reduce = true;     // allow __reduce__/__reduce_ex__
    bool   allow_getstate = true;   // allow __getstate__/__setstate__
    bool   snapshot_dict = true;    // allow fallback snapshot of __dict__

    // Project root used to decide whether a module/type/function is "local".
    // If empty, current working directory will be used.
    std::string base_dir;
};

struct PyDeserOptions {
    size_t max_depth = 1'000;
    size_t max_nodes = 10'000'000;

    bool   allow_reduce = true;      // allow executing reduce callables
    bool   prefer_setstate = true;   // prefer __setstate__ if present
};

// ================== API ==================
class PyBinarySerializer {
    std::string m_basePath;
public:
    inline void SetBasePath(std::string& path)
    {
        m_basePath = path;
    }
    // Serialize: returns true on success; on failure sets a Python exception and returns false.
    bool Dump(PyObject* obj, std::string& out, const PySerOptions& opt = {});

    // Deserialize: returns NEW REF PyObject* on success; NULL on failure with Python exception set.
    PyObject* Load(const char* data, size_t n, const PyDeserOptions& opt = {});

private:
    // ============= Wire Tags (format v1) =============
    enum Tag : uint8_t {
        TAG_NONE = 0x00,
        TAG_FALSE = 0x01,
        TAG_TRUE = 0x02,
        TAG_INT = 0x03,        // zig-zag varint 64
        TAG_BIGINT = 0x04,     // decimal utf-8
        TAG_FLOAT = 0x05,      // IEEE754 little-endian double
        TAG_STR = 0x06,        // UTF-8 bytes
        TAG_BYTES = 0x07,      // raw bytes
        TAG_LIST = 0x08,       // id + count + items
        TAG_TUPLE = 0x09,      // id + count + items
        TAG_DICT = 0x0A,       // id + count + (key,val)*
        TAG_SET = 0x0B,        // id + count + items
        TAG_FROZENSET = 0x0C,  // count + items (non-ref)
        TAG_REF = 0x0D,        // obj_id
        TAG_OBJECT = 0x0E,     // generic instance (id + module + qualname + strategy + payload)

        // New in this revision:
        TAG_FUNCTION = 0x0F,   // function (local code or import-by-name)
        TAG_MODULE = 0x10,   // module (embedded source or import-by-name)
        TAG_TYPE = 0x11    // class/type (module source or import-by-name)
    };

    enum ObjStrategy : uint8_t {
        OBJ_STATE_EMPTY = 0,
        OBJ_STATE_GETSTATE = 1,
        OBJ_STATE_REDUCE = 2,
        OBJ_STATE_DICT = 3
    };

    enum RebuildStrategy : uint8_t {
        RBY_IMPORT = 0,     // module + qualname then import
        RBY_SOURCE = 1,     // module source shipped, exec, then qualname
        RBY_CODE = 2      // for functions: code object + captured state (defaults/closure), with globals from source/import
    };

    // -------- Writer / Reader --------
    class Writer {
        PyBinarySerializer* m_parent;
    public:
        explicit Writer(PyBinarySerializer* parent,std::string& out, const PySerOptions& opt);
        void writeByte(uint8_t b);
        void writeVarU(uint64_t v);
        void writeVarI(int64_t v); // zig-zag
        void writeDouble(double d);
        void writeBytes(const void* p, size_t n);
        void writeString(const char* s, size_t n);
        void writeString(const std::string& s) { writeString(s.data(), s.size()); }
        void writeTag(Tag t);
        size_t size() const { return buf.size(); }
        std::string& buffer() { return buf; }
    private:
        std::string& buf;
        const PySerOptions& opt;
    };

    class Reader {
        PyBinarySerializer* m_parent;
    public:
        Reader(PyBinarySerializer* parent,const char* p, size_t n, const PyDeserOptions& opt);
        bool eof() const { return cur >= end; }
        uint8_t  readByte();
        uint64_t readVarU();
        int64_t  readVarI(); // zig-zag
        double   readDouble();
        void     readBytes(void* dst, size_t n); // reads length prefix and n bytes if dst!=nullptr; skips if dst==nullptr
        std::string readString();
        Tag      readTag();
    private:
        const char* cur;
        const char* end;
        const PyDeserOptions& opt;
    };

    // -------- State for one Dump/Load pass --------
    struct DumpState {
        Writer& w;
        const PySerOptions& opt;
        size_t depth = 0;
        uint64_t next_id = 1;
        std::unordered_map<PyObject*, uint64_t> memo; // identity-based
    };

    struct LoadState {
        Reader& r;
        const PyDeserOptions& opt;
        size_t depth = 0;
        std::unordered_map<uint64_t, PyObject*> id_to_obj; // NEW REFs
        ~LoadState();
        void track(uint64_t id, PyObject* o); // steals NEW ref into table
    };

    // -------- Dump helpers (primitives/containers/objects) --------
    bool dump_value(PyObject* o, DumpState& S);
    bool dump_none(DumpState& S);
    bool dump_bool(bool v, DumpState& S);
    bool dump_int(PyObject* o, DumpState& S);
    bool dump_float(PyObject* o, DumpState& S);
    bool dump_str(PyObject* o, DumpState& S);
    bool dump_bytes_like(PyObject* o, DumpState& S);
    bool dump_list(PyObject* o, DumpState& S);
    bool dump_tuple(PyObject* o, DumpState& S);
    bool dump_dict(PyObject* o, DumpState& S);
    bool dump_set(PyObject* o, DumpState& S);
    bool dump_frozenset(PyObject* o, DumpState& S);
    bool dump_object(PyObject* o, DumpState& S);

    // New callable/module/type dumping
    bool dump_function(PyObject* func, DumpState& S);
    bool dump_module(PyObject* mod, DumpState& S);
    bool dump_type(PyObject* type, DumpState& S);

    bool is_referenceable(PyObject* o);
    uint64_t ensure_id(PyObject* o, DumpState& S, bool& is_new);

    // Object strategy selection
    bool try_getstate(PyObject* o, PyObject** out_state);
    bool try_reduce(PyObject* o, PyObject** out_reduce_tuple);
    bool try_dict_snapshot(PyObject* o, PyObject** out_dict);

    // -------- Load helpers --------
    PyObject* load_value(LoadState& L);
    PyObject* load_list(LoadState& L);
    PyObject* load_tuple(LoadState& L);
    PyObject* load_dict(LoadState& L);
    PyObject* load_set(LoadState& L);
    PyObject* load_frozenset(LoadState& L);
    PyObject* load_object(LoadState& L);

    // New callable/module/type loading
    PyObject* load_function(LoadState& L);
    PyObject* load_function0(LoadState& L);
    PyObject* load_module(LoadState& L);
    PyObject* load_type(LoadState& L);

    // -------- Reconstruction utils --------
    PyObject* import_qualified(const std::string& module, const std::string& qualname);
    PyObject* new_without_init(PyObject* cls);
    bool set_via_setstate(PyObject* inst, PyObject* state);
    bool assign_via_dict(PyObject* inst, PyObject* d);
    PyObject* apply_reduce_tuple(PyObject* tpl, const PyDeserOptions& opt);

    // For modules & local/global decisions
    bool get_module_name_qual(PyObject* obj, std::string& module, std::string& qual);
    bool get_module_file(PyObject* mod, std::string& file_path);
    bool is_path_under_base(const std::string& path, const std::string& base_dir);
    std::string get_default_base_dir(); // cwd if available

    bool read_text_file(const std::string& path, std::string& out_text);

    // For function (de)serialization
    bool marshal_code_object(PyObject* code, std::string& out_bytes);
    PyObject* unmarshal_code_object(const std::string& bytes);
    bool is_local_entity(PyObject* owner_module, const PySerOptions& opt);

    PyObject* ensure_module_built_from_source(const std::string& module_name,
        const std::string& source_text);

    PyObject* build_function_from_bits(PyObject* code_obj,
        PyObject* globals_dict,
        const std::string& name,
        PyObject* defaults_tuple,      // may be NULL
        PyObject* kwdefaults_dict,     // may be NULL
        PyObject* closure_tuple);      // may be NULL

    PyObject* get_types_FunctionType();

    // Utilities
    inline uint64_t zigzag(int64_t v);
    inline int64_t unzigzag(uint64_t u);
    bool py_is_true(PyObject* o);
};