
file(GLOB_RECURSE SOURCES
    ./*.hpp
    ./*.cpp
    )

add_executable(exercise5 ${SOURCES})

target_link_libraries(exercise5 PRIVATE Microtech::Common)

target_include_directories(exercise5 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
