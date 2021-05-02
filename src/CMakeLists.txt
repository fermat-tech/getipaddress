cmake_minimum_required( VERSION 3.16.3 )

set ( PROJECT getipaddress )

project( ${PROJECT} )

add_executable( ${PROJECT} main.cpp )

set_target_properties(
    ${PROJECT}
    PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS YES
    LINK_FLAGS_RELEASE "-O3 -s"
    LINK_FLAGS_DEBUG "-g"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../bin"
)

find_package(Boost 1.74.0 REQUIRED)

target_include_directories(
    ${PROJECT}
    PRIVATE
    ${Boost_INCLUDE_DIRS}
)

# threads
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

execute_process(COMMAND "uname" "-o" OUTPUT_VARIABLE uname_o )

if( "${uname_o}" MATCHES "[Mm][Ss][Yy][Ss]" )
    target_link_libraries(
        ${PROJECT}
        Threads::Threads
        ws2_32
        )
elseif( "${uname_o}" MATCHES "([Uu][Nn][Ii][Xx])|([Ii][Nn][Uu][Xx])" )
    target_link_libraries(
        ${PROJECT}
        Threads::Threads
    )
else()
    message( FATAL_ERROR "Unsupported OS" )
endif()

