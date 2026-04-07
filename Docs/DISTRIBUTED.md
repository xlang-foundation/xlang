# Distributed and IPC Overview

Modern workflows assume processes don't live in a siloed thread or run independently on a single machine. **XLang™** intrinsically provides semantics and infrastructure to tackle concurrency, node orchestration, and edge deployment safely.

## Concurrency and Event Flows

A core design aspect is acting as a "Glue Language" that securely links disconnected execution subsystems.
- **Native Thread Safety**: The interpreter loop protects cross-thread value sharing directly at the AST node level, simplifying the creation of data pipelines and preventing obscure race conditions.
- **Event-Driven Resilience**: Built upon an asynchronous event-bus handling lightweight event dispatch. It supports resolving asynchronous tasks and integrating with an overarching event loop seamlessly, making XLang capable of processing simultaneous interactions cleanly.

## Remote Orchestration and AI Tooling

By adopting standards for recursive task management and tool dispatching (often coordinating with AI agents), XLang nodes communicate fluidly. An embedded `Cantor.Connect` interop pattern enforces structured RPC, allowing individual device nodes or agents to seamlessly broadcast statuses, accept logic closures, and pull new compiled script block executions reliably from command nodes.

## High-Performance Inter-Process Communication (IPC)

XLang ships with a native, zero-friction IPC layer specifically built to traverse process boundaries. It is engineered to solve a difficult problem: maintaining legal (e.g. GPL 3.0) or security boundaries via process isolation without destroying the throughput required by high-framerate tensor and video operations. 

In XLang IPC, communication is split into two specialized planes:

1. **Control Plane (LRPC)**: 
   Uses extremely compact binary envelopes to serialize method calls, properties, configuration, and event notifications. 
2. **Data Plane (Shared Memory Slices)**: 
   For heavy payloads like video streams or megabyte-sized float tensors, XLang skips TCP entirely and falls back to a memory-mapped zero-copy protocol.

To the developer, distributing objects or APIs across processes is radically simple. Below is an example where a proprietary engine interacts with an isolated streaming/encoding server over IPC:

```python
# The client merely imports the exposed server via LRPC interface
import sunshine_srv thru 'lrpc:9090' as sunshine

# Remote objects feel entirely local with object introspection natively supported
session = sunshine.StreamController()
session.StartSession(display_id=0)

# The runtime automatically marshals bulk tensor stats silently
stats = sunshine.EncoderStats()
print("Latency:", stats.latency_ms, "Bitrate:", stats.bitrate)
```

By decoupling control packets and bulk data payload sharing (handling bytes via slice algorithms over SHM under the hood), streaming 4K240 RAW or inference tensors crosses the boundary at speeds nearing internal memory-bandwidth margins. See the *[Using XLang IPC to Enforce GPL 3.0 Document](Using%20XLang%20IPC%20to%20Enforce%20GPL%203.md)* for a deeper architectural dive on this feature.
