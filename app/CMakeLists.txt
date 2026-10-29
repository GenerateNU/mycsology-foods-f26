
cmake_minimum_required(VERSION 3.20)

# for global installs
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

# populated the $PROJECT_NAME var and this is what the output files are named 
project(mycology_foods)

# app is a predesignated target that zephyr uses
target_sources(app PRIVATE 
    src/main.c
    src/UI/seven_seg.c
    src/UI/buzzer.c
    src/UI/buttons.c
    src/UI/dial.c
    src/sensors/sht30.c
    )

target_include_directories(app PRIVATE 
    inc
    inc/UI
    )
