cmake_minimum_required(VERSION 3.20)

project(MinuanBrowser
    VERSION 1.1.0
    DESCRIPTION "Minuan - a SimVerse browser using the Sunay rendering pipeline"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

include(FetchContent)
include(CTest)
include(GNUInstallDirs)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Widgets WebEngineWidgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Widgets WebEngineWidgets)

FetchContent_Declare(
    Sunay
    GIT_REPOSITORY https://github.com/Minuan-org/Sunay.git
    GIT_TAG v1.0.1
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(Sunay)

set(MINUAN_SOURCES
    browser/src/main.cpp
    browser/src/browser_window.cpp
    browser/src/navigation_utils.cpp
    src/html6_engine.cpp
    src/dom_processor.cpp
    src/html6_validator.cpp
    src/utils.cpp
)

add_executable(MinuanBrowser ${MINUAN_SOURCES})
target_include_directories(MinuanBrowser PRIVATE
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/browser/include
)
target_link_libraries(MinuanBrowser PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::WebEngineWidgets
    Sunay::Sunay
)

set_target_properties(MinuanBrowser PROPERTIES
    OUTPUT_NAME "Minuan"
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${PROJECT_BINARY_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${PROJECT_BINARY_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${PROJECT_BINARY_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${PROJECT_BINARY_DIR}/bin"
)

if(MSVC)
    target_compile_options(MinuanBrowser PRIVATE /W4 /permissive-)
else()
    target_compile_options(MinuanBrowser PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_custom_command(TARGET MinuanBrowser POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${PROJECT_SOURCE_DIR}/browser/websites.json"
        "$<TARGET_FILE_DIR:MinuanBrowser>/websites.json"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${PROJECT_SOURCE_DIR}/browser/home.html"
        "$<TARGET_FILE_DIR:MinuanBrowser>/home.html"
    VERBATIM
)

if(BUILD_TESTING)
    add_executable(MinuanNavigationTests
        tests/navigation_utils_test.cpp
        browser/src/navigation_utils.cpp
    )
    target_include_directories(MinuanNavigationTests PRIVATE
        ${PROJECT_SOURCE_DIR}/browser/include
    )
    target_link_libraries(MinuanNavigationTests PRIVATE
        Qt${QT_VERSION_MAJOR}::Core
    )
    add_test(NAME MinuanNavigation COMMAND MinuanNavigationTests)

    add_executable(MinuanHtml6Tests
        tests/html6_pipeline_test.cpp
        src/html6_engine.cpp
        src/dom_processor.cpp
        src/html6_validator.cpp
        src/utils.cpp
    )
    target_include_directories(MinuanHtml6Tests PRIVATE ${PROJECT_SOURCE_DIR}/include)
    if(MSVC)
        target_compile_options(MinuanHtml6Tests PRIVATE /W4 /permissive-)
    else()
        target_compile_options(MinuanHtml6Tests PRIVATE -Wall -Wextra -Wpedantic)
    endif()
    add_test(NAME MinuanHtml6Pipeline COMMAND MinuanHtml6Tests)
endif()

install(TARGETS MinuanBrowser RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(
    FILES
        "${PROJECT_SOURCE_DIR}/browser/websites.json"
        "${PROJECT_SOURCE_DIR}/browser/home.html"
    DESTINATION ${CMAKE_INSTALL_BINDIR}
)
