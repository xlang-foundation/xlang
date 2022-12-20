#How to call python built-in function
https://stackoverflow.com/questions/64135495/how-to-call-a-python-built-in-function-in-c-with-pybind11
``` python
py::object builtins = py::module_::import("builtins");
py::object range = builtins.attr("range");
range(0, 10);
```


