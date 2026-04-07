# Embedding and API Exposure

**XLang™** is fundamentally built as an embedded engine. It stands out by breaking the traditional boundary between an external scripting language and the core system.

## Key Advantages

- **Lightweight Architecture**: XLang is structured with a minimal, optimized native stack, allowing it to be effortlessly injected into constrained or latency-sensitive game engines, IoT software, and distributed servers.
- **Robust Event-Loop & Runtime**: Featuring a fully validated parser and asynchronous runtime, it natively integrates with application-level timing cycles, rather than forcing the application to work around a disconnected garbage-collector loop.
- **Effortless API Exposure**: Exposing host-application logic or hardware capabilities does not require convoluted extension architectures like Python's C-Extensions. XLang allows "in-site" binding directly against native variables and method pointers.
- **Native Data Wrapping**: The language uses established data marshalling patterns (`X::Value`), ensuring memory safety without complex serialization overheads when passing native dictionaries or lists. 

## Python Ecosystem Interop

In addition to serving as a host embedding language, XLang acts as an embedded inter-process interface to the broader Python ecosystem (`PyEng`), allowing a host application to effortlessly tap into AI ecosystems (PyTorch, TensorFlow) while orchestrating logic within the safer XLang event boundary.
