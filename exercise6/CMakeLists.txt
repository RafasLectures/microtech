
file(GLOB_RECURSE SOURCES
    ./*.hpp
    ./*.cpp
    )

add_executable(exercise6 ${SOURCES})

target_link_libraries(exercise6 PRIVATE Microtech::Common)

target_include_directories(exercise6 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
