# XLang IPC: A Transparent, High‑Throughput Inter‑Process Communication Model for AI & IoT

**Keywords —** inter‑process communication (IPC), shared memory, zero‑copy, Arrow IPC, multiprocessing, dataflow graphs, LRCP, remote object proxy

**Author** — Shawn Xiong  

---

## Abstract

We present the design, implementation, and benchmarking methodology of **XLang’s IPC**—a transparent, shared‑memory–first communication model that lets developers import and use **remote objects exactly like local ones**. XLang’s IPC aligns with the language’s core philosophy: programs are compiled into **mathematical expression sets** that preserve a **dataflow graph** for optimization across heterogeneous executors (GPU, edge, cloud). The IPC layer mirrors this philosophy by separating **control** (object introspection, method calls, events) from **data** (bulk payloads via zero‑copy shared memory), yielding low latency and high throughput while keeping code simple:

```xlang
import ipc_srv thru 'lrpc:9089' as srv
y = srv.model.predict(x)   # same syntax as local use
```

We detail architecture and algorithms, provide a reproducible benchmark (XLang vs Python Multiprocessing vs Arrow IPC-on-shared-file), and outline how to evaluate throughput, latency, and CPU overhead. Our results discussion is methodological (to keep the paper tool‑ and hardware‑agnostic), but the suite is turnkey and ready to run.

---

## 1. Background: Objectives, Features, and Design Philosophy

**XLang™** is a dynamic, high‑performance language for AI and IoT. It is \~99% Python‑syntax compatible yet designed to run significantly faster in AI workloads. **Code in XLang is compiled into a set of mathematical expressions**, preserving a **dataflow graph** that the runtime can optimize and schedule on CPUs, GPUs, or other parallel backends. Core features include **natural distributed computing**, **seamless interop** (C/C++/Python/JS), and **transparent cross‑process object access**—all with an emphasis on performance and ease of use.

---

## 2. Motivation to Design a New IPC Method in XLang

Existing high‑level IPCs (e.g., Python MP, ZeroMQ, RPC stacks) are effective but **not language‑native** and often **not transparent**: developers must write different code for remote vs local objects. They also introduce serialization overhead that limits throughput for large AI tensors. XLang’s IPC therefore targets:

* **Transparency:** treat **remote objects** (properties/methods) **like local** ones—same syntax, same semantics.
* **Speed:** bulk data via **shared memory** with **zero‑copy** reads/writes whenever possible.
* **Binary serialization:** compact, schemaed payloads to reduce bytes on the wire/control channel.
* **Events & backpressure:** **event‑driven notifications** for readiness/ack to avoid polling and to maintain flow control.

---

## 3. Programming Model

* **Importing remote modules:**
  `import anotherword thru 'lrpc:PORT' as m` creates a **proxy** for the remote module/object `anotherword` exported by the peer process listening on `PORT`.
* **Uniform object semantics:** `m.var`, `m.method(a,b)` behave as if `m` were local. Under the hood, the runtime performs remote introspection, marshals arguments, and routes data through shared memory if large.
* **Events:** processes can **signal** each other when data is ready or when a method finishes. Clients block (or await) on events rather than spinning.

---

## 4. Architecture

**Control Plane (LRPC).** Lightweight RPC (“**LRPC**”) carries:

* **Object schema:** names, attributes, methods, signatures.
* **Invocations:** compact binary envelopes describing target object, method id, and small arguments.
* **Signals:** event notifications (ready, ack, closed).

**Data Plane (Shared Memory).**

* **Large payloads** (e.g., tensors, byte buffers) are exchanged via **memory‑mapped regions** (Windows: `CreateFileMapping`/`MapViewOfFile`; Linux: `shm_open`/`mmap`).
* A **slice protocol** divides large buffers into **fixed‑size chunks** (e.g., 1 MiB). Each chunk publish is accompanied by an **event**; the receiver acks to apply **backpressure**.

**Serialization.**

* **Metadata/control**: compact binary (fixed headers + varints for sizes).
* **Bulk data**: zero‑copy views where safe (e.g., native numeric tensors) or **binary blocks** with minimal framing when copy is unavoidable.

**Safety & Lifetimes.**

* **Proxy references** hold a cross‑process **object token**. The server pins the object until all proxies release.
* **Region leases** (read/write) ensure no writer tears while a reader is active.
* **Time‑bounded waits** + failure paths (timeout/cancel) for robustness.

---

## 5. Algorithms

### Algorithm 1 — Server: `PrepareData(length, sliceLen)`

```
Input: length L, slice size S
State: data buffer B (shared), write index p = 0
1: Allocate/resize shared region |B| >= L; fill B with fast pattern or generated data
2: Set p ← 0; publish {L, S} in control plane
3: Signal “ready”
```

### Algorithm 2 — Server: `GetData()`

```
State: current position p, total L, slice size S
1: if p ≥ L: return NULL
2: end ← min(p + S, L)
3: Return a read-only view B[p:end]
4: p ← end
```

### Algorithm 3 — Client: Fetch Loop

```
Input: Total length L, slice size S (from server)
State: received = 0
1: t0 ← now()
2: while received < L:
3:     D ← remote.GetData()
4:     if D == NULL: break
5:     process(D)         # e.g., checksum, sum bytes, hash
6:     received += |D|
7: t1 ← now(); report Δ = t1 - t0, throughput = L / Δ
```

---

## 6. Implementation

### 6.1 XLang Reference Benchmark (one file; server/client via args)

```xlang
# xlang_ipc_benchmark.x
import time

# Globals
data = None
data_len = 0
slice_len = 0
current_position = 0

def PrepareData(length, slice_length):
    global data, data_len, slice_len, current_position
    data_len = length
    slice_len = slice_length
    # Fast generator: bytes(total_len, low_char, high_char, GenData=True)
    data = bytes(data_len, 48, 122, GenData = True)
    current_position = 0
    print("Data prepared with length ${data_len} and slice length ${slice_len}")

def GetData():
    global current_position
    if current_position >= data_len:
        return None
    end_position = current_position + slice_len
    if end_position > data_len:
        end_position = data_len
    part = data[current_position:end_position]
    current_position = end_position
    return part

def close():
    global data, data_len, slice_len, current_position
    data = None; data_len = 0; slice_len = 0; current_position = 0
    print("Data closed and resources released")

def main():
    args = get_args()
    mode = args[0] if args.size() > 0 else 'client'
    port = 9089

    if mode == 'server':
        register_remote_object("ipc_srv")
        PrepareData(1024*1024*1024, 1024*1024)   # 1 GiB, 1 MiB slices
        print("Starting as server...")
        lrpc_listen(port, True)

    elif mode == 'client':
        import ipc_srv thru 'lrpc:9089' as server
        print("Starting as client...")
        t0 = time.time()
        while True:
            chunk = server.GetData()
            if chunk == None:
                break
            # optional: compute checksum / accumulate
        t1 = time.time()
        print("Time taken: ${t1 - t0} seconds")
        server.close()
    else:
        print("Unknown mode: ${mode}")

main()
```

**How to run**

```
xlang xlang_ipc_benchmark.x server
xlang xlang_ipc_benchmark.x client
```

### 6.2 Python Baseline A — Multiprocessing (Queue)

```python
# ipc_benchmark_mp.py
import multiprocessing as mp, time, sys

def server(q_cmd, q_data):
    data_len = 1024*1024*1024
    slice_len = 1024*1024
    def gen_slice(n):  # cheap deterministic bytes
        return bytes((48 + (i % 75) for i in range(n)))
    while True:
        msg = q_cmd.get()
        if msg[0] == "prepare":
            _, L, S = msg; data_sent = 0
            while data_sent < L:
                sz = min(S, L - data_sent)
                q_data.put(gen_slice(sz))
                data_sent += sz
            q_data.put(None)
        elif msg[0] == "close":
            break

def client(q_cmd, q_data, L=1024*1024*1024, S=1024*1024):
    q_cmd.put(("prepare", L, S))
    t0 = time.time(); rcv = 0
    while True:
        part = q_data.get()
        if part is None: break
        rcv += len(part)
    t1 = time.time()
    print(f"MP: bytes={rcv} time={t1-t0:.3f}s throughput={rcv/1e6/(t1-t0):.1f} MB/s")
    q_cmd.put(("close",))

if __name__ == "__main__":
    mode = sys.argv[1] if len(sys.argv) > 1 else "all"
    q_cmd, q_data = mp.Queue(), mp.Queue()
    if mode in ("server","all"):
        p = mp.Process(target=server, args=(q_cmd,q_data)); p.start()
    if mode in ("client","all"):
        client(q_cmd, q_data)
```

### 6.3 Python Baseline B — Arrow IPC on a Shared File (slice stream)

This baseline **does not serialize 1 GiB at once**. The server writes an **Arrow IPC stream of 1 MiB record batches** (each batch is a `uint8` column) to a file; the client memory‑maps the file and reads the stream sequentially. This models per‑slice Arrow serialization overhead.

```python
# ipc_benchmark_arrow_ipc.py
import pyarrow as pa, pyarrow.ipc as ipc
import numpy as np, time, sys, os

FILE = "shared.arrow"
SIG  = "ready.signal"
L    = 1024*1024*1024
S    = 1024*1024

def gen_slice(n):
    # vectorized: values in [48,122)
    return np.fromfunction(lambda i: 48 + (i % 74), (n,), dtype=np.uint8)

def server():
    schema = pa.schema([("b", pa.uint8())])
    with pa.output_stream(FILE) as sink, ipc.new_stream(sink, schema) as w:
        remain = L
        while remain > 0:
            sz = min(S, remain)
            arr = pa.array(gen_slice(sz))
            w.write_batch(pa.record_batch([arr], schema=schema))
            remain -= sz
    open(SIG, "w").close()
    print("Arrow stream ready.")

def client():
    while not os.path.exists(SIG):
        time.sleep(0.05)
    t0 = time.time()
    total = 0
    with pa.memory_map(FILE, "r") as src, ipc.open_stream(src) as r:
        for batch in r:
            total += batch.num_rows
    t1 = time.time()
    print(f"Arrow IPC: bytes={total} time={t1-t0:.3f}s "
          f"throughput={total/1e6/(t1-t0):.1f} MB/s")

if __name__ == "__main__":
    mode = sys.argv[1] if len(sys.argv) > 1 else "client"
    if mode == "server": server()
    elif mode == "client": client()
```

> **Note**: This Arrow baseline uses a shared file (mmap‑capable) and **Arrow IPC stream** per slice; it is portable (Windows/Linux/macOS) and keeps memory bounded.

---

## 7. Benchmark Methodology

### 7.1 Metrics

* **Throughput (MB/s):** `total_bytes_transferred / elapsed_seconds / 1e6`
* **End‑to‑end latency:** client wall‑clock time from first request to last byte.
* **CPU utilization:** per process (optional).
* **Copy count:** estimated number of user‑space copies per slice (qualitative).

### 7.2 Workloads

* **W1 (bytes):** 1 GiB of byte data in 1 MiB slices (as shown).
* **W2 (tensors):** replace bytes with `float32`/`int8` tensors to test zero‑copy paths.
* **W3 (mixed):** control‑heavy sequences (many small calls) + occasional large slices.

### 7.3 Procedure

1. **Warm up** once per mode to stabilize caches.
2. Run **at least 5 trials** per configuration; report mean and standard deviation.
3. Pin server/client to separate cores if possible to reduce scheduler noise.
4. Disable background tasks; use release builds.

### 7.4 What to Expect (Qualitative)

* **XLang IPC (shared memory + events):** minimal copy paths and transparent object semantics should deliver the **highest throughput** and **lowest CPU** for large slices.
* **Python MP (Queue/Pipe):** convenient but incurs pickling/copy overheads—**lower throughput**, higher CPU.
* **Arrow IPC (slice stream):** efficient columnar encoding, but **per‑slice serialization framing** adds overhead compared to XLang’s raw binary slices; throughput is typically **between** the other two depending on slice size and disk/mmap performance.

*(Provide your actual numbers in a table once you run on your target hardware.)*

---

## 8. Analysis & Discussion

* **Transparency vs ceremony.** XLang hides remote/local differences. Baselines require explicit server scafolding (MP) or format generation (Arrow).
* **Copy budget.** XLang’s design aims for **zero extra copies** in the data plane; MP and Arrow both copy/encode per slice.
* **Backpressure.** XLang’s event‑driven acks reduce buffering and tail latency; MP Queues can build pressure; Arrow’s stream reader keeps state but not end‑to‑end flow control.
* **Executor affinity.** Because XLang preserves a **dataflow graph** of expressions, IPC can be **fused** with compute (e.g., GPU pre/post‑processing near the producer or consumer), cutting bytes‑on‑the‑wire—beyond what generic IPC stacks offer.

---

## 9. Limitations & Future Work

* **Cross‑host transport.** The shared‑memory fast path is local‑only; XLang falls back to network transports for remote hosts. Future work: RDMA/GPUDirect paths.
* **Security/isolation.** Named shared memory requires ACLs; future: per‑session keys and encrypted control plane.
* **Schema evolution.** Today’s binary envelopes are compact; richer schemas (e.g., Arrow schema negotiation) could improve cross‑language interop when needed.
* **Zero‑copy Arrow interop.** Map Arrow arrays directly onto shared regions to eliminate slice serialization in mixed stacks.

---

## 10. Related Work

* **Python Multiprocessing:** high‑level, batteries‑included IPC; simplicity at the cost of overhead for large payloads.
* **Apache Arrow IPC/Flight:** standardized columnar interchange; excellent tool for analytics pipelines; Flight (gRPC) is network‑centric, while file IPC streams fit local use.
* **ZeroMQ/gRPC:** powerful messaging/RPC stacks, but not language‑transparent for local/remote object unification.

---

## 11. Conclusion

XLang’s IPC delivers **transparent remote object access** and **shared‑memory performance** aligned with the language’s expression‑graph design. By cleanly separating control (LRPC) and data (zero‑copy slices + events), it achieves high throughput with minimal code changes. The provided benchmarks and baselines make it straightforward to reproduce results and to quantify gains on your hardware.

---

## Appendix A — How to Run

**XLang**

```
xlang xlang_ipc_benchmark.x server
xlang xlang_ipc_benchmark.x client
```

**Python MP**

```
python ipc_benchmark_mp.py all        # spawns server + runs client in one process
# or in two terminals:
python ipc_benchmark_mp.py server
python ipc_benchmark_mp.py client
```

**Python Arrow IPC**

```
pip install pyarrow
python ipc_benchmark_arrow_ipc.py server
python ipc_benchmark_arrow_ipc.py client
```

---

## Appendix B — Result Table Template

| Workload | Method             | Bytes (GiB) | Slice (MiB) | Time (s) | Throughput (MB/s) | Notes |
| -------- | ------------------ | ----------- | ----------- | -------- | ----------------- | ----- |
| W1       | XLang IPC          | 1.0         | 1           |          |                   |       |
| W1       | Python MP (Queue)  | 1.0         | 1           |          |                   |       |
| W1       | Arrow IPC (Stream) | 1.0         | 1           |          |                   |       |

*(Fill after running; include CPU% if you measure it.)*

---

### Acknowledgments

Thanks to the XLang Foundation and community for the APIs (`import ... thru 'lrpc:PORT'`, `register_remote_object`, `lrpc_listen`) and the shared‑memory primitives that make this design possible.
