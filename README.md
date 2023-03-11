# xlang™
* A new dynamic programing language for **AI and IOT** with natural born **distributed computing ability**    
* A super glue to easily integrating with other languages such as c++/c, python and javascript and any framework cross operation system barriers.  
* Running faster than python about 3x-5x  

# for AI/Deep learning
- fully optimized tensor computing architecture 
- easily build neural network with tensor expression
- automatically generate tensor data flow graph and compile for target  
- boost inference/training performance about 6x-10x in GPU(CUDA)  

# How to Build  
- build from Windows  
1. git clone https://github.com/xlang-foundation/xlang.git  
2. use Visual Studio to open this xlang folder  
3. choose configuration for example Local Machine/x64-Debug, WSL:Ubuntu/WSL-GCC-Debug  
4. build ( click on Visual Studio's menu: build/build all) 

- build from Linux(Ubuntu)  
    ## Prerequisites  
        sudo apt-get install uuid-dev  
    ## for openssl required by http plugin
        sudo apt-get install libssl-dev
    ## if want to enable xlang to call python libs directly  
        sudo apt-get install python3-dev
        and also need to pip install numpy
        if not want to enable this feature, 
        just comment out line below in CMakeLists.txt in root folder
        add_subdirectory("PyEng")
    ## Steps to build
        1. git clone https://github.com/xlang-foundation/xlang.git
        2. cd xlang
        3. mkdir out
        4. cd out
        5. cmake ..
        6. make


# How to Run
- go to console window, cd to xlang executable file folder
- xlang

# How to use vscode to debug on xlang code  
1. install xlang plugin in vscode  
2. start xlang with parameter: -event_loop -dbg -enable_python  
    xlang -event_loop -dbg -enable_python
3. open or new a file with .x extension name  
    click on vs code menu run/start debuging then vscode will automatcilly connect with xlang to run  

# How to build for Android
1. in Windows, install Android Studio
2. open project from folder xlang\Android
3. then build from Android Stduio menu: Build/Make Project
