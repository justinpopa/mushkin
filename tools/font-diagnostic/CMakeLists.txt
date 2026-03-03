cmake_minimum_required(VERSION 3.20)
project(font-diagnostic LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Qt6 REQUIRED COMPONENTS Widgets)
qt_standard_project_setup()

add_executable(font-diagnostic main.cpp)

target_link_libraries(font-diagnostic PRIVATE Qt6::Widgets)

# Enable Qt's automatic MOC/RCC/UIC (required for QApplication)
set_target_properties(font-diagnostic PROPERTIES
    AUTOMOC ON
    AUTOUIC ON
    AUTORCC ON
)
