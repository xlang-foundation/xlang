# Tensor and Compute Model

Crafted specifically for AI, IoT, and compute-heavy architectures, **XLang™** treats quantitative and distributed computation as first-class citizens.

## Built-In Tensor Computing

Traditional scripting environments generally rely on external C-based libraries to bring tensor capabilities into the language. In XLang, optimizing data arrays and tensor math happens directly inside the language's parser and execution core.
- **Optimized Tensor Expressions**: You can transparently construct and evaluate multi-dimensional algorithms using inline primitives.
- **Partial Optimization Layer**: Behind the scenes, XLang lowers numeric executions and vectorizes standard loop structures to prevent interpreter bottlenecking on intensive data manipulation tasks.

## Hardware-Accelerated Native Tensors (Pluggable GPU Support)

To support seamless transitions between CPU memory and hardware-accelerated memory (like CUDA VRAM), `xlang` implements a **Decoupled Instance-Bound Ops Dictionary** architecture.

Because `xlang` is designed to be a highly portable, dependency-free core engine, it does not link against CUDA directly. Instead, GPU capabilities are injected via dynamic language patterns from hardware-aware plugins (such as `xWorld`).

### The `m_deviceOps` Architecture
1. **Device Identification**: The `XTensor` interface natively tracks its memory locality using the `TensorDeviceType` enum (`CPU = 0`, `GPU = 1`, etc.).
2. **Instance-Bound Operations (`m_deviceOps`)**: Each Tensor instance carries an `X::Value` dictionary that holds registered operations (`X::Func` callbacks). 
   - When a plugin like `xWorld` allocates a GPU Tensor, it packages its custom `.cu` CUDA kernels (e.g., `gpuFree`, `gpuPermute`, `gpuAdd`) into this dictionary.
   - It then binds the dictionary to the Tensor instance using `tensor->SetDeviceOps()`.
3. **Op Dispatching**: When an operation like `tensor.permute()` is invoked from an XLang script, the internal execution engine checks the Tensor's device type.
   - If the Tensor is `GPU`-bound, the engine dynamically looks up `"permute"` inside the Tensor's `m_deviceOps` dictionary. If found, it routes the execution directly to the plugin's registered CUDA callback, completely bypassing standard CPU loop logic.
   - If the Tensor is `CPU`-bound (or the op is missing), it routes to the standard `CpuTensor` mathematical backend.
4. **Zero-Leak Memory Management**: When a GPU Tensor goes out of scope, the `~Tensor()` destructor automatically searches the `m_deviceOps` dictionary for a `"free"` key. If present, it executes the callback, allowing the plugin to safely run `cudaFree` on the underlying pointer without `xlang` ever needing to know about the host/device memory distinction.

## GPU Performance Boost

Under a **CUDA-enabled GPU** environment:
- The execution backend seamlessly interfaces to generate **tensor data flow graphs**.
- Target-specific compilations map mathematical and AI inference kernels precisely onto the available hardware logic.
- Due to the engine's elimination of memory boundary conversions—an issue common with thick scripting abstraction layers—real-time AI, computer vision streaming, and high-frequency IoT parsing see massive inference and data throughput improvements natively.
