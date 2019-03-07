/**
 * @file   PCT_constants.hpp
 * @author Simon Voigt Nesbo
 * @date   January 15, 2019
 * @brief  Various constants for the PCT detector
 *
 */

#ifndef PCT_CONSTANTS_HPP
#define PCT_CONSTANTS_HPP

namespace PCT {
  static const unsigned int N_LAYERS = 41;
  static const unsigned int STAVES_PER_LAYER = 12;

  // PCT consists entirely of IB staves
  static const unsigned int SUB_STAVES_PER_STAVE = 1;
  static const unsigned int MODULES_PER_SUB_STAVE = 1;
  static const unsigned int CHIPS_PER_STAVE = 9;
  static const unsigned int CHIPS_PER_MODULE = CHIPS_PER_STAVE;

  static const unsigned int CHIPS_PER_LAYER = CHIPS_PER_STAVE*STAVES_PER_LAYER;
  static const unsigned int CHIP_COUNT_TOTAL = CHIPS_PER_LAYER*N_LAYERS;

  static const unsigned int READOUT_UNITS_PER_LAYER = 1;

  static const unsigned int CTRL_LINKS_PER_STAVE = 1;
  static const unsigned int DATA_LINKS_PER_STAVE = CHIPS_PER_STAVE;
  static const unsigned int CTRL_LINKS_PER_RU = STAVES_PER_LAYER*DATA_LINKS_PER_STAVE;
  static const unsigned int DATA_LINKS_PER_RU = STAVES_PER_LAYER*DATA_LINKS_PER_STAVE;
}

#endif
