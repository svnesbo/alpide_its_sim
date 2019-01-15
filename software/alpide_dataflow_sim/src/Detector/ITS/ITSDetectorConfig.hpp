/**
 * @file   ITSDetectorConfig.hpp
 * @author Simon Voigt Nesbo
 * @date   January 15, 2019
 * @brief  Classes and functions for detector configuration
 *         and position specific for ITS detector.
 */

#ifndef ITS_DETECTOR_CONFIG_HPP
#define ITS_DETECTOR_CONFIG_HPP

#include "Detector/Common/DetectorConfig.hpp"
#include "Detector/ITS/ITS_constants.hpp"
#include <iostream>

namespace ITS {
  struct ITSDetectorConfig : public Detector::DetectorConfigBase {
    ITSDetectorConfig()
      {
        num_layers = ITS::N_LAYERS;
        layer.resize(ITS::N_LAYERS);

        std::cout << "Initializing ITSDetectorConfig..." << std::endl;

        // Initialize full configuration, all staves included
        for(unsigned int i = 0; i < ITS::N_LAYERS; i++) {
          layer[i].num_staves = STAVES_PER_LAYER[i];
          layer[i].num_sub_staves_per_full_stave = SUB_STAVES_PER_STAVE[i];
          layer[i].num_modules_per_sub_stave = MODULES_PER_SUB_STAVE_IN_LAYER[i];
          layer[i].num_chips_per_module = CHIPS_PER_MODULE_IN_LAYER[i];
        }
      }
  };

  unsigned int ITS_position_to_global_chip_id(const Detector::DetectorPosition& pos);
  Detector::DetectorPosition ITS_global_chip_id_to_position(unsigned int global_chip_id);

/*
  struct ITSDetectorPosition : public Detector::DetectorPositionBase {
    ITSDetectorPosition()
      {
        position.layer_id = 0;
        position.stave_id = 0;
        position.sub_stave_id = 0;
        position.module_id = 0;
        position.module_chip_id = 0;
      }

    ITSDetectorPosition(unsigned int global_chip_id) {
      globalChipIdToPosition(global_chip_id);
    }

    inline unsigned int toGlobalChipId(void) const {
      return ITS_detector_position_to_chip_id(position);
    }

    inline void globalChipIdToPosition(unsigned int global_chip_id) {
      position = ITS_chip_id_to_detector_position(global_chip_id);
    }
  };
*/
}


#endif
