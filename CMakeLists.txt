cmake_minimum_required(VERSION 3.15)

project(browser
    VERSION 1.0
    DESCRIPTION "Minuan - C++ Browser for SimVerse"
    LANGUAGES CXX
)

# Specify the generator and platform
set(CMAKE_GENERATOR "Visual Studio 16 2019 Win64")

# -----------------------------
# C++ Standard
# -----------------------------
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# -----------------------------
# Include directories
# -----------------------------
include_directories(
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/browser
    ${PROJECT_SOURCE_DIR}/src
)

# -----------------------------
# Source files
# -----------------------------
file(GLOB_RECURSE MINUAN_SOURCES
    "browser/*.cpp"
    "browser/BrowserWindow.cpp"
    "src/*.cpp"
)

# -----------------------------
# Create executable
# -----------------------------
add_executable(MinuanBrowser ${MINUAN_SOURCES})
add_custom_command(TARGET MinuanBrowser POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${CMAKE_SOURCE_DIR}/websites.json $<TARGET_FILE_DIR:MinuanBrowser>)


# -----------------------------
# Windows: WebView2 support
# -----------------------------
if (WIN32)
    target_link_libraries(MinuanBrowser PRIVATE WebView2Loader)
    target_compile_definitions(MinuanBrowser PRIVATE MINUAN_WINDOWS)
endif()

# -----------------------------
# Linux: GTK3 support
# -----------------------------
if (UNIX AND NOT APPLE)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GTK3 REQUIRED gtk+-3.0)
    include_directories(${GTK3_INCLUDE_DIRS})
    link_directories(${GTK3_LIBRARY_DIRS})
    target_link_libraries(MinuanBrowser PRIVATE ${GTK3_LIBRARIES})
    target_compile_definitions(MinuanBrowser PRIVATE MINUAN_LINUX)
endif()

# -----------------------------
# macOS: Cocoa/WebKit support
# -----------------------------
if (APPLE)
    find_library(COCOA_LIBRARY Cocoa REQUIRED)
    find_library(WEBKIT_LIBRARY WebKit REQUIRED)
    target_link_libraries(MinuanBrowser PRIVATE ${COCOA_LIBRARY} ${WEBKIT_LIBRARY})
    target_compile_definitions(MinuanBrowser PRIVATE MINUAN_MACOS)
endif()