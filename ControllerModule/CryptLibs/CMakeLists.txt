message(@${CMAKE_CURRENT_SOURCE_DIR})
set(cryptLibsHdr ${cryptLibsHdr}
    aes.h
    bigdigits.h
    bigdtypes.h
    diffie-hellman.h
    md5.h
)

set(cryptLibsSrc ${cryptLibsSrc}
    aes.cpp
    bigdigits.cpp
    diffie-hellman.cpp
    md5.cpp
)

add_library(dcrypto STATIC ${cryptLibsSrc} ${cryptLibsHdr})

# The following line is very practical:
# it will allow you to automatically add the correct include directories with "target_link_libraries"
target_include_directories(dcrypto PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})