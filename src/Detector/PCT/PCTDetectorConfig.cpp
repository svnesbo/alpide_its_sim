/**
 * @file   PCTDetectorConfig.cpp
 * @author Simon Voigt Nesbo
 * @date   January 15, 2019
 * @brief  Classes and functions for detector configuration
 *         and position specific for PCT detector.
 */

#include "PCTDetectorConfig.hpp"
#include <iostream>

unsigned int PCT::PCT_position_to_global_chip_id(const Detector::DetectorPosition& pos)
{
  unsigned int chip_num;

  chip_num = pos.layer_id * PCT::CHIPS_PER_LAYER;
  chip_num += pos.stave_id * PCT::CHIPS_PER_STAVE;
  chip_num += pos.module_chip_id;

  return chip_num;
}

Detector::DetectorPosition PCT::PCT_global_chip_id_to_position(unsigned int global_chip_id) {
  unsigned int layer_id = global_chip_id / PCT::CHIPS_PER_LAYER;
  unsigned int chip_num_in_layer = global_chip_id % PCT::CHIPS_PER_LAYER;

  unsigned int stave_id = chip_num_in_layer / PCT::CHIPS_PER_STAVE;
  unsigned int chip_num_in_stave = chip_num_in_layer % PCT::CHIPS_PER_STAVE;

  Detector::DetectorPosition position = {layer_id,
                                         stave_id,
                                         0,
                                         0,
                                         chip_num_in_stave};

  return position;
}
