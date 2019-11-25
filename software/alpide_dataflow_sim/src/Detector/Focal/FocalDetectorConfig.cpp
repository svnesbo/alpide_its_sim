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
  unsigned int chip_num;

  chip_num = Focal::CUMULATIVE_CHIP_COUNT_AT_LAYER[pos.layer_id];

  chip_num += pos.stave_id * Focal::CHIPS_PER_STAVE_IN_LAYER[pos.layer_id];

  chip_num += pos.module_chip_id;

  return chip_num;
}

Detector::DetectorPosition Focal::Focal_global_chip_id_to_position(unsigned int global_chip_id) {
  unsigned int layer_id = 0;
  unsigned int stave_id = 0;
  unsigned int sub_stave_id = 0;
  unsigned int module_id = 0;
  unsigned int chip_num_in_stave = 0;

  while(layer_id < Focal::N_LAYERS-1) {
    if(global_chip_id < Focal::CUMULATIVE_CHIP_COUNT_AT_LAYER[layer_id+1])
      break;
    else
      layer_id++;
  }

  unsigned int chip_num_in_layer = global_chip_id;

  if(layer_id > 0)
    chip_num_in_layer -= Focal::CUMULATIVE_CHIP_COUNT_AT_LAYER[layer_id];

  stave_id = chip_num_in_layer / Focal::CHIPS_PER_STAVE_IN_LAYER[layer_id];
  chip_num_in_stave = chip_num_in_layer % Focal::CHIPS_PER_STAVE_IN_LAYER[layer_id];

  Detector::DetectorPosition position = {layer_id,
                                         stave_id,
                                         sub_stave_id, // Not used in Focal
                                         module_id,    // Not used in Focal
                                         chip_num_in_stave};

  return position;
}
