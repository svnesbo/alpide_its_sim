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

}

#endif
