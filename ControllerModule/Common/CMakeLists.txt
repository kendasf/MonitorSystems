
message(@${CMAKE_CURRENT_SOURCE_DIR})

set(commonHdrs ${commonHdrs}
    ./inc/adc.h
    ./inc/pinctl.h
    ./inc/pwm.h
    ./inc/spi.h
    ./inc/timer.h
    ./inc/uart.h
)

set(commonSrc ${commonSrc}
    ./src/adc.cpp
    ./src/pinctl.cpp
    ./src/pwm.cpp
    ./src/spi.cpp
    ./src/timer.cpp
    ./src/uart.cpp
)

add_library(dcommon STATIC ${commonSrc} ${commonHdrs})


# The following line is very practical:
# it will allow you to automatically add the correct include directories with "target_link_libraries"
target_include_directories(dcommon PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/inc)

