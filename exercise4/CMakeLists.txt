
file(GLOB_RECURSE SOURCES
    ./*.hpp
    ./*.cpp
    )

add_executable(exercise4 ${SOURCES})

target_link_libraries(exercise4 PRIVATE Microtech::Common)

target_include_directories(exercise4 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
