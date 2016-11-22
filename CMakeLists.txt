
if(WIN32)
    set(
        CMAKE_USER_MAKE_RULES_OVERRIDE
        "${CMAKE_CURRENT_SOURCE_DIR}/msvc_override.cmake"
    )
endif(WIN32)

cmake_minimum_required(VERSION 2.8)

project(test-udp)

set(Boost_USE_STATIC_LIBS     ON)
set(Boost_USE_MULTITHREADED   ON)
set(Boost_USE_STATIC_RUNTIME  ON)


if(MSVC) 
    find_package( Boost 1.53 COMPONENTS
                    system
                    program_options
                    filesystem
                    regex # I have no idea why it has to be here on Windows
                  REQUIRED)
else( )
    find_package( Boost 1.53 COMPONENTS
                    system
                    program_options
                    filesystem
                  REQUIRED)
endif( )

include_directories( ${CMAKE_CURRENT_SOURCE_DIR} )

if(NOT WIN32)
    set(CMAKE_CXX_FLAGS
        "${CMAKE_CXX_FLAGS} -Wextra -Wall -Wno-long-long -pedantic -std=c++11")
endif( )


include_directories(    ${Boost_INCLUDE_DIRS}  )

add_executable( udp-server server.cpp)
add_executable( udp-client client.cpp)

target_link_libraries(  udp-server ${Boost_LIBRARIES} )
target_link_libraries(  udp-server "-lpthread" )
target_link_libraries(  udp-client ${Boost_LIBRARIES} )

