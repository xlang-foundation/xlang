# Embedding and API Exposure

**XLang™** is fundamentally built as an embedded engine. It stands out by breaking the traditional boundary between an external scripting language and the core system.

## Key Advantages

- **Lightweight Architecture**: XLang is structured with a minimal, optimized native stack, allowing it to be effortlessly injected into constrained or latency-sensitive game engines, IoT software, and distributed servers.
- **Robust Event-Loop & Runtime**: Featuring a fully validated parser and asynchronous runtime, it natively integrates with application-level timing cycles, rather than forcing the application to work around a disconnected garbage-collector loop.
- **Effortless API Exposure**: Exposing host-application logic or hardware capabilities does not require convoluted extension architectures like Python's C-Extensions. XLang allows "in-site" binding directly against native variables and method pointers.
- **Native Data Wrapping**: The language uses established data marshalling patterns (`X::Value`), ensuring memory safety without complex serialization overheads when passing native dictionaries or lists. 

## Exposing C++ APIs

XLang offers an incredibly streamlined set of C++ macros (`BEGIN_PACKAGE` and `APISET()`) that dynamically map native class methods and properties to the script runtime. 

Here is a common pattern for exposing a host application module directly to XLang:

```cpp
#include "xlang.h"

class Window : public ControlBase {
public:
  // Dynamically expose the class to the XLang runtime
  BEGIN_PACKAGE(Window)
    ADD_BASE(ControlBase);
    
    // Bind XLang native events
    APISET().AddEvent("OnDraw");
    APISET().AddEvent("OnSize");
    
    // Map related types and namespaces directly
    APISET().AddClass<0, Button, Window>("Button");
    
    // Wire native C++ methods cleanly
    APISET().AddFunc<4>("CreateChildWindow", &Window::CreateChildWindow);
    APISET().AddFunc<1>("SetMenu", &Window::SetMenu);
  END_PACKAGE

  Window(Window* parent) : ControlBase(parent) {}
  Window() : ControlBase(nullptr) {}
};
```

This drastically reduces boilerplate code. The developer does not need to write serializers, converters, or wrapper layers. Once exposed, the type drops into the XLang interpreter seamlessly and acts as a native datatype:

```python
# Script consuming the embedded C++ API logic
win = ui.Window()
win.SetMenu(my_menu)
win.OnDraw += handle_draw
```

## Advanced Python Ecosystem Interop

In addition to serving as a host embedding language, XLang acts as an embedded inter-process interface to the broader Python ecosystem (`PyEng`). This allows a host C++ application to effortlessly tap into AI ecosystems (PyTorch, TensorFlow) without leaving the faster and safer XLang event boundary, bringing Python's capabilities natively into the application's address space.
