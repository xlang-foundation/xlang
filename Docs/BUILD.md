# Building XLang™

For **optimal performance**, please ensure that XLang is built in **Release mode**.

```bash
cmake -DCMAKE_BUILD_TYPE=Release ..
```

## Windows

1. Clone the repository:
    ```bash
    git clone https://github.com/xlang-foundation/xlang.git
    ```
2. Open the XLang folder in **Visual Studio**.
3. Select your configuration (e.g., **Local Machine/x64-Debug**, **WSL:Ubuntu/WSL-GCC-Debug**).
4. Build via Visual Studio's build menu.

## Linux (Ubuntu)

1. Install prerequisites:
    - **UUID (required):**
      ```bash
      sudo apt-get install uuid-dev
      ```
    - **OpenSSL (for HTTP plugin):**
      ```bash
      sudo apt-get install libssl-dev
      sudo apt-get install libcurl4-openssl-dev
      ```
    - **Python3 (optional for Python library integration):**
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

## macOS

1. Install prerequisites:
    - **UUID (required):**
      ```bash
      brew install ossp-uuid
      ```
    - **OpenSSL (for HTTP plugin):**
      ```bash
      brew install openssl
      brew install curl
      ```
    - **Turbo-jpeg (optional for image module):**
      ```bash
      brew install jpeg-turbo
      ```
    - **Python3 (optional for Python library integration):**
      ```bash
      brew install python3
      pip install numpy
      ```
      *(To disable Python integration, comment out `add_subdirectory("PyEng")` in `CMakeLists.txt`.)*
2. Clone and build as usual (use `mkdir out`, `cmake ..`, and `make`).

*You can also use **Xcode** to open the XLang folder for compilation.*

## Building for Android

1. On **Windows**, install **Android Studio**.
2. Open the XLang project from the `xlang/Android` folder.
3. Build using **Android Studio's Build menu**.
