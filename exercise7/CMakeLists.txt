
file(GLOB_RECURSE SOURCES
    ./*.hpp
    ./*.cpp
    )

add_executable(exercise7 ${SOURCES})

target_link_libraries(exercise7 PRIVATE Microtech::Common)

target_include_directories(exercise7 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
