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

#target_link_libraries(
#    ${PROJECT}
#    -static
#)

#target_compile_options(
#    ${PROJECT}
#    PRIVATE
#    -Wno-unused-result
#)

#set_target_properties(
#    ${PROJECT}
#    PROPERTIES
#    C_STANDARD 11
#    C_STANDARD_REQUIRED YES
#    C_EXTENSIONS YES
#    LINK_FLAGS_RELEASE "-O3 -s"
#    LINK_FLAGS_DEBUG "-g"
#    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../bin"
#)

target_include_directories(
    ${PROJECT}
    PRIVATE
    "/usr/local/boost_1_74_0/include"
)

# threads
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

target_link_libraries(
    ${PROJECT}
    Threads::Threads
)
