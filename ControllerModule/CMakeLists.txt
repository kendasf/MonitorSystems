cmake_minimum_required(VERSION 3.07)
set(CMAKE_VERBOSE_MAKEFILE ON)

macro(print_all_variables)
    message(STATUS "print_all_variables------------------------------------------{")
    get_cmake_property(_variableNames VARIABLES)
    foreach (_variableName ${_variableNames})
        message(STATUS "${_variableName}=${${_variableName}}")
    endforeach()
    message(STATUS "print_all_variables------------------------------------------}")
endmacro()


set(projectName vms)

project(${projectName} VERSION 1.0)

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_options(-Werror=unused-function)

add_subdirectory(Common)
add_subdirectory(CryptLibs)
add_subdirectory(Ethernet)
add_subdirectory(json)

#Additional Headers
set(gnode ${CMAKE_SOURCE_DIR}/gnode/)
set(http ${CMAKE_SOURCE_DIR}/http/)
set(appInc ${CMAKE_SOURCE_DIR}/include)

set(sysInc ${sysInc}
    #/usr/arm-geolux-linux-gnueabi/include
    #/usr/arm-geolux-linux-gnueabi/include/c++/8/
    /usr/arm-linux-gnueabihf/include
    /usr/arm-linux-gnueabihf/include/c++/8/
    /usr/include
    /usr/local/include
)

set(appSource ${appSource}
    ${CMAKE_SOURCE_DIR}/src/camera.cpp
    ${CMAKE_SOURCE_DIR}/src/cameraFuncts.cpp
    ${CMAKE_SOURCE_DIR}/src/CommProt.cpp
    ${CMAKE_SOURCE_DIR}/src/FileRoutines.cpp
    ${CMAKE_SOURCE_DIR}/src/FlashDriveTask.cpp
    ${CMAKE_SOURCE_DIR}/src/http.cpp
    ${CMAKE_SOURCE_DIR}/src/main.cpp
    ${CMAKE_SOURCE_DIR}/src/radar.cpp
    ${CMAKE_SOURCE_DIR}/src/SystemUtilities.cpp
    ${CMAKE_SOURCE_DIR}/src/VMSDriver.cpp
    ${CMAKE_SOURCE_DIR}/src/webservices.cpp
)

set(Boost_USE_STATIC_LIBS ON) 
set(Boost_USE_MULTITHREADED ON)  
set(Boost_USE_STATIC_RUNTIME ON) 
set(BOOST_ROOT /home/saer/Downloads/boost_1_56_0/)
find_package(Boost 1.56.0 COMPONENTS system regex REQUIRED)

set(allIncludes ${allIncludes} ${sysInc} ${gnode} ${http} ${appInc} )
set(SOURCES ${SOURCES} ${appSource} )

#print_all_variables()
 
add_executable(${projectName} ${SOURCES} ${allIncludes})
target_include_directories(${projectName} PRIVATE ${allIncludes})
target_link_libraries(${projectName} dcommon dcrypto dethernet djson ${Boost_LIBRARIES} rt)

# Auto copy exe after cean build
add_custom_command( TARGET ${projectName} 
                     POST_BUILD 
                     COMMAND rsync -avzhPe ssh ${CMAKE_BINARY_DIR}/vms debian@10.8.0.2:/home/debian/testApp/vms
                     )

# Auto copy exe after cean build
#add_custom_command( TARGET ${projectName} 
#                     POST_BUILD 
#                     COMMAND scp ${CMAKE_BINARY_DIR}/vms debian@10.0.2.18:/home/debian/testApp/
#                     )