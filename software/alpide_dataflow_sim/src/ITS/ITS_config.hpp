/**
 * @file   ITS_config.hpp
 * @author Simon Voigt Nesbo
 * @date   August 15, 2017
 * @brief  Various data structures used for configuration of ITS
 */

#ifndef ITS_CONFIG_HPP
#define ITS_CONFIG_HPP

#include "ITS_constants.hpp"
#include <iostream>

namespace ITS {
  struct layerConfig {
    unsigned int num_staves;
  };

  struct detectorConfig {
    layerConfig layer[N_LAYERS];

    unsigned int alpide_dtu_delay_cycles;
    unsigned int alpide_strobe_length_ns;
    unsigned int alpide_min_busy_cycles;
    bool alpide_strobe_ext;
    bool alpide_cluster_en;
    bool alpide_continuous_mode;
    bool alpide_matrix_speed;
  };

  struct detectorPosition {
    unsigned int layer_id;
    unsigned int stave_id;
    unsigned int sub_stave_id;
    unsigned int module_id;
    unsigned int module_chip_id;

    inline friend std::ostream& operator<<(std::ostream& stream, const detectorPosition& pos) {
      stream << "Layer: " << pos.layer_id;
      stream << ", Stave: " << pos.stave_id;
      stream << ", Sub-stave: " << pos.sub_stave_id;
      stream << ", Module: " << pos.module_id;
      stream << ", Module chip ID: " << pos.module_chip_id;
      return stream;
    }
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


  inline detectorPosition chip_id_to_detector_position(unsigned int chip_id) {
    unsigned int layer_id = 0;
    unsigned int stave_id = 0;
    unsigned int sub_stave_id = 0;
    unsigned int module_id = 0;
    unsigned int chip_num_in_stave = 0;
    unsigned int chip_num_in_module = 0;

    while(layer_id < N_LAYERS-1) {
      if(chip_id < CUMULATIVE_CHIP_COUNT_AT_LAYER[layer_id+1])
        break;
      else
        layer_id++;
    }

    unsigned int chip_num_in_layer = chip_id;

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

    detectorPosition pos = {layer_id,
                            stave_id,
                            sub_stave_id,
                            module_id,
                            chip_num_in_module};

    return pos;
  }

}


#endif
