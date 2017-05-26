cmake_minimum_required(VERSION 2.8)

set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
set(CMAKE_MODULE_PATH  ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/../../../cmake/")

set(SRCS )

macro (add_sources)
  file (RELATIVE_PATH _relPath "${PROJECT_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}")
  foreach (_src ${ARGN})
    if (_relPath)
      list (APPEND SRCS "${_relPath}/${_src}")
    else()
      list (APPEND SRCS "${_src}")
    endif()
  endforeach()
  if (_relPath)
    # propagate SRCS to parent directory
    set (SRCS ${SRCS} PARENT_SCOPE)
  endif()
endmacro()

project(AlpideTest)
include(CTest)

message(${CMAKE_MODULE_PATH})

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

#find_package (Boost COMPONENTS random unit_test_framework REQUIRED)
find_package (Boost REQUIRED)

FIND_PACKAGE(SystemC MODULE REQUIRED)

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  set (CMAKE_CXX_FLAGS "--std=c++11 ${CMAKE_CXX_FLAGS}")
endif ()


# Set modules used
set(MODULES
  Alpide
  )


add_subdirectory("../../../../" "${CMAKE_CURRENT_BINARY_DIR}/alpide")


include_directories (../../../../source/bench
  ${SystemC_INCLUDE_DIRS}
  ${Boost_INCLUDE_DIRS}
  ../../src/
  )


add_sources(
  alpide_test.cpp
  ../event/EventFrame.cpp
  ../event/Hit.cpp
  ../AlpideDataParser/AlpideDataParser.cpp
  )



add_executable (alpide_test EXCLUDE_FROM_ALL ${SRCS})
target_link_libraries(alpide_test ${SystemC_LIBRARIES} pthread boost_random)

add_definitions(-DEXCEPTION_CHECKS)


add_definitions (-DBOOST_TEST_DYN_LINK)

#################################################
# PixelColumn class test
#################################################
set(PIXEL_COL_SRCS
  pixel_col_test.cpp
  ../../../../source/bench/Alpide/PixelDoubleColumn.cpp)


add_executable(pixel_col_test EXCLUDE_FROM_ALL ${PIXEL_COL_SRCS})
target_link_libraries (pixel_col_test
  ${Boost_SYSTEM_LIBRARY}
  ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
  )


#################################################
# PixelMatrix class test
#################################################
set(PIXEL_MATRIX_SRCS
  pixel_matrix_test.cpp
  ../../../../source/bench/Alpide/PixelDoubleColumn.cpp
  ../../../../source/bench/Alpide/PixelMatrix.cpp)

add_executable(pixel_matrix_test EXCLUDE_FROM_ALL ${PIXEL_MATRIX_SRCS})
target_link_libraries (pixel_matrix_test
  ${Boost_SYSTEM_LIBRARY}
  ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
  )



add_test(NAME alpide_test COMMAND alpide_test)
add_test(NAME pixel_col_test COMMAND pixel_col_test)
add_test(NAME pixel_matrix_test COMMAND pixel_matrix_test)


add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND}
                  DEPENDS alpide_test pixel_col_test pixel_matrix_test)
