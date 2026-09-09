cmake_minimum_required(VERSION 3.15)
project(ImageViewer C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Add source files
add_executable(image-viewer
    src/main.c
    src/app_state.c
    src/viewer_core.c
    src/viewer_ui.c
)

# Include directories
target_include_directories(image-viewer PRIVATE src)

# Compiler flags for MSVC
if(MSVC)
    # target_compile_options(image-viewer PRIVATE /W4 /Zi /Od)
    target_link_options(image-viewer PRIVATE /DEBUG /SUBSYSTEM:WINDOWS)
endif()

# Link libraries
target_link_libraries(image-viewer PRIVATE
    user32
    gdi32
    shell32
    shlwapi
    windowscodecs
    ole32
    comdlg32
)

# Set output directory for executable
set_target_properties(image-viewer PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
