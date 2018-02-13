cmake_minimum_required(VERSION 2.8)

project(analysis_macros)

set(CMAKE_CXX_FLAGS "-Wall -Wextra -O3 -g")

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  set (CMAKE_CXX_FLAGS "--std=c++11 ${CMAKE_CXX_FLAGS}")
endif ()

#set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../bin)


# Find ROOT with CMake:
# https://root.cern.ch/how/integrate-root-my-project-cmake

list(APPEND CMAKE_PREFIX_PATH $ENV{ROOTSYS})

#---Locate the ROOT package and defines a number of variables (e.g. ROOT_INCLUDE_DIRS)
find_package(ROOT REQUIRED COMPONENTS RooFit Html Minuit RooFitCore RooStats HistFactory)

#---Define useful ROOT functions and macros (e.g. ROOT_GENERATE_DICTIONARY)
include(${ROOT_USE_FILE})



find_package(Qt5Core REQUIRED)
get_target_property(QtCore_location Qt5::Core LOCATION)


#include_directories(
#  ${SystemC_INCLUDE_DIRS}
#  ${MODULE_INCLUDES}
#  vcd_hierarchy_manipulator
#  alpide_dataflow_sim
#  )

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


add_sources(
  misc.cpp
  LinkStats.cpp
  ReadoutUnitStats.cpp
  ITSLayerStats.cpp
  DetectorStats.cpp
  process_readout_trigger_stats.C
)

add_executable(process_readout_trigger_stats ${SRCS})
target_link_libraries(process_readout_trigger_stats Qt5Core)
target_link_libraries(process_readout_trigger_stats ${ROOT_LIBRARIES})
set_target_properties(process_readout_trigger_stats PROPERTIES LINKER_LANGUAGE CXX)
qt5_use_modules(process_readout_trigger_stats Core Xml)

add_executable(compare_simulations compare_simulations.C)
target_link_libraries(compare_simulations ${ROOT_LIBRARIES})
set_target_properties(compare_simulations PROPERTIES LINKER_LANGUAGE CXX)

# add a target to generate API documentation with Doxygen
#find_package(Doxygen)
#if(DOXYGEN_FOUND)
#  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/doc/Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)
#  add_custom_target(doc
#    ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
#    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
#    COMMENT "Generating API documentation with Doxygen" VERBATIM
#    )
#endif(DOXYGEN_FOUND)
