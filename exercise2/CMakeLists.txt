
file(GLOB_RECURSE SOURCES
    ./*.hpp
    ./*.cpp
    )

add_executable(exercise2 ${SOURCES})

target_link_libraries(exercise2 PRIVATE Microtech::Common)

target_include_directories(exercise2 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
