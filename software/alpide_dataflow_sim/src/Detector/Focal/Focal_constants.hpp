/**
 * @file   Focal_constants.hpp
 * @author Simon Voigt Nesbo
 * @date   November 24, 2019
 * @brief  Various constants for the Focal detector
 *
 */

#ifndef FOCAL_CONSTANTS_HPP
#define FOCAL_CONSTANTS_HPP

#include "Alpide/alpide_constants.hpp"

namespace Focal {
  static const unsigned int N_LAYERS = 2;

  static const double MACRO_CELL_SIZE_X_MM = 0.5;
  static const double MACRO_CELL_SIZE_Y_MM = 0.5;

  static const double GAP_SIZE_X_MM = 80;
  static const double GAP_SIZE_Y_MM = 80;

  static const double GAP_SIZE_X_CELLS = GAP_SIZE_X_MM/MACRO_CELL_SIZE_X_MM;
  static const double GAP_SIZE_Y_CELLS = GAP_SIZE_Y_MM/MACRO_CELL_SIZE_Y_MM;

  // Inner Stave structure: <Module 0><Module 1>
  // Module 0: 8x IB chips
  // Module 1: 7x OB chips

  // Outer Stave structure: <Module 0><Module 1><Module 2>
  // Module 0: 5x OB chips
  // Module 1: 5x OB chips
  // Module 2: 5x OB chips

  static const unsigned int CHIPS_PER_FOCAL_IB_MODULE = 8;
  static const unsigned int CHIPS_PER_FOCAL_OB_MODULE = 5;
  static const unsigned int CHIPS_PER_STAVE = 15;
  static const unsigned int STAVES_PER_PATCH = 6;
  static const unsigned int STAVES_PER_HALF_PATCH = 3;
  static const unsigned int HALF_PATCHES_PER_QUADRANT = 11;
  static const unsigned int HALF_PATCHES_PER_LAYER = 4*HALF_PATCHES_PER_QUADRANT;

  static const unsigned int STAVES_PER_QUADRANT = HALF_PATCHES_PER_QUADRANT*STAVES_PER_HALF_PATCH;
  static const unsigned int STAVES_PER_LAYER = STAVES_PER_HALF_PATCH*HALF_PATCHES_PER_LAYER;
  static const unsigned int CHIPS_PER_QUADRANT = STAVES_PER_QUADRANT*CHIPS_PER_STAVE;
  static const unsigned int CHIPS_PER_LAYER = STAVES_PER_LAYER*CHIPS_PER_STAVE;

  static const unsigned int CUMULATIVE_CHIP_COUNT_AT_LAYER[2] = {0,
                                                                 CHIPS_PER_LAYER};

  // Half-patches of FocalInnerStave (IB+OB modules)
  static const unsigned int INNER_HALF_PATCHES_PER_QUADRANT = 5;
  static const unsigned int INNER_STAVES_PER_QUADRANT = STAVES_PER_HALF_PATCH*INNER_HALF_PATCHES_PER_QUADRANT;

  static const unsigned int MODULES_PER_INNER_STAVE = 2; // 8 IB chips + 7 OB chips
  static const unsigned int MODULES_PER_OUTER_STAVE = 3; // 5 + 5 + 5 OB chips

  //static const unsigned int CTRL_LINKS_PER_INNER_STAVE = 1;
  static const unsigned int CTRL_LINKS_PER_INNER_STAVE = 2;
  static const unsigned int DATA_LINKS_PER_INNER_STAVE = CHIPS_PER_FOCAL_IB_MODULE+1;

  //static const unsigned int CTRL_LINKS_PER_OUTER_STAVE = 1;
  static const unsigned int CTRL_LINKS_PER_OUTER_STAVE = 3;
  static const unsigned int DATA_LINKS_PER_OUTER_STAVE = 3;

  static const double STAVE_SIZE_X_MM = (CHIP_WIDTH_CM*10) * CHIPS_PER_STAVE;
  static const double STAVE_SIZE_Y_MM = CHIP_HEIGHT_CM*10;

  static const double PATCH_SIZE_X_MM = STAVE_SIZE_X_MM;
  static const double PATCH_SIZE_Y_MM = STAVE_SIZE_Y_MM*STAVES_PER_PATCH;

  static const double HALF_PATCH_SIZE_X_MM = PATCH_SIZE_X_MM;
  static const double HALF_PATCH_SIZE_Y_MM = PATCH_SIZE_Y_MM/2;
}

#endif
