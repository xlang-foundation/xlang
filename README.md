
---

# **XLang™**

**XLang™** is a **high-performance glue language** designed to seamlessly integrate system components and application subsystems. It also serves as an excellent **embedding language** that can be easily integrated with application and game systems, thanks to its **lightweight stack**, **complete syntax**, and **robust runtime** for application integration. Moreover, **XLang™** features a syntax that is similar to **Python**, making it familiar and easy to adopt for developers.

Crafted specifically for **AI** and **IoT** applications, **XLang™** delivers dynamic, high-performance computing by leveraging advanced features such as **optimized tensor expressions** with partial optimization. This approach enables efficient neural network construction and data-intensive computing, making **XLang™** an ideal choice for **distributed environments** and **deep learning tasks**.

## **Key Features**

- **High Efficiency:**  
  **XLang™** exhibits a significant performance advantage over **Python**, delivering much faster execution times in **AI** and **deep learning** applications.
- **High-Performance Glue Capabilities:**  
  Seamlessly integrate system components and application subsystems with **fast method calls** (efficient inter-process communication even with large data payloads), a **robust event and notification system** for handling high-volume data exchanges, and **enhanced performance** through direct **AST execution** and **parallel expression-based data flows**.
- **Effortless API Exposure:**  
  Expose APIs within developed systems or applications without the complexity of traditional methods (e.g., **Python C extensions**). **XLang™** allows direct, in-site API exposure for rapid integration.
- **Native Thread Safety and Asynchronous Programming:**  
  Designed from the ground up to be **thread safe**, **XLang™** includes built-in support for **threads**, **asynchronous operations**, and natural **task management**, making it simple to write concurrent code and handle large-scale data exchanges.
- **Optimized Tensor Computing:**  
  A fully optimized **tensor computing architecture** enables effortless neural network construction via **tensor expressions**.
- **GPU Performance Boost:**  
  In **CUDA-enabled GPU** environments, inference and training performance can be enhanced by 6 to 10 times thanks to automated **tensor data flow graph generation** and **target-specific compilation**.

## **Contributing**

-Zack Gaylord: Using github for Systems Engineering class in college. Have used it in the past for html web design work. 

If you're interested in contributing to the **XLang™** project, we would love to hear from you. Whether you're a developer, tester, or simply passionate about advancing this technology, please reach out. For more information or to get involved, send us an email at [**info@xlangfoundation.org**](mailto:info@xlangfoundation.org) and we'll provide the necessary details.

**XLang™** has been thoroughly tested on **Linux** and **Windows** platforms—including successful deployments on **Raspberry Pi** boards and the **Raspberry Pi Pico**. If you need specific build instructions or have any questions regarding the **Raspberry Pi Pico** setup, please contact us at the same email address. We're here to support you and ensure your **XLang™** experience is smooth and rewarding.

## **Building XLang™**

For **optimal performance**, please ensure that **XLang™** is built in **Release mode**. You can do this by running:

```bash
cmake -DCMAKE_BUILD_TYPE=Release ..
```

### **Windows**

1. Clone the repository:
    ```bash
    git clone https://github.com/xlang-foundation/xlang.git
    ```
2. Open the **XLang™** folder in **Visual Studio**.
3. Select your configuration (e.g., **Local Machine/x64-Debug**, **WSL:Ubuntu/WSL-GCC-Debug**).
4. Build via Visual Studio's build menu.

### **Linux (Ubuntu)**

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

### **macOS**

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

*You can also use **Xcode** to open the **XLang™** folder for compilation.*

### **Building for Android**

1. On **Windows**, install **Android Studio**.
2. Open the **XLang™** project from the `xlang\Android` folder.
3. Build using **Android Studio's Build menu**.

## **Running XLang™**

Navigate to the **XLang™** executable folder and run the `xlang` command:

```bash
$ ./xlang
xlang [-dbg] [-enable_python|-python]
      [-run_as_backend|-backend] [-event_loop]
      [-c "code,use \n as line separator"]
      [-cli]
      [file parameters]
xlang -help | -? | -h for help
```

### **Example Commands**

- **Running a Script File:**  
  To run an **XLang™** script file:
  ```bash
  $ ./xlang your_script.x
  ```

- **Running Inline Code with Event Loop:**  
  To execute inline code:
  ```bash
  $ ./xlang -c "print('Hello, XLang!')"
  ```

- **Running in Command-Line Interface (CLI) Mode:**  
  To start in **CLI** mode without executing a file:
  ```bash
  $ ./xlang -cli
  ```

Under the `test` folder, you'll find numerous **XLang** and **Python** code examples for testing. While some files may currently break, we are actively working to improve compatibility with **Python syntax** and its ecosystem.

## **Debugging in VS Code**

- Install the **XLang™ plugin**.
- Start **XLang™** with:
  ```bash
  -event_loop -dbg -enable_python
  ```
- You can launch a new **XLang™** instance or attach to an existing application/process that uses **XLang™ embedding**.
- Open or create a `.x` file and begin debugging from the **VS Code** menu.

> **Note:** Debugging in **VS Code** has not been tested on **macOS**.

---

Happy coding with **XLang™**!  
For any questions or support, please reach out at [**info@xlangfoundation.org**](mailto:info@xlangfoundation.org).
