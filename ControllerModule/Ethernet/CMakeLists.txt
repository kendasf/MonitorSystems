message(@${CMAKE_CURRENT_SOURCE_DIR})
set(ethernetHdrs ${ethernetHdrs}
    TcpInterface.h
    UdpInterface.h
)

set(ethernetSrc ${ethernetSrc}
    TcpInterface.cpp
    UdpInterface.cpp
)

add_library(dethernet STATIC ${ethernetSrc} ${ethernetHdrs})

# The following line is very practical:
# it will allow you to automatically add the correct include directories with "target_link_libraries"
target_include_directories(dethernet PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})