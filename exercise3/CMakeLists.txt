
file(GLOB_RECURSE SOURCES
    ./*.hpp
    ./*.cpp
    )

add_executable(exercise3 ${SOURCES})

target_link_libraries(exercise3 PRIVATE Microtech::Common)

target_include_directories(exercise3 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
