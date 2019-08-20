/**
 * @file   PCTDetectorConfig.hpp
 * @author Simon Voigt Nesbo
 * @date   January 15, 2018
 * @brief  Classes and functions for detector configuration
 *         and position specific for PCT detector.
 */

#ifndef PCT_DETECTOR_CONFIG_HPP
#define PCT_DETECTOR_CONFIG_HPP

#include "Detector/Common/DetectorConfig.hpp"
#include "Detector/PCT/PCT_constants.hpp"
#include <iostream>

namespace PCT {
  struct PCTDetectorConfig : public Detector::DetectorConfigBase {
    PCTDetectorConfig()
      {
        num_layers = PCT::N_LAYERS;
        layer.resize(PCT::N_LAYERS);

        // Initialize full configuration, all staves included
        for(unsigned int i = 0; i < PCT::N_LAYERS; i++) {
          layer[i].num_staves = PCT::STAVES_PER_LAYER;
          layer[i].num_sub_staves_per_full_stave = PCT::SUB_STAVES_PER_STAVE;
          layer[i].num_modules_per_sub_stave = PCT::MODULES_PER_SUB_STAVE;
          layer[i].num_chips_per_module = PCT::CHIPS_PER_MODULE;
        }
      }
  };

  unsigned int PCT_position_to_global_chip_id(const Detector::DetectorPosition& pos);
  Detector::DetectorPosition PCT_global_chip_id_to_position(unsigned int global_chip_id);
}


#endif
