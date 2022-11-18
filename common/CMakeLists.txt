
file(GLOB_RECURSE SOURCES
    ./*.hpp
    ./*.cpp
    )

add_library(microtechcommon INTERFACE)
add_library(Microtech::Common ALIAS microtechcommon)

target_include_directories(microtechcommon INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(microtechcommon
    INTERFACE -llnk_msp430g2553.cmd
    INTERFACE -llibc.a
    )
