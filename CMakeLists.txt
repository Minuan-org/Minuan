cmake_minimum_required(VERSION 3.15)

project(MinuanBrowser
    VERSION 1.0
    DESCRIPTION "Minuan - C++ Browser for SimVerse"
    LANGUAGES CXX
)

# -----------------------------
# C++ Standard
# -----------------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# -----------------------------
# Source Files
# Automatically find all .cpp files
# -----------------------------
file(GLOB_RECURSE MINUAN_SOURCES
    "browser/*.cpp"
    "html6/*.cpp"
    "cssnext/*.cpp"
    "js2/*.cpp"
    "engine/*.cpp"
    "ui/*.cpp"
)

# -----------------------------
# Include Directories
# -----------------------------
include_directories(
    ${PROJECT_SOURCE_DIR}/browser
    ${PROJECT_SOURCE_DIR}/html6
    ${PROJECT_SOURCE_DIR}/cssnext
    ${PROJECT_SOURCE_DIR}/js2
    ${PROJECT_SOURCE_DIR}/engine
    ${PROJECT_SOURCE_DIR}/ui
)

# -----------------------------
# Create Executable
# -----------------------------
add_executable(minuan ${MINUAN_SOURCES})

# -----------------------------
# Platform‑Specific Settings
# -----------------------------

# Windows: WebView2
if (WIN32)
    message(STATUS "Configuring for Windows (WebView2)")
    target_link_libraries(minuan PRIVATE WebView2Loader)
    target_compile_definitions(minuan PRIVATE MINUAN_WINDOWS)
endif()

# Linux: GTK or WebKitGTK (future)
if (UNIX AND NOT APPLE)
    message(STATUS "Configuring for Linux")
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GTK3 REQUIRED gtk+-3.0)
    include_directories(${GTK3_INCLUDE_DIRS})
    link_directories(${GTK3_LIBRARY_DIRS})
    target_link_libraries(minuan PRIVATE ${GTK3_LIBRARIES})
    target_compile_definitions(minuan PRIVATE MINUAN_LINUX)
endif()

# macOS: Cocoa/WebKit
if (APPLE)
    message(STATUS "Configuring for macOS")
    find_library(COCOA_LIBRARY Cocoa REQUIRED)
    find_library(WEBKIT_LIBRARY WebKit REQUIRED)
    target_link_libraries(minuan PRIVATE ${COCOA_LIBRARY} ${WEBKIT_LIBRARY})
    target_compile_definitions(minuan PRIVATE MINUAN_MACOS)
endif()

# -----------------------------
# SQLite (for history, bookmarks)
# -----------------------------
find_package(SQLite3 REQUIRED)
target_include_directories(minuan PRIVATE ${SQLite3_INCLUDE_DIRS})
target_link_libraries(minuan PRIVATE SQLite::SQLite3)

# -----------------------------
# Resources (icons, HTML6 test pages)
# -----------------------------
file(COPY resources DESTINATION ${CMAKE_BINARY_DIR})

# -----------------------------
# Install Rules
# -----------------------------
install(TARGETS minuan
    RUNTIME DESTINATION bin
)

install(DIRECTORY resources/
    DESTINATION share/minuan/resources
)
