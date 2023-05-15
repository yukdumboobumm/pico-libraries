##############

add_library(dc_motor dc_motor.c)

#add_executable(dc_motor)

# Create C header file with the name <pio program>.pio.h
pico_generate_pio_header(dc_motor ${CMAKE_CURRENT_LIST_DIR}/dc_motor.pio)

target_include_directories(dc_motor PUBLIC ${CMAKE_CURRENT_LIST_DIR})

# Link to pico_stdlib (gpio, time, etc. functions)
target_link_libraries(
    dc_motor 
    pico_stdlib
    hardware_pio
)