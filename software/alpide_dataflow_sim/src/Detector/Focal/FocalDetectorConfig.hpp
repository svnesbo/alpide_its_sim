/**
 * @file   FocalDetectorConfig.hpp
 * @author Simon Voigt Nesbo
 * @date   November 24, 2019
 * @brief  Classes and functions for detector configuration
 *         and position specific for Focal detector.
 */

#ifndef FOCAL_DETECTOR_CONFIG_HPP
#define FOCAL_DETECTOR_CONFIG_HPP

#include "Detector/Common/DetectorConfig.hpp"
#include "Detector/Focal/Focal_constants.hpp"
#include <iostream>

namespace Focal {
  struct FocalDetectorConfig : public Detector::DetectorConfigBase {
    FocalDetectorConfig()
      {
        num_layers = Focal::N_LAYERS;
        layer.resize(Focal::N_LAYERS);

        // Initialize full configuration, all staves included
        for(unsigned int i = 0; i < Focal::N_LAYERS; i++) {
          layer[i].num_staves = Focal::STAVES_PER_LAYER[i];
          layer[i].num_sub_staves_per_full_stave = 0; // Not used in Focal
          layer[i].num_modules_per_sub_stave = 0; // Not used in Focal
          layer[i].num_chips_per_module = CHIPS_PER_STAVE_IN_LAYER[i];
        }
      }
  };

  unsigned int Focal_position_to_global_chip_id(const Detector::DetectorPosition& pos);
  Detector::DetectorPosition Focal_global_chip_id_to_position(unsigned int global_chip_id);
}


#endif
