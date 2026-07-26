# Embedding a Private Python Runtime in an XLang Application

This guide describes how an application can package Python with XLang, install
application-local Python dependencies, call Python from C++, expose XLang to
Python, and launch private Python child processes without using or modifying the
operating system's Python installation.

It is also written as implementation context for coding LLMs. Read the
architecture and invariants before changing the integration.

## 1. Intended architecture

The XLang tool installation and the initialized application project are
different locations.

The XLang installation/build provides the installer and source bridge:

```text
XLangInstall/
`-- bin/
    |-- xlang.exe
    |-- xlang_eng.dll
    |-- pyeng.cpXY.dll
    `-- pyeng.cpXY.version.txt
```

These files do **not** need to be copied into every project by
`xlang init python`.

The command initializes only the project-local Python runtime:

```text
MyApplication/
`-- .xlang/
    `-- python/
        |-- python.exe
        |-- pythonXY.dll
        |-- pythonXY.zip
        |-- pythonXY._pth
        |-- .xlang-runtime
        `-- Lib/
            `-- site-packages/
                |-- pip/
                |-- xlang.pyd
                |-- xlang_engine_path.txt
                `-- application dependencies...
```

`xlang.pyd` is copied from the ABI-matched PyEng binary.
`xlang_engine_path.txt` contains the absolute directory of the XLang CLI that
initialized the project. `xlang.pyd` reads it to locate `xlang_eng.dll`
without copying the engine into private Python.

If an application only uses `xlang pip` and `xlang python`, the globally
installed/built `xlang.exe` can remain outside the project. If the final
application embeds XLang in its own native process, its packaging step must
separately include the native XLang engine and ABI-matched PyEng in an
application resource directory.

There are two Python execution models:

1. **Embedded Python** runs inside the application process through PyEng.
   Existing `XHost::PyRun`, `PyImport`, serialization, and conversion APIs use
   this interpreter.
2. **Python child processes** are launched with
   `.xlang/python/python.exe`. They do not share memory with the embedded
   interpreter and should communicate with the application through XLang IPC or
   another explicit application protocol.

Do not use `xlang.exe` as a replacement executable for `python.exe`. Use
`xlang python ...` when a command-line convenience wrapper is desired; it
launches the project-private `python.exe`.

## 2. Critical invariants

- A packaged application must never silently fall back to an unrelated OS
  Python.
- The native host selects its Python runtime through the C++ configuration API,
  not an XLang command-line option.
- A PyEng binary supports the CPython minor ABI in its tag. For example,
  `pyeng.cp312.dll` supports Python 3.12.x, while Python 3.13.x requires
  `pyeng.cp313.dll`.
- A new PyEng build is normally needed per Python **minor** version, not per
  patch version.
- Renaming an ABI-tagged PyEng binary to `pyeng.dll` does not make it
  compatible with another Python minor version.
- The generic `pyeng.dll` is retained only as a legacy compatibility copy.
- Python project setup, CLI parsing, process launching, runtime discovery, and
  PyEng initialization must remain in separate C++ components.
- Existing `XHost` Python APIs must remain source compatible.

PyEng currently uses the full CPython and NumPy C APIs. It cannot be made
minor-version independent merely by enabling Python's limited API; that would
require a separate compatibility refactor.

### Supported Python versions

The Python versions supported by an XLang release are determined by the
`pyeng.cpXY` artifacts shipped with that release, not by the Python versions
installed on an end user's machine.

For example, this development machine currently produces `pyeng.cp312.dll`
because its configured build SDK is Python 3.12. That is a property of this
local build, not a product decision that XLang supports only Python 3.12.

A release build or CI system can produce and package:

```text
pyeng.cp312.dll
pyeng.cp312.version.txt
pyeng.cp313.dll
pyeng.cp313.version.txt
```

An end user does not need either Python version installed globally.
`xlang init python` downloads the official runtime matching the selected
packaged bridge. Only the machine building a PyEng artifact needs the
corresponding CPython development headers, import library, and compatible NumPy
headers.

## 3. Initialize an application project

Run from the application project:

```powershell
xlang init python
```

The current folder is used by default:

```text
current-folder/.xlang/python
```

An explicit project folder is also supported:

```powershell
xlang init python D:\Applications\MyApplication
```

By default, XLang reads the version metadata packaged beside its PyEng binary:

```text
pyeng.cpXY.dll
pyeng.cpXY.version.txt
```

It then downloads the matching official CPython embeddable distribution. This
prevents an XLang build from downloading a Python minor version for which it
does not contain a bridge.

An explicit version can be requested:

```powershell
xlang init python --version 3.13.14
xlang init python D:\Applications\MyApplication --version 3.13.14
```

The requested version still requires an ABI-matched bridge beside `xlang.exe`.
An explicit version must still have a bridge with the same minor ABI.

The initializer:

1. downloads the official Windows embeddable CPython distribution;
2. extracts it into `.xlang/python`;
3. enables `Lib/site-packages` and `import site` in `pythonXY._pth`;
4. copies the ABI-matched PyEng binary as `xlang.pyd`;
5. dynamically writes `xlang_engine_path.txt` beside `xlang.pyd`, pointing to
   the directory containing the `xlang.exe` and `xlang_eng.dll` used for
   initialization;
6. bootstraps pip unless `--without-pip` was specified; and
7. writes `.xlang-runtime` only after initialization succeeds.

An incomplete runtime is not considered initialized.

## 4. Install private dependencies

Use:

```powershell
xlang pip install requests
xlang pip install numpy
xlang -pip install pandas
```

From outside the application folder:

```powershell
xlang pip --project D:\Applications\MyApplication install requests
```

`xlang pip` searches the selected folder and its parents for a valid
`.xlang/python` installation. It requires:

- `python.exe`;
- `.xlang-runtime`;
- `Lib/site-packages/xlang.pyd`; and
- `Lib/site-packages/xlang_engine_path.txt`, whose target contains
  `xlang_eng.dll`.

If these files are absent, the command stops and tells the user to run
`xlang init python`. It does not invoke an OS-installed Python or pip.

Dependencies installed this way affect only the application-private runtime.
They do not affect other Python installations, and packages installed into
other Python installations do not automatically appear in the application.

Native wheels must support the packaged Python minor version and architecture.
For example, a Python 3.13 Windows x64 runtime needs compatible `cp313` Windows
x64 wheels.

For reproducible releases, keep the dependency specification in the
application repository and install exact versions during packaging:

```powershell
xlang pip install -r requirements.lock
```

## 5. Run the private Python from the CLI

The `xlang python` command passes arguments directly to the project-private
interpreter:

```powershell
# Interactive Python
xlang python

# Inline code
xlang python -c "import sys; print(sys.executable)"

# A Python file and its arguments
xlang python scripts\worker.py input.json

# A Python module
xlang python -m http.server 8080

# An explicitly selected project
xlang python --project D:\Applications\MyApplication scripts\worker.py
```

The wrapper:

- inherits the terminal's standard input, output, and error handles;
- waits for the Python child process;
- returns the Python process's exit code; and
- never substitutes an OS Python when project initialization is missing.

`--project PATH` is an XLang wrapper option only when it immediately follows
`xlang python`. All remaining arguments are passed to Python unchanged.

## 6. Select the private runtime from a native application

The application computes an absolute path to its packaged Python executable and
passes it to `X::Config` before loading XLang:

```cpp
#include "xload.h"

X::Config config;

// Configure appPath, appFullName, and other normal XLang host fields.
// Use the application's actual installation root rather than the process
// working directory.

config.SetPythonExecutable(
    "D:\\Applications\\MyApplication\\.xlang\\python\\python.exe");

X::XLoad xload;
int loadResult = xload.Load(&config);
if (loadResult != 0)
{
    // Report that the XLang engine could not be loaded.
    return loadResult;
}

int runResult = xload.Run();

// Application work...

xload.Unload();
return runResult;
```

`SetPythonExecutable` copies and owns the path. The path must be configured
before `XLoad::Load`/`Run` initializes PyEng.

When the path is present, XLang:

1. launches that exact interpreter only to query its executable, home, ABI tag,
   and runtime-library path;
2. finds an ABI-matched PyEng binary;
3. preloads the selected Python runtime library;
4. calls PyEng's backward-compatible `LoadEx` entry point; and
5. initializes CPython with isolated `PyConfig` values for the selected
   executable and home.

Do not pass the private Python path as a general `xlang.exe` engine option. The
native application owns this decision.

### System Python compatibility

If the native host does not call `SetPythonExecutable`, XLang preserves its
legacy Python discovery behavior. This can be useful for development tools, but
a packaged application should always pass its private absolute path.

## 7. Calling Python from C++

The selected embedded interpreter is used by the existing methods in
`Api/xhost.h`:

```cpp
virtual bool PyAddImportPaths(X::Value& paths) = 0;
virtual bool PyRemoveImportPaths(X::Value& paths) = 0;
virtual bool PyRun(const char* code, X::ARGS& args) = 0;
virtual bool PyImport(
    XRuntime* rt,
    const char* moduleName,
    const char* from,
    const char* currentPath,
    X::Value& pyObj) = 0;
virtual bool PyImportWithGlobals(
    XRuntime* rt,
    const char* moduleName,
    const char* from,
    const char* currentPath,
    X::KWARGS& globals,
    X::Value& pyObj) = 0;
virtual bool PySerialize(X::Value& input, X::Value& output) = 0;
virtual bool PyDeserialize(X::Value& input, X::Value& output) = 0;
virtual bool PyObjToValue(void* pyObj, X::Value& valObject) = 0;
```

No separate environment or process API is required for these calls. They
operate on embedded CPython after XLang has initialized PyEng.

Use a child process when isolation, crash containment, parallel worker
lifetime, or command-line compatibility is required. Use the embedded APIs when
the application needs direct object conversion and low-latency in-process
calls.

## 8. Python calling XLang

The private runtime contains `xlang.pyd`, so a private Python process can load
XLang:

```powershell
xlang python -c "import xlang; print(xlang.__file__)"
```

`xlang_engine_path.txt` must be beside `xlang.pyd` and must point to a
directory containing `xlang_eng.dll`; otherwise the import raises
`ImportError`. The module initializer must not dereference an unavailable
XLang host.

An embedded Python interpreter can exchange objects directly through PyEng.
A separate Python process cannot use the application's in-memory XLang objects.
It should connect through XLang local RPC/IPC and exchange explicit,
serializable messages.

See [XLang IPC](../Docs/xlang-ipc.md) for the transport model. The application
should define a small stable service contract, for example:

```text
Python worker                         Application
-------------                         -----------
connect/register  ------------------> XLang IPC endpoint
request(method, payload) -----------> dispatch application operation
response(result/error) <------------- serialize bounded response
event(name, payload) <--------------- optional application event
shutdown/close ----------------------> release worker resources
```

Do not assume that importing `xlang` automatically connects a child process to
the parent application. Connection address, authentication/capability policy,
timeouts, cancellation, and shutdown behavior belong to the application's IPC
contract.

## 9. Electron application pattern

For an Electron application:

1. package XLang and the private Python runtime inside the application;
2. initialize XLang from the native/main-process integration;
3. call `SetPythonExecutable` with the packaged absolute `python.exe` path;
4. expose narrow application operations to renderer JavaScript through the
   Electron preload/main IPC boundary; and
5. start Python workers with `xlang python` or the exact private `python.exe`.

Recommended flow:

```text
Renderer JavaScript
        |
        | Electron IPC (narrow allow-listed methods)
        v
Electron main/native host
        |
        +--> embedded XLang/PyEng --> private CPython
        |
        +--> private Python child --> XLang IPC --> application service
```

Do not initialize a separate embedded CPython in every renderer. Do not expose
an unrestricted "execute arbitrary Python" operation to untrusted renderer
content.

## 10. Packaging

There are two packaging cases.

### Python-only project runtime

If users have XLang installed separately and the application only needs its
private Python files, package:

```text
.xlang/python/**
```

The project does not need its own copy of `xlang.exe`,
`pyeng.cpXY.version.txt`, or the source `pyeng.cpXY.dll`. The initializer has
already copied the required bridge into private Python as `xlang.pyd`.

### Application that embeds XLang

An application that loads XLang through the native `XLoad` API must also
package XLang's native engine files. They can live in a resource directory;
they do not have to be in the application root:

```text
MyApplication/
|-- MyApplication.exe
|-- resources/
|   `-- xlang/
|       |-- xlang_eng.dll
|       `-- pyeng.cpXY.dll
`-- .xlang/
    `-- python/**
```

`Config::appPath`, `Config::dllSearchPath`, and
`Config::SetPythonExecutable` must point to the corresponding packaged
locations.

`xlang.exe` is needed inside the final application bundle only when the
application intends to expose or internally invoke the XLang CLI. It is not
required merely because the application embeds `xlang_eng.dll`.

If the packaged application imports `xlang` from standalone Python, retain:

```text
.xlang/python/Lib/site-packages/xlang.pyd
.xlang/python/Lib/site-packages/xlang_engine_path.txt
```

The pointer is generated with the initializer CLI's absolute engine directory.
If packaging relocates the engine, the packaging/installation step must
regenerate this small text file to point to the packaged native XLang resource
directory. Do not copy `xlang_eng.dll` into Python `site-packages`.

The `.version.txt` sidecar belongs beside the installer-side PyEng binary. It
is required when `xlang init python` needs to choose a default Python version,
but it is not required inside an already initialized project's runtime.

Do not package caches, credentials, temporary downloads, or a developer's
unrelated global `site-packages`.

## 11. Build PyEng

The normal build emits an ABI-tagged PyEng file, its version metadata, and a
legacy compatibility copy. Using `cpXY` as the selected build ABI:

```text
pyeng.cpXY.dll
pyeng.cpXY.version.txt
pyeng.dll
```

The default PyEng target uses the Python found by CMake. Optional multi-version
builds can use configured Python roots such as `PY312_ROOT` and `PY313_ROOT`.
Each resulting binary must retain its ABI tag.

For a product that supports only one private Python version, build and package
only that version. Supporting multiple minor versions is optional.

## 12. Troubleshooting

### `No ABI-matched XLang Python bridge was found`

Example mismatch:

```text
No ABI-matched XLang Python bridge was found at ...\pyeng.cpXY.dll
```

The requested/downloaded Python minor version does not match the packaged
PyEng. For plain `xlang init python`, verify that these are together:

```text
pyeng.cpXY.dll
pyeng.cpXY.version.txt
```

Then rebuild `xlang` and run the updated executable. Check which executable the
shell resolves:

```powershell
Get-Command xlang | Select-Object Source
```

### `.xlang\python exists but is not a complete Python runtime`

A previous setup was interrupted. Confirm that it is the application's
incomplete `.xlang/python` folder, remove only that folder, and initialize
again:

```powershell
Remove-Item .xlang\python -Recurse -Force
xlang init python
```

Never recursively remove an unresolved, empty, root, home, or workspace path.

### `xlang pip` or `xlang python` says the project is not initialized

Run:

```powershell
xlang init python
```

Also confirm that the command is running inside the project or pass
`--project`.

### `import xlang` fails

Verify:

```text
Lib/site-packages/xlang.pyd
Lib/site-packages/xlang_engine_path.txt
```

The `xlang.pyd` ABI must match the private interpreter. The first line of
`xlang_engine_path.txt` must be an existing directory containing
`xlang_eng.dll`.

### The wrong Python executable is used

For CLI child processes:

```powershell
xlang python -c "import sys; print(sys.executable); print(sys.prefix)"
```

For a packaged native host, confirm that it calls `SetPythonExecutable` with an
absolute application-private path before XLang initialization.

## 13. Acceptance checks

Run these checks from a newly initialized disposable application project:

```powershell
xlang init python
xlang python -c "import sys; print(sys.executable); print(sys.prefix)"
xlang python -c "import xlang; print(xlang.__file__)"
xlang pip --version
xlang pip install -r requirements.lock
xlang python scripts\smoke_test.py
```

Verify:

- every printed Python path is under the application project's
  `.xlang/python`;
- `xlang.pyd` loads from private `site-packages`;
- the configured native host loads the matching `pyeng.cpXY.dll`;
- a child Python exit code is returned unchanged by `xlang python`;
- running outside an initialized project does not invoke OS Python; and
- loading/unloading the application does not modify another Python install.

## 14. Prompt template for a coding LLM

Use the following prompt when integrating XLang and private Python into another
application:

```text
Integrate this application with XLang and an application-private CPython
runtime. Follow xlang/doc/EMBEDDED_PYTHON_APP_INTEGRATION.md.

Requirements:
1. Resolve the packaged application root independently of the current working
   directory.
2. Use <app-root>/.xlang/python/python.exe.
3. Before XLang initialization, call
   X::Config::SetPythonExecutable(absolute_private_python_path).
4. Preserve all existing XHost Py* APIs. Do not replace embedded calls with
   subprocesses.
5. For isolated workers, launch only the private Python executable (or
   `xlang python`) and use an explicit XLang IPC application contract.
6. Never fall back to OS Python or pip for a packaged application.
7. Keep CLI parsing, Python project setup, child-process execution, runtime
   discovery, and PyEng initialization in separate source files.
8. Always package the complete private runtime and locked Python
   dependencies. Only when the application itself embeds XLang, also package
   xlang_eng and the ABI-matched pyeng.cpXY binary in a native resource
   directory. Do not copy xlang.exe or PyEng version metadata into every
   project unless the application also ships the XLang initializer CLI.
9. Do not modify unrelated repositories or application components.
10. Add tests proving the exact sys.executable, xlang module path, pip path,
    ABI match, child exit-code forwarding, missing-project guard, and native
    host load/unload behavior.

First inspect the application's existing native host, packaging scripts, and
process/IPC conventions. Then implement the smallest compatible integration.
Report the exact files changed and the verification results.
```

## 15. Relevant implementation files

- `Api/xload.h`: native host configuration and `SetPythonExecutable`.
- `Api/xhost.h`: existing embedded Python APIs.
- `Inc/PyEngConfig.h`: ABI-safe configuration passed to PyEng.
- `Main/PythonRuntime.cpp`: exact-runtime probing.
- `Main/PyLoad.cpp`: ABI-matched PyEng resolution.
- `Main/DllEntry.cpp`: Python runtime and PyEng loading.
- `PyEng/main.cpp`: configured CPython initialization and `LoadEx`.
- `PyEng/PyBind.cpp`: Python `import xlang` initialization.
- `PyEng/CMakeLists.txt`: ABI-tagged PyEng and version metadata.
- `Tools/PythonCli.cpp`: `init python`, `pip`, and `python` command routing.
- `Tools/PythonProject.cpp`: private-runtime installation and discovery.
- `Tools/PythonProcess.cpp`: safe child-process execution.
