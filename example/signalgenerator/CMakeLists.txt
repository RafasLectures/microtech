
file(GLOB_RECURSE SOURCES
    ./*.hpp
    ./*.cpp
    )
#-DPython3_EXECUTABLE=/usr/local/bin/python3
#set(Python3_EXECUTABLE /usr/local/bin/python3)

FetchContent_Declare(
    matplotlib
    GIT_REPOSITORY https://github.com/lava/matplotlib-cpp.git
    GIT_TAG master
)
FetchContent_MakeAvailable(matplotlib)

add_executable(signalgeneratorExample ${SOURCES})

target_link_libraries(signalgeneratorExample PRIVATE Microtech::Common
    PRIVATE matplotlib_cpp)

target_include_directories(signalgeneratorExample PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} PRIVATE ${matplotlib_cpp_SOURCE_DIR})
