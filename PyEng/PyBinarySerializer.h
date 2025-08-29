#pragma once
#include "PyGILState.h"
#include <string>
#include <unordered_map>
#include <vector>
#include <cstdint>
#include <limits>
#include <stdexcept>

struct PySerOptions {
    // Limits to avoid pathological inputs
    size_t max_depth = 1'000;
    size_t max_nodes = 10'000'000;
    size_t max_bytes_out = (size_t)-1;
    bool   allow_reduce = true;   // allow __reduce__/__reduce_ex__
    bool   allow_getstate = true;   // allow __getstate__/__setstate__
    bool   snapshot_dict = true;   // allow fallback snapshot of __dict__
};

struct PyDeserOptions {
    size_t max_depth = 1'000;
    size_t max_nodes = 10'000'000;
    bool   allow_reduce = true;   // allow executing reduce callables
    bool   prefer_setstate = true; // prefer __setstate__ if present
};

class PyBinarySerializer {
public:
    // Serialize: returns true on success; on failure sets a Python exception and returns false.
    static bool Dump(PyObject* obj, std::string& out, const PySerOptions& opt = {});

    // Deserialize: returns NEW REF PyObject* on success; NULL on failure with Python exception set.
    static PyObject* Load(const char* data, size_t n, const PyDeserOptions& opt = {});

private:
    enum Tag : uint8_t {
        TAG_NONE = 0x00,
        TAG_FALSE = 0x01,
        TAG_TRUE = 0x02,
        TAG_INT = 0x03,   // zig-zag varint 64
        TAG_BIGINT = 0x04,   // 2's complement byte array
        TAG_FLOAT = 0x05,   // IEEE754 little-endian double
        TAG_STR = 0x06,   // UTF-8 bytes
        TAG_BYTES = 0x07,   // raw bytes
        TAG_LIST = 0x08,   // id + count + items
        TAG_TUPLE = 0x09,   // id + count + items
        TAG_DICT = 0x0A,   // id + count + (key,val)*
        TAG_SET = 0x0B,   // id + count + items
        TAG_FROZENSET = 0x0C,   // count + items (non-ref)
        TAG_REF = 0x0D,   // obj_id
        TAG_OBJECT = 0x0E    // id + module + qualname + strategy + payload
    };

    enum ObjStrategy : uint8_t {
        OBJ_STATE_EMPTY = 0,
        OBJ_STATE_GETSTATE = 1,
        OBJ_STATE_REDUCE = 2,
        OBJ_STATE_DICT = 3
    };

    // -------- Writer / Reader --------
    class Writer {
    public:
        explicit Writer(std::string& out, const PySerOptions& opt);
        void writeByte(uint8_t b);
        void writeVarU(uint64_t v);
        void writeVarI(int64_t v); // zig-zag
        void writeDouble(double d);
        void writeBytes(const void* p, size_t n);
        void writeString(const char* s, size_t n);
        void writeTag(Tag t);

        size_t size() const { return buf.size(); }
        std::string& buffer() { return buf; }
    private:
        std::string& buf;
        const PySerOptions& opt;
    };

    class Reader {
    public:
        Reader(const char* p, size_t n, const PyDeserOptions& opt);
        bool eof() const { return cur >= end; }
        uint8_t  readByte();
        uint64_t readVarU();
        int64_t  readVarI(); // zig-zag
        double   readDouble();
        void     readBytes(void* dst, size_t n);
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
        std::unordered_map<PyObject*, uint64_t> memo; // borrowed keys; identity
    };

    struct LoadState {
        Reader& r;
        const PyDeserOptions& opt;
        size_t depth = 0;
        std::unordered_map<uint64_t, PyObject*> id_to_obj; // NEW REFs stored
        ~LoadState(); // decref everything on failure unwind
        void track(uint64_t id, PyObject* o); // steals a NEW ref into table
    };

    // -------- Dump helpers --------
    static bool dump_value(PyObject* o, DumpState& S);
    static bool dump_none(DumpState& S);
    static bool dump_bool(bool v, DumpState& S);
    static bool dump_int(PyObject* o, DumpState& S);
    static bool dump_float(PyObject* o, DumpState& S);
    static bool dump_str(PyObject* o, DumpState& S);
    static bool dump_bytes_like(PyObject* o, DumpState& S);
    static bool dump_list(PyObject* o, DumpState& S);
    static bool dump_tuple(PyObject* o, DumpState& S);
    static bool dump_dict(PyObject* o, DumpState& S);
    static bool dump_set(PyObject* o, DumpState& S);
    static bool dump_frozenset(PyObject* o, DumpState& S);
    static bool dump_object(PyObject* o, DumpState& S);

    static bool is_referenceable(PyObject* o);
    static uint64_t ensure_id(PyObject* o, DumpState& S, bool& is_new);

    // Object strategy selection
    static bool try_getstate(PyObject* o, PyObject** out_state);
    static bool try_reduce(PyObject* o, PyObject** out_reduce_tuple);
    static bool try_dict_snapshot(PyObject* o, PyObject** out_dict);

    // -------- Load helpers --------
    static PyObject* load_value(LoadState& L);
    static PyObject* load_list(LoadState& L);
    static PyObject* load_tuple(LoadState& L);
    static PyObject* load_dict(LoadState& L);
    static PyObject* load_set(LoadState& L);
    static PyObject* load_frozenset(LoadState& L);
    static PyObject* load_object(LoadState& L);

    // Object reconstruction
    static PyObject* import_qualified(const std::string& module, const std::string& qualname);
    static PyObject* new_without_init(PyObject* cls);
    static bool set_via_setstate(PyObject* inst, PyObject* state);
    static bool assign_via_dict(PyObject* inst, PyObject* d);
    static PyObject* apply_reduce_tuple(PyObject* tpl, const PyDeserOptions& opt);

    // Utilities
    static inline uint64_t zigzag(int64_t v);
    static inline int64_t unzigzag(uint64_t u);
    static bool py_is_true(PyObject* o);
};
