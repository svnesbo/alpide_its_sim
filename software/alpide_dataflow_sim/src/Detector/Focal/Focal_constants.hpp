/**
 * @file   Focal_constants.hpp
 * @author Simon Voigt Nesbo
 * @date   November 24, 2019
 * @brief  Various constants for the Focal detector
 *
 */

#ifndef FOCAL_CONSTANTS_HPP
#define FOCAL_CONSTANTS_HPP

#include "Detector/ITS/ITS_constants.hpp"

namespace Focal {

  static const unsigned int N_LAYERS = 2;

  static const unsigned int STAVES_PER_LAYER[N_LAYERS] = {3,
                                                          3};

  ///@brief Number of sub staves per stave in a specific layer.
  ///       Technically there are no sub staves" in the IB staves,
  ///       but the code that uses it requires it to be 1 and not 0 for inner barrel
  static const unsigned int SUB_STAVES_PER_STAVE[N_LAYERS] = {1,
                                                              1};

  ///@brief Number of sub staves per stave in a specific layer.
  ///       Technically there are no sub staves" in the IB staves,
  ///       but the code that uses it requires it to be 1 and not 0 for inner barrel
  static const unsigned int MODULES_PER_SUB_STAVE_IN_LAYER[N_LAYERS] = {1,
                                                                        1};

  static const unsigned int STAVE_COUNT_TOTAL =
    STAVES_PER_LAYER[0] +
    STAVES_PER_LAYER[1];

  static const unsigned int READOUT_UNIT_COUNT = STAVE_COUNT_TOTAL;

  static const unsigned int CHIPS_PER_STAVE_IN_LAYER[N_LAYERS] = {ITS::CHIPS_PER_IB_STAVE,
                                                                  ITS::CHIPS_PER_IB_STAVE};

  static const unsigned int CHIPS_PER_LAYER[N_LAYERS] = {
    STAVES_PER_LAYER[0]*ITS::CHIPS_PER_IB_STAVE,
    STAVES_PER_LAYER[1]*ITS::CHIPS_PER_IB_STAVE};

  ///@brief Number of chips "before" a specific layer
  static const unsigned int CUMULATIVE_CHIP_COUNT_AT_LAYER[N_LAYERS] = {
    0, CHIPS_PER_LAYER[0]
  };


  static const unsigned int CHIP_COUNT_TOTAL =
    CHIPS_PER_LAYER[0] + CHIPS_PER_LAYER[1];


  static const unsigned int DATA_LINKS_PER_LAYER[N_LAYERS] = {
    STAVES_PER_LAYER[0]*ITS::DATA_LINKS_PER_IB_STAVE,
    STAVES_PER_LAYER[1]*ITS::DATA_LINKS_PER_IB_STAVE};

  static const unsigned int CTRL_LINKS_PER_LAYER[N_LAYERS] = {
    STAVES_PER_LAYER[0]*ITS::CTRL_LINKS_PER_IB_STAVE,
    STAVES_PER_LAYER[1]*ITS::CTRL_LINKS_PER_IB_STAVE};

}

#endif
