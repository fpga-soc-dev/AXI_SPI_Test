cmake_minimum_required(VERSION 3.20)

project(oled-spi-test 
	VERSION 1.0.0
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
find_package(spdlog REQUIRED)
find_package(fmt REQUIRED)
pkg_check_modules(GPIODCXX REQUIRED IMPORTED_TARGET libgpiodcxx>=2.0)

set(SOURCES
    Utilities/FileHandle.cpp
    SPIInterface/SPIDevice.cpp
    Gpio/Gpio.cpp
    OLEDDriver.cpp
    main.cpp
)

add_executable(${PROJECT_NAME}
    ${SOURCES}
)

target_include_directories(${PROJECT_NAME}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        Utilities/
        SPIInterface/
        Gpio/
)

target_link_libraries(${PROJECT_NAME} 
    PRIVATE 
    PkgConfig::GPIODCXX
    fmt::fmt
    spdlog::spdlog_header_only
)

add_definitions(-DSPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_DEBUG)
target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic)


