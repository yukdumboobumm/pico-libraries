##############

add_library(pulseCounter pulseCounter.c)

#add_executable(pulseCounter)

# Create C header file with the name <pio program>.pio.h
pico_generate_pio_header(pulseCounter ${CMAKE_CURRENT_LIST_DIR}/pulseCounter.pio)

target_include_directories(pulseCounter PUBLIC ${CMAKE_CURRENT_LIST_DIR})

# Link to pico_stdlib (gpio, time, etc. functions)
target_link_libraries(
    pulseCounter
    pico_stdlib
    hardware_pio
    hardware_clocks
)