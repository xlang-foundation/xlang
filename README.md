---

# XLang™

XLang™ is a next-generation programming language crafted for AI and IoT applications, designed to deliver dynamic, high-performance computing. It excels in distributed computing and offers seamless integration with popular languages like C++, Python, and JavaScript, making it a versatile choice across diverse operating systems.

### Key Features
- **High Efficiency**: XLang™ runs 3 to 5 times faster than Python, particularly in AI and deep learning applications.
- **Optimized Tensor Computing**: The language includes a fully optimized tensor computing architecture, enabling effortless neural network construction through tensor expressions.
- **Performance Boost on GPU**: In CUDA-enabled GPU environments, XLang™ can enhance inference and training performance by 6 to 10 times, automating tensor data flow graph generation and target-specific compilation.

### Building XLang™

#### Windows:
1. Clone the repository: 
    ```bash
    git clone https://github.com/xlang-foundation/xlang.git
    ```
2. Open the XLang™ folder in Visual Studio.
3. Select your configuration (e.g., Local Machine/x64-Debug, WSL:Ubuntu/WSL-GCC-Debug).
4. Build via Visual Studio's build menu.

#### Linux (Ubuntu):
1. Install prerequisites:
    - UUID: `sudo apt-get install uuid-dev`
    - OpenSSL (for HTTP plugin): `sudo apt-get install libssl-dev`
    - Python3 (optional for Python library integration): 
      ```bash
      sudo apt-get install python3-dev
      pip install numpy
      ```
      *(To disable Python integration, comment out `add_subdirectory("PyEng")` in `CMakeLists.txt`.)*
2. Clone the repository: 
    ```bash
    git clone https://github.com/xlang-foundation/xlang.git
    ```
3. Navigate to the cloned directory: 
    ```bash
    cd xlang
    ```
4. Create and enter the build directory:
    ```bash
    mkdir out && cd out
    ```
5. Generate build files: 
    ```bash
    cmake ..
    ```
6. Compile: 
    ```bash
    make
    ```

#### macOS:
1. Install prerequisites:
    - UUID: `brew install ossp-uuid`
    - OpenSSL (for HTTP plugin): `brew install openssl`
    - Python3 (optional for Python library integration): 
      ```bash
      brew install python3
      pip install numpy
      ```
      *(To disable Python integration, comment out `add_subdirectory("PyEng")` in `CMakeLists.txt`.)*
2. Clone the repository: 
    ```bash
    git clone https://github.com/xlang-foundation/xlang.git
    ```
3. Navigate to the cloned directory:
    ```bash
    cd xlang
    ```
4. Create and enter the build directory:
    ```bash
    mkdir out && cd out
    ```
5. Generate build files:
    ```bash
    cmake ..
    ```
6. Compile:
    ```bash
    make
    ```

### Running XLang™

Navigate to the XLang™ executable folder and run the `xlang` command:

```bash
$ ./xlang
xlang [-dbg] [-enable_python|-python]
      [-run_as_backend|-backend] [-event_loop]
      [-c "code,use \n as line separator"]
      [-cli]
      [file parameters]
xlang -help | -? | -h for help
```

#### Example Commands

- **Running a Script File**:  
  To run an XLang™ script file:
  ```bash
  $ ./xlang my_script.x
  ```

- **Running Inline Code with Event Loop**:  
  To execute inline code:
  ```bash
  $ ./xlang -c "print('Hello, XLang!')"
  ```

- **Running in Command-Line Interface (CLI) Mode**:  
  To start in CLI mode without executing a file:
  ```bash
  $ ./xlang -cli
  ```
> Under the test folder, you'll find numerous XLang and Python code examples that can be used for testing. While some files may currently break, we are actively working on improving compatibility with Python syntax and its ecosystem.

#### Debugging in VS Code:
- Install the XLang™ plugin.
- Start XLang™ with `-event_loop -dbg -enable_python`.
- Open or create a `.x` file and begin debugging from the VS Code menu.
> **Note:** Debugging in VS Code has not been tested on Linux and macOS.

### Building for Android:
1. On Windows, install Android Studio.
2. Open the XLang™ project from the `xlang\Android` folder.
3. Build using Android Studio's Build menu.
