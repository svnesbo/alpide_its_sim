/**
 * @file   region_readout.cpp
 * @Author Simon Voigt Nesbo
 * @date   November 27, 2016
 * @brief  Class for implementing the Region Readout Unit (RRU) in the Alpide chip.
 *
 * Detailed description of file.
 */

#include <iostream>
#include "region_readout.h"


RegionReadoutUnit::RegionReadoutUnit(PixelRegion* r) {
  region = r;

  //@todo Throw an exception here maybe?
  if(r == NULL) {
    std::cout << "Error. Pixel row address > number of cols. Hit ignored." << std::endl
  }
}
