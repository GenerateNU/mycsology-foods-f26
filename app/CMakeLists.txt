
cmake_minimum_required(VERSION 3.20)

# for global installs
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

# populated the $PROJECT_NAME var and this is what the output files are named 
project(firepod)

# app is a predesignated target that zephyr uses
target_sources(app PRIVATE src/main.c)


#[[
Example from my capstone project:
cmake_minimum_required(VERSION 3.22)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(firepod)

# Include dirs so the Bosch API header is visible
target_include_directories(app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/bosch/BME68x_SensorAPI
)

# All source files, including the Bosch driver and your stubs
target_sources(app PRIVATE
    src/main.c
    src/sensor_thread.c
    src/lora_thread.c
    src/base_station_thread.c
    src/bme688_stubs.c
    src/bosch/BME68x_SensorAPI/bme68x.c
)
]]
