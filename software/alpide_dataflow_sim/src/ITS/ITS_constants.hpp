/**
 * @file   ITS_constants.hpp
 * @author Simon Voigt Nesbo
 * @date   July 11, 2017
 * @brief  Various constants for the ITS detector
 *
 */

#ifndef ITS_CONSTANTS_HPP
#define ITS_CONSTANTS_HPP

#include <iostream>

namespace ITS {

  static const unsigned int N_LAYERS = 7;

  static const unsigned int CHIPS_PER_IB_STAVE    = 9;
  static const unsigned int CHIPS_PER_HALF_MODULE = 7;
  static const unsigned int CHIPS_PER_FULL_MODULE = 2*CHIPS_PER_HALF_MODULE;
  static const unsigned int MODULES_PER_MB_STAVE  = 8;
  static const unsigned int MODULES_PER_OB_STAVE  = 14;

  static const unsigned int DATA_LINKS_PER_IB_STAVE = 9;
  static const unsigned int CTRL_LINKS_PER_IB_STAVE = 1;

  static const unsigned int DATA_LINKS_PER_HALF_MODULE = 1;
  static const unsigned int CTRL_LINKS_PER_HALF_MODULE = 1;

  static const unsigned int DATA_LINKS_PER_FULL_MODULE = 2*DATA_LINKS_PER_HALF_MODULE;
  static const unsigned int CTRL_LINKS_PER_FULL_MODULE = 2*CTRL_LINKS_PER_HALF_MODULE;


  static const unsigned int STAVES_PER_LAYER[N_LAYERS] = {12,
                                                          16,
                                                          20,
                                                          24,
                                                          30,
                                                          42,
                                                          48};

  static const unsigned int STAVE_COUNT_TOTAL =
    STAVES_PER_LAYER[0] +
    STAVES_PER_LAYER[1] +
    STAVES_PER_LAYER[2] +
    STAVES_PER_LAYER[3] +
    STAVES_PER_LAYER[4] +
    STAVES_PER_LAYER[5] +
    STAVES_PER_LAYER[6];


  static const unsigned int READOUT_UNIT_COUNT = STAVE_COUNT_TOTAL;


  static const unsigned int MODULES_PER_STAVE_IN_LAYER[N_LAYERS] = {1,
                                                                    1,
                                                                    1,
                                                                    MODULES_PER_MB_STAVE,
                                                                    MODULES_PER_MB_STAVE,
                                                                    MODULES_PER_OB_STAVE,
                                                                    MODULES_PER_OB_STAVE};


  static const unsigned int CHIPS_PER_MODULE_IN_LAYER[N_LAYERS] = {CHIPS_PER_IB_STAVE,
                                                                   CHIPS_PER_IB_STAVE,
                                                                   CHIPS_PER_IB_STAVE,
                                                                   CHIPS_PER_FULL_MODULE,
                                                                   CHIPS_PER_FULL_MODULE,
                                                                   CHIPS_PER_FULL_MODULE,
                                                                   CHIPS_PER_FULL_MODULE};


  static const unsigned int CHIPS_PER_STAVE_IN_LAYER[N_LAYERS] = {
    MODULES_PER_STAVE_IN_LAYER[0]*CHIPS_PER_MODULE_IN_LAYER[0],
    MODULES_PER_STAVE_IN_LAYER[1]*CHIPS_PER_MODULE_IN_LAYER[1],
    MODULES_PER_STAVE_IN_LAYER[2]*CHIPS_PER_MODULE_IN_LAYER[2],
    MODULES_PER_STAVE_IN_LAYER[3]*CHIPS_PER_MODULE_IN_LAYER[3],
    MODULES_PER_STAVE_IN_LAYER[4]*CHIPS_PER_MODULE_IN_LAYER[4],
    MODULES_PER_STAVE_IN_LAYER[5]*CHIPS_PER_MODULE_IN_LAYER[5],
    MODULES_PER_STAVE_IN_LAYER[6]*CHIPS_PER_MODULE_IN_LAYER[6]
  };


  static const unsigned int CHIPS_PER_LAYER[N_LAYERS] = {
    STAVES_PER_LAYER[0]*CHIPS_PER_IB_STAVE,
    STAVES_PER_LAYER[1]*CHIPS_PER_IB_STAVE,
    STAVES_PER_LAYER[2]*CHIPS_PER_IB_STAVE,
    STAVES_PER_LAYER[3]*MODULES_PER_MB_STAVE*CHIPS_PER_FULL_MODULE,
    STAVES_PER_LAYER[4]*MODULES_PER_MB_STAVE*CHIPS_PER_FULL_MODULE,
    STAVES_PER_LAYER[5]*MODULES_PER_OB_STAVE*CHIPS_PER_FULL_MODULE,
    STAVES_PER_LAYER[6]*MODULES_PER_OB_STAVE*CHIPS_PER_FULL_MODULE
  };


  static const unsigned int CUMULATIVE_CHIP_COUNT_AT_LAYER[N_LAYERS] = {
    0
    ,
    CHIPS_PER_LAYER[0]
    ,
    CHIPS_PER_LAYER[0]+
    CHIPS_PER_LAYER[1]
    ,
    CHIPS_PER_LAYER[0]+
    CHIPS_PER_LAYER[1]+
    CHIPS_PER_LAYER[2]
    ,
    CHIPS_PER_LAYER[0]+
    CHIPS_PER_LAYER[1]+
    CHIPS_PER_LAYER[2]+
    CHIPS_PER_LAYER[3]
    ,
    CHIPS_PER_LAYER[0]+
    CHIPS_PER_LAYER[1]+
    CHIPS_PER_LAYER[2]+
    CHIPS_PER_LAYER[3]+
    CHIPS_PER_LAYER[4]
    ,
    CHIPS_PER_LAYER[0]+
    CHIPS_PER_LAYER[1]+
    CHIPS_PER_LAYER[2]+
    CHIPS_PER_LAYER[3]+
    CHIPS_PER_LAYER[4]+
    CHIPS_PER_LAYER[5]
  };


  static const unsigned int CHIP_COUNT_TOTAL =
    CHIPS_PER_LAYER[0] +
    CHIPS_PER_LAYER[1] +
    CHIPS_PER_LAYER[2] +
    CHIPS_PER_LAYER[3] +
    CHIPS_PER_LAYER[4] +
    CHIPS_PER_LAYER[5] +
    CHIPS_PER_LAYER[6];


  static const unsigned int DATA_LINKS_PER_LAYER[N_LAYERS] = {
    STAVES_PER_LAYER[0]*DATA_LINKS_PER_IB_STAVE,
    STAVES_PER_LAYER[1]*DATA_LINKS_PER_IB_STAVE,
    STAVES_PER_LAYER[2]*DATA_LINKS_PER_IB_STAVE,
    STAVES_PER_LAYER[3]*MODULES_PER_MB_STAVE*DATA_LINKS_PER_FULL_MODULE,
    STAVES_PER_LAYER[4]*MODULES_PER_MB_STAVE*DATA_LINKS_PER_FULL_MODULE,
    STAVES_PER_LAYER[5]*MODULES_PER_OB_STAVE*DATA_LINKS_PER_FULL_MODULE,
    STAVES_PER_LAYER[6]*MODULES_PER_OB_STAVE*DATA_LINKS_PER_FULL_MODULE
  };


  static const unsigned int CTRL_LINKS_PER_LAYER[N_LAYERS] = {
    STAVES_PER_LAYER[0]*CTRL_LINKS_PER_IB_STAVE,
    STAVES_PER_LAYER[1]*CTRL_LINKS_PER_IB_STAVE,
    STAVES_PER_LAYER[2]*CTRL_LINKS_PER_IB_STAVE,
    STAVES_PER_LAYER[3]*MODULES_PER_MB_STAVE*CTRL_LINKS_PER_FULL_MODULE,
    STAVES_PER_LAYER[4]*MODULES_PER_MB_STAVE*CTRL_LINKS_PER_FULL_MODULE,
    STAVES_PER_LAYER[5]*MODULES_PER_OB_STAVE*CTRL_LINKS_PER_FULL_MODULE,
    STAVES_PER_LAYER[6]*MODULES_PER_OB_STAVE*CTRL_LINKS_PER_FULL_MODULE
  };



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

    inline friend std::ostream& operator<<(std::ostream& stream, const detectorPosition& pos) {
      stream << "Layer: " << pos.layer_id;
      stream << ", Stave: " << pos.stave_id;
      stream << ", Module: " << pos.module_id;
      stream << ", Chip: " << pos.stave_chip_id;
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

    chip_num += pos.module_id * CHIPS_PER_MODULE_IN_LAYER[pos.layer_id];

    chip_num += pos.stave_chip_id;

    return chip_num;
  }


  inline detectorPosition chip_id_to_detector_position(unsigned int chip_id) {
    unsigned int layer_id = 0;
    unsigned int stave_id = 0;
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

    detectorPosition pos = {layer_id,
                            stave_id,
                            module_id,
                            chip_num_in_module};

    return pos;
  }
}

#endif
