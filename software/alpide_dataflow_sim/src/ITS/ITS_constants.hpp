/**
 * @file   ITS_constants.hpp
 * @author Simon Voigt Nesbo
 * @date   July 11, 2017
 * @brief  Various constants for the ITS detector
 *
 */

#ifndef ITS_CONSTANTS_HPP
#define ITS_CONSTANTS_HPP


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


  static const unsigned int STAVES_PER_LAYER = {12,
                                                16,
                                                20,
                                                24,
                                                30,
                                                42,
                                                48};


  static const unsigned int MODULES_PER_STAVE_IN_LAYER = {1,
                                                          1,
                                                          1,
                                                          MODULES_PER_MB_STAVE,
                                                          MODULES_PER_MB_STAVE,
                                                          MODULES_PER_OB_STAVE,
                                                          MODULES_PER_OB_STAVE};


  static const unsigned int CHIPS_PER_MODULE_IN_LAYER = {CHIPS_PER_IB_STAVE,
                                                         CHIPS_PER_IB_STAVE,
                                                         CHIPS_PER_IB_STAVE,
                                                         CHIPS_PER_FULL_MODULE,
                                                         CHIPS_PER_FULL_MODULE,
                                                         CHIPS_PER_FULL_MODULE,
                                                         CHIPS_PER_FULL_MODULE};


  static const unsigned int CHIPS_PER_STAVE_IN_LAYER = {
    MODULES_PER_STAVE_IN_LAYER[0]*CHIPS_PER_MODULE_IN_LAYER[0],
    MODULES_PER_STAVE_IN_LAYER[1]*CHIPS_PER_MODULE_IN_LAYER[1],
    MODULES_PER_STAVE_IN_LAYER[2]*CHIPS_PER_MODULE_IN_LAYER[2],
    MODULES_PER_STAVE_IN_LAYER[3]*CHIPS_PER_MODULE_IN_LAYER[3],
    MODULES_PER_STAVE_IN_LAYER[4]*CHIPS_PER_MODULE_IN_LAYER[4],
    MODULES_PER_STAVE_IN_LAYER[5]*CHIPS_PER_MODULE_IN_LAYER[5],
    MODULES_PER_STAVE_IN_LAYER[6]*CHIPS_PER_MODULE_IN_LAYER[6]
  };


  static const unsigned int CHIPS_PER_LAYER = {
    STAVES_PER_LAYER[0]*CHIPS_PER_IB_STAVE,
    STAVES_PER_LAYER[1]*CHIPS_PER_IB_STAVE,
    STAVES_PER_LAYER[2]*CHIPS_PER_IB_STAVE,
    STAVES_PER_LAYER[3]*MODULES_PER_MB_STAVE*CHIPS_PER_FULL_MODULE,
    STAVES_PER_LAYER[4]*MODULES_PER_MB_STAVE*CHIPS_PER_FULL_MODULE,
    STAVES_PER_LAYER[5]*MODULES_PER_OB_STAVE*CHIPS_PER_FULL_MODULE,
    STAVES_PER_LAYER[6]*MODULES_PER_OB_STAVE*CHIPS_PER_FULL_MODULE
  };


  static const unsigned int CUMULATIVE_CHIP_COUNT_AT_LAYER = {
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


  static const unsigned int DATA_LINKS_PER_LAYER = {
    STAVES_PER_LAYER[0]*DATA_LINKS_PER_IB_STAVE,
    STAVES_PER_LAYER[1]*DATA_LINKS_PER_IB_STAVE,
    STAVES_PER_LAYER[2]*DATA_LINKS_PER_IB_STAVE,
    STAVES_PER_LAYER[3]*MODULES_PER_MB_STAVE*DATA_LINKS_PER_FULL_MODULE,
    STAVES_PER_LAYER[4]*MODULES_PER_MB_STAVE*DATA_LINKS_PER_FULL_MODULE,
    STAVES_PER_LAYER[5]*MODULES_PER_OB_STAVE*DATA_LINKS_PER_FULL_MODULE,
    STAVES_PER_LAYER[6]*MODULES_PER_OB_STAVE*DATA_LINKS_PER_FULL_MODULE
  };


  static const unsigned int CTRL_LINKS_PER_LAYER = {
    STAVES_PER_LAYER[0]*CTRL_LINKS_PER_IB_STAVE,
    STAVES_PER_LAYER[1]*CTRL_LINKS_PER_IB_STAVE,
    STAVES_PER_LAYER[2]*CTRL_LINKS_PER_IB_STAVE,
    STAVES_PER_LAYER[3]*MODULES_PER_MB_STAVE*CTRL_LINKS_PER_FULL_MODULE,
    STAVES_PER_LAYER[4]*MODULES_PER_MB_STAVE*CTRL_LINKS_PER_FULL_MODULE,
    STAVES_PER_LAYER[5]*MODULES_PER_OB_STAVE*CTRL_LINKS_PER_FULL_MODULE,
    STAVES_PER_LAYER[6]*MODULES_PER_OB_STAVE*CTRL_LINKS_PER_FULL_MODULE
  };


  // static const unsigned int CHIPS_PER_LAYER = {108,
  //                                              144,
  //                                              180,
  //                                              2688,
  //                                              3360,
  //                                              8232,
  //                                              9408};

  // static const unsigned int LINKS_PER_LAYER = {108,
  //                                              144,
  //                                              180,
  //                                              384,
  //                                              480,
  //                                              1176,
  //                                              1344};


  inline unsigned int chip_id_to_layer_id(unsigned int chip_id) {
    unsigned int chip_num = 0;

    for(int layer = 0; layer < N_LAYERS; layer++) {
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

    chip_num += stave_chip_id;

    return chip_num;
  }

}

#endif
