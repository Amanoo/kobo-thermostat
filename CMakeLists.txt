cmake_minimum_required(VERSION 3.16)
project(ToonDashboard LANGUAGES CXX)


set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC OFF)
set(CMAKE_AUTOUIC OFF)


find_package(Qt5 5.15 REQUIRED COMPONENTS
Widgets
Mqtt
)


add_executable(${PROJECT_NAME}
src/main.cpp
src/backend.cpp
src/mainwindow.cpp
src/usagebar.cpp
src/powergraph.cpp
src/thermostatdial.cpp
)


target_include_directories(${PROJECT_NAME} PRIVATE src)


target_link_libraries(${PROJECT_NAME} PRIVATE
Qt5::Widgets
Qt5::Mqtt
)


install(TARGETS ${PROJECT_NAME}
RUNTIME DESTINATION bin
)