# How to call python built-in function
https://stackoverflow.com/questions/64135495/how-to-call-a-python-built-in-function-in-c-with-pybind11
``` python
py::object builtins = py::module_::import("builtins");
py::object range = builtins.attr("range");
range(0, 10);
```


# How to install xlang python package
 for windows only,in the release bin folder, run the following command:
 ```bash
 install_py_xlang.bat
 ``` 


