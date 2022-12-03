
file(GLOB_RECURSE SOURCES
    ./*.hpp
    ./*.cpp
    )

add_library(microtechcommon STATIC ${SOURCES})
add_library(Microtech::Common ALIAS microtechcommon)

target_include_directories(microtechcommon INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
if(MSP_COMPILER)
  target_link_libraries(microtechcommon
      INTERFACE -llnk_msp430g2553.cmd
      INTERFACE -llibc.a
      )
else()
  target_link_libraries(microtechcommon PUBLIC msp430Mock)
endif()

