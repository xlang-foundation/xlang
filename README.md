# XLang™
XLang™ is a cutting-edge language designed for AI and IoT applications, offering exceptional dynamic and high-performance capabilities. It stands out with its innate ability for distributed computing. XLang™ excels in seamless integration with popular languages like C++, Python, and JavaScript, bridging the gap across various operating systems.

Performance-wise, XLang™ is notably efficient, running approximately 3 to 5 times faster than Python, especially in AI and deep learning contexts. It features a fully optimized tensor computing architecture, enabling users to effortlessly construct neural networks through tensor expressions. XLang™ automates the generation of tensor data flow graphs and compiles them for specific targets. Particularly in GPU environments utilizing CUDA, it can enhance inference and training performance by about 6 to 10 times.

**Building XLang™:**

- For Windows:
  - Clone the repository: `git clone https://github.com/xlang-foundation/xlang.git`
  - Open the XLang™ folder with Visual Studio.
  - Select a configuration (e.g., Local Machine/x64-Debug, WSL:Ubuntu/WSL-GCC-Debug).
  - Build using Visual Studio's build menu.

- For Linux (Ubuntu):
  - Install prerequisites:
    - UUID: `sudo apt-get install uuid-dev`
    - OpenSSL (for HTTP plugin): `sudo apt-get install libssl-dev`
    - Python3 (optional for Python library integration): `sudo apt-get install python3-dev` and `pip install numpy`. To disable, comment out `add_subdirectory("PyEng")` in `CMakeLists.txt`.
  - Building steps:
    1. Clone the repository: `git clone https://github.com/xlang-foundation/xlang.git`
    2. Navigate to the cloned directory: `cd xlang`
    3. Create and enter the build directory: `mkdir out && cd out`
    4. Generate build files: `cmake ..`
    5. Compile: `make`

**Running XLang™:**

- Navigate to the XLang™ executable folder and run the `xlang` command.
- For debugging in VS Code, install the XLang™ plugin and start XLang™ with `-event_loop -dbg -enable_python`. Open or create a `.x` file, and start debugging from the VS Code menu.

**Building for Android:**

- On Windows, install Android Studio.
- Open the XLang™ project from the `xlang\Android` folder and build using the Android Studio's Build menu.  
