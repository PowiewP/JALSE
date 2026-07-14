cmake_minimum_required(VERSION 3.20)
project(JALSE CXX)

set(CMAKE_CXX_STANDARD 17)          # adjust if you're using 20
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(JALSE
        HeliosDac.cpp
        HeliosDac.h
        LsObject.cpp
        LsObject.hpp
        classes.hpp
        globals.hpp
        idn/idn-hello.h
        idn/idn-stream.h
        idn/idn.cpp
        idn/idn.h
        idn/idnServerList.cpp
        idn/idnServerList.h
        idn/plt-windows.cpp
        idn/plt-windows.h
        libusb.h
        main.cpp
        utility.hpp
        UI.hpp
)

# --- Compiler flags (-Wall always, -O2 for Release) ---
target_compile_options(JALSE PRIVATE
        -Wall
        $<$<CONFIG:Release>:-O2>
)

# --- Include dirs (headers, SFML, project root) ---
target_include_directories(JALSE PRIVATE
        "C:/msys64/ucrt64/include"
        "E:/domowe_rzeczy/SFML/include"
        "D:/domowe_rzeczy/SFML/include"
        "${CMAKE_SOURCE_DIR}"
)

# --- Lib search dirs ---
target_link_directories(JALSE PRIVATE
        "C:/msys64/ucrt64/lib"
        "E:/domowe_rzeczy/SFML/lib"
        "D:/domowe_rzeczy/SFML/lib"
)

# --- Linker flags ---
target_link_options(JALSE PRIVATE
        -static-libstdc++
        -static-libgcc
        $<$<CONFIG:Release>:-s>
)

# --- Libraries ---
target_link_libraries(JALSE PRIVATE
        sfml-graphics
        sfml-window
        sfml-system
        sfml-audio
        setupapi
        usb-1.0
        ws2_32
        winmm
        iphlpapi
)

set_target_properties(JALSE PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/Release"
)