/**
 * @file   ITSDetectorConfig.hpp
 * @author Simon Voigt Nesbo
 * @date   August 15, 2017
 * @brief  Classes and functions for detector configuration
 *         and position specific for ITS detector.
 */

#ifndef ITS_DETECTOR_CONFIG_HPP
#define ITS_DETECTOR_CONFIG_HPP

#include "Detector/Common/DetectorConfig.hpp"
#include "Detector/ITS/ITS_constants.hpp"
#include <iostream>

namespace ITS {
  struct ITSDetectorConfig : public DetectorConfigBase {
    ITSDetectorConfig()
      : num_layers(ITS::N_LAYERS)
      , layers(ITS::N_LAYERS)
      {
        // Initialize full configuration, all staves included
        for(unsigned int i = 0; i < ITS::N_LAYERS; i++) {
          layers[i].staves_per_layers = STAVES_PER_LAYER[i];
          layers[i].num_sub_staves_per_full_stave = SUB_STAVES_PER_STAVE[i];
        }
      }
  };


  struct ITSDetectorPosition : public DetectorPositionBase {
    ITSDetectorPosition(unsigned int global_chip_id) {
      globalChipIdToPosition(global_chip_id);
    }

    inline unsigned int toGlobalChipId(void) const {
      unsigned int chip_num;

      chip_num = CUMULATIVE_CHIP_COUNT_AT_LAYER[position.layer_id];

      chip_num += position.stave_id * CHIPS_PER_STAVE_IN_LAYER[position.layer_id];

      // sub_stave is always 0 for inner barrel stave, and either 0 or 1 for middle and outer
      // barrel staves. For MB/OB stave, add number of chips in first sub stave (when sub_stave
      // is 1), and then add the chip number within the sub_stave)
      chip_num += position.sub_stave_id *
        MODULES_PER_SUB_STAVE_IN_LAYER[position.layer_id] *
        CHIPS_PER_MODULE_IN_LAYER[position.layer_id];

      chip_num += position.module_id * CHIPS_PER_MODULE_IN_LAYER[position.layer_id];

      chip_num += position.module_chip_id;

      return chip_num;
    }

    void globalChipIdToPosition(unsigned int global_chip_id) {
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

      position.layer_id = layer_id;
      position.stave_id = stave_id;
      position.sub_stave_id = sub_stave_id;
      position.module_id = module_id;
      position.module_chip_id = chip_num_in_module;
    }
  };



}


#endif
