message(@${CMAKE_CURRENT_SOURCE_DIR})
set(jsonHdrs ${jsonHdrs}
    json-forwards.h
    json.h
)

set(jsonSrc ${jsonSrc}
    jsoncpp.cpp
)

add_library(djson STATIC ${jsonSrc} ${jsonHdrs})

# The following line is very practical:
# it will allow you to automatically add the correct include directories with "target_link_libraries"
target_include_directories(djson PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})