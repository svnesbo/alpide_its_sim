/**
 * @file   FocalDetectorConfig.cpp
 * @author Simon Voigt Nesbo
 * @date   November 24, 2019
 * @brief  Classes and functions for detector configuration
 *         and position specific for Focal detector.
 */

#include "FocalDetectorConfig.hpp"
#include <iostream>


unsigned int Focal::Focal_position_to_global_chip_id(const Detector::DetectorPosition& pos)
{
  unsigned int global_chip_id = pos.layer_id*CHIPS_PER_LAYER;

  // Quadrant 0: top right, 1: top left, 2: bottom left, 3: bottom right
  unsigned int quadrant = pos.stave_id/STAVES_PER_QUADRANT;
  unsigned int stave_in_quadrant = pos.stave_id % STAVES_PER_QUADRANT;

  global_chip_id += quadrant*CHIPS_PER_QUADRANT;
  global_chip_id += stave_in_quadrant*CHIPS_PER_STAVE;

  if(stave_in_quadrant < INNER_STAVES_PER_QUADRANT) {
    // Focal Inner Stave
    if(pos.module_id > 0) {
      // Second module with 7 outer barrel chips in this stave
      global_chip_id += CHIPS_PER_FOCAL_IB_MODULE;
    }
  } else {
    // Focal Outer Stave
    global_chip_id += pos.module_id * CHIPS_PER_FOCAL_OB_MODULE;
  }

  global_chip_id += pos.module_chip_id;

  return global_chip_id;
}


Detector::DetectorPosition Focal::Focal_global_chip_id_to_position(unsigned int global_chip_id) {
  unsigned int layer_id = 0;
  unsigned int stave_id = 0;
  unsigned int sub_stave_id = 0;
  unsigned int module_id = 0;
  unsigned int module_chip_id = 0;

  layer_id = global_chip_id/CHIPS_PER_LAYER;
  global_chip_id -= layer_id*CHIPS_PER_LAYER;

  stave_id = global_chip_id/CHIPS_PER_STAVE;
  global_chip_id -= stave_id*CHIPS_PER_STAVE;

  unsigned int stave_in_quadrant = stave_id % STAVES_PER_QUADRANT;

  if(stave_in_quadrant < INNER_STAVES_PER_QUADRANT) {
    // Focal Inner Stave
    if(global_chip_id >= CHIPS_PER_FOCAL_IB_MODULE) {
      module_id = 1;
      global_chip_id -= CHIPS_PER_FOCAL_IB_MODULE;
    }
  } else {
    // Focal Outer Stave
    module_id = global_chip_id / CHIPS_PER_FOCAL_OB_MODULE;
    global_chip_id -= module_id * CHIPS_PER_FOCAL_OB_MODULE;
  }

  module_chip_id = global_chip_id;

  Detector::DetectorPosition position = {layer_id,
                                         stave_id,
                                         sub_stave_id, // Not used in Focal
                                         module_id,
                                         module_chip_id};
  return position;
}
