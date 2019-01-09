cmake_minimum_required(VERSION 2.8)

set(CMAKE_CXX_FLAGS "-Wall -Wextra -O3 -g")

set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
set(CMAKE_MODULE_PATH  ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/../cmake/")

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

project(AlpideDataflowSimulation)

message(${CMAKE_MODULE_PATH})

find_package(Boost REQUIRED)
if(Boost_FOUND)
	include_directories(${Boost_INCLUDE_DIRS})
endif()


find_package(Qt5Core REQUIRED)
get_target_property(QtCore_location Qt5::Core LOCATION)

FIND_PACKAGE(SystemC MODULE REQUIRED)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../bin)

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  set (CMAKE_CXX_FLAGS "--std=c++11 ${CMAKE_CXX_FLAGS}")
endif ()

# Set modules used
set(MODULES
  Alpide
  )

#foreach(MODULE ${MODULES})
#  add_subdirectory(modules/${MODULE})
#  list(APPEND MODULE_INCLUDES modules/${MODULE}/source/bench)
#endforeach()

# Use the CMakeLists.txt file from the parent directory.
# That define is the "module cmake file" for this Alpide module
add_subdirectory("../../" "${CMAKE_CURRENT_BINARY_DIR}/alpide")
list(APPEND MODULE_INCLUDES ../../source/bench)

include_directories(
  ${SystemC_INCLUDE_DIRS}
  ${MODULE_INCLUDES}
  vcd_hierarchy_manipulator
  alpide_dataflow_sim
  )

add_sources(
  src/AlpideDataParser/AlpideDataParser.cpp
  src/ITS/ITSDetector.cpp
  src/ITS/ITSModulesStaves.cpp
  src/ITS/ITSSimulationStats.cpp
  src/ReadoutUnit/ReadoutUnit.cpp
  src/Event/EventGenBase.cpp
  src/Event/EventGenITS.cpp
  #src/Event/EventGenPCT.cpp
  src/Event/EventBase.cpp
  src/Event/EventBinary.cpp
  src/Event/EventXML.cpp
  src/Settings/Settings.cpp
  src/Settings/parse_cmdline_args.cpp
  src/Stimuli/StimuliBase.cpp
  #src/Stimuli/StimuliPCT.cpp
  src/Stimuli/StimuliITS.cpp
  src/main.cpp
  )

# Uncomment to keep track of when pixels are read out from pixel matrix,
# RRUs, TRU, and finally off the chip. All this debug info is printed to stdout.
#add_definitions(-DPIXEL_DEBUG)

add_executable(alpide_dataflow_sim  ${SRCS})
target_link_libraries(alpide_dataflow_sim ${SystemC_LIBRARIES} pthread boost_random Qt5Core)
set_target_properties(alpide_dataflow_sim PROPERTIES LINKER_LANGUAGE CXX)
qt5_use_modules(alpide_dataflow_sim Core Xml)

# add a target to generate API documentation with Doxygen
find_package(Doxygen)
if(DOXYGEN_FOUND)
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/doc/Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)
  add_custom_target(doc
    ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    COMMENT "Generating API documentation with Doxygen" VERBATIM
    )
endif(DOXYGEN_FOUND)


enable_testing()
add_subdirectory(src/unit_tests/)
