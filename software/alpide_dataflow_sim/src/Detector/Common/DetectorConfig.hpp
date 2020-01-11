/**
 * @file   DetectorConfig.hpp
 * @author Simon Voigt Nesbo
 * @date   January 14, 2019
 * @brief  Various common data structures and functions used for
 *         configuration of detector classes (ITS and PCT)
 */

#ifndef DETECTOR_CONFIG_HPP
#define DETECTOR_CONFIG_HPP

#include "Alpide/AlpideConfig.hpp"
#include <iostream>
#include <vector>

namespace Detector {
  struct LayerConfig {
    unsigned int num_staves;
    unsigned int num_sub_staves_per_full_stave;
    unsigned int num_modules_per_sub_stave;
    unsigned int num_chips_per_module;
  };

  struct DetectorConfigBase {
    unsigned int num_layers;
    unsigned int staves_per_quadrant; // Used by Focal only
    std::vector<LayerConfig> layer;
    AlpideConfig chip_cfg;
  };

  struct DetectorPosition {
    unsigned int layer_id;
    unsigned int stave_id;
    unsigned int sub_stave_id;
    unsigned int module_id;
    unsigned int module_chip_id;

    inline friend std::ostream& operator<<(std::ostream& stream, const DetectorPosition& pos) {
      stream << "Layer: " << pos.layer_id;
      stream << ", Stave: " << pos.stave_id;
      stream << ", Sub-stave: " << pos.sub_stave_id;
      stream << ", Module: " << pos.module_id;
      stream << ", Module chip ID: " << pos.module_chip_id;
      return stream;
    }
  };

  /// Definition of function for determining detector position based on global chip id
  typedef DetectorPosition (*t_global_chip_id_to_position_func)(const unsigned int);

  /// Definition of function for determining global chip id based on detector position
  typedef unsigned int (*t_position_to_global_chip_id_func)(const DetectorPosition&);
}


#endif
