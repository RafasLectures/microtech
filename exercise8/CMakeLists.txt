
file(GLOB_RECURSE SOURCES
    ./*.hpp
    ./*.cpp
    )

add_executable(exercise8 ${SOURCES})

target_link_libraries(exercise8 PRIVATE Microtech::Common)

target_include_directories(exercise8 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
