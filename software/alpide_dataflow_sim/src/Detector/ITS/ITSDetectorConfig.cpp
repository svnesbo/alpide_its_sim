/**
 * @file   ITSDetectorConfig.cpp
 * @author Simon Voigt Nesbo
 * @date   January 15, 2019
 * @brief  Classes and functions for detector configuration
 *         and position specific for ITS detector.
 */

#include "ITSDetectorConfig.hpp"
#include <iostream>

using namespace ITS;

unsigned int ITS::ITS_position_to_global_chip_id(const Detector::DetectorPosition& pos)
{
  unsigned int chip_num;

  chip_num = CUMULATIVE_CHIP_COUNT_AT_LAYER[pos.layer_id];

  chip_num += pos.stave_id * CHIPS_PER_STAVE_IN_LAYER[pos.layer_id];

  // sub_stave is always 0 for inner barrel stave, and either 0 or 1 for middle and outer
  // barrel staves. For MB/OB stave, add number of chips in first sub stave (when sub_stave
  // is 1), and then add the chip number within the sub_stave)
  chip_num += pos.sub_stave_id *
    MODULES_PER_SUB_STAVE_IN_LAYER[pos.layer_id] *
    CHIPS_PER_MODULE_IN_LAYER[pos.layer_id];

  chip_num += pos.module_id * CHIPS_PER_MODULE_IN_LAYER[pos.layer_id];

  chip_num += pos.module_chip_id;

  return chip_num;
}

Detector::DetectorPosition ITS::ITS_global_chip_id_to_position(unsigned int global_chip_id) {
  unsigned int layer_id = 0;
  unsigned int stave_id = 0;
  unsigned int sub_stave_id = 0;
  unsigned int module_id = 0;
  unsigned int chip_num_in_stave = 0;
  unsigned int chip_num_in_module = 0;

  while(layer_id < N_LAYERS-1) {
    if(global_chip_id < CUMULATIVE_CHIP_COUNT_AT_LAYER[layer_id+1])
      break;
    else
      layer_id++;
  }

  unsigned int chip_num_in_layer = global_chip_id;

  if(layer_id > 0)
    chip_num_in_layer -= CUMULATIVE_CHIP_COUNT_AT_LAYER[layer_id];

  stave_id = chip_num_in_layer / CHIPS_PER_STAVE_IN_LAYER[layer_id];
  chip_num_in_stave = chip_num_in_layer % CHIPS_PER_STAVE_IN_LAYER[layer_id];

  module_id = chip_num_in_stave / CHIPS_PER_MODULE_IN_LAYER[layer_id];
  chip_num_in_module = chip_num_in_stave % CHIPS_PER_MODULE_IN_LAYER[layer_id];

  // Middle/outer barrel stave? Calculate sub stave id
  if(layer_id > 2) {
    sub_stave_id = module_id / MODULES_PER_SUB_STAVE_IN_LAYER[layer_id];
    module_id = module_id % MODULES_PER_SUB_STAVE_IN_LAYER[layer_id];
  }

  Detector::DetectorPosition position = {layer_id,
                                         stave_id,
                                         sub_stave_id,
                                         module_id,
                                         chip_num_in_module};

  return position;
}
