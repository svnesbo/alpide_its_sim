/**
 * @file   ITS_config.hpp
 * @author Simon Voigt Nesbo
 * @date   August 15, 2017
 * @brief  Various data structures used for configuration of ITS
 */

#ifndef ITS_CONFIG_HPP
#define ITS_CONFIG_HPP

#include "ITS_constants.hpp"

namespace ITS {

  struct layerConfig {
    unsigned int num_staves;
  };

  struct detectorConfig {
    layerConfig layer[N_LAYERS];
  };

  struct detectorPosition {
    unsigned int layer_id;
    unsigned int stave_id;
    unsigned int module_id;
    unsigned int stave_chip_id;
  };

  inline unsigned int chip_id_to_layer_id(unsigned int chip_id) {
    unsigned int chip_num = 0;
    unsigned int layer;

    for(layer = 0; layer < N_LAYERS; layer++) {
      if(chip_id < chip_num+CHIPS_PER_LAYER[layer])
        break;
      chip_num += CHIPS_PER_LAYER[layer];
    }
    return layer;
  }


  inline unsigned int detector_position_to_chip_id(const detectorPosition &pos) {
    unsigned int chip_num;

    chip_num = CUMULATIVE_CHIP_COUNT_AT_LAYER[pos.layer_id];

    chip_num += pos.stave_id * CHIPS_PER_STAVE_IN_LAYER[pos.layer_id];

    chip_num += pos.module_id * CHIPS_PER_MODULE_IN_LAYER[pos.layer_id];

    chip_num += pos.stave_chip_id;

    return chip_num;
  }
}


#endif
