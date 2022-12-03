
file(GLOB_RECURSE SOURCES
    ./*.hpp
    ./*.cpp
    )

add_executable(exercise1  ${SOURCES})

target_link_libraries(exercise1 PRIVATE Microtech::Common)

target_include_directories(exercise1 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
