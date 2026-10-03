cmake_minimum_required(VERSION 3.16)

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
# Qt Core Settings
# -----------------------------
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# -----------------------------
# Find Qt Packages
# -----------------------------
find_package(Qt6 COMPONENTS Widgets WebEngineWidgets QUIET)
if (NOT Qt6_FOUND)
    find_package(Qt5 COMPONENTS Widgets WebEngineWidgets REQUIRED)
endif()

# -----------------------------
# Source Files
# -----------------------------
set(MINUAN_SOURCES
    src/main.cpp
    src/html6_engine.cpp
    src/dom_processor.cpp
    src/html6_validator.cpp
    src/utils.cpp
)

# -----------------------------
# Create Executable
# -----------------------------
add_executable(MinuanBrowser ${MINUAN_SOURCES})

# -----------------------------
# Include Directories
# -----------------------------
target_include_directories(MinuanBrowser PRIVATE
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/src
)

# -----------------------------
# Link Qt Libraries
# -----------------------------
if (Qt6_FOUND)
    target_link_libraries(MinuanBrowser PRIVATE
        Qt6::Widgets
        Qt6::WebEngineWidgets
    )
else()
    target_link_libraries(MinuanBrowser PRIVATE
        Qt5::Widgets
        Qt5::WebEngineWidgets
    )
endif()

# -----------------------------
# Post-Build Assets
# -----------------------------
if (EXISTS "${CMAKE_SOURCE_DIR}/websites.json")
    add_custom_command(TARGET MinuanBrowser POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/websites.json $<TARGET_FILE_DIR:MinuanBrowser>
    )
endif()
