/**
 * @file   alpide.h
 * @Author Simon Voigt Nesbo
 * @date   November 27, 2016
 * @brief  Classes for alpide chip implementation
 *
 * Detailed description of file.
 */

#ifndef ALPIDE_H
#define ALPIDE_H

#include <queue>
#include <set>
#include "alpide_constants.h"

// See figures 2.1 and 2.2 in alpide operations manual
// RRU - Region Readout Unit
// TRU - Top Region Unit
// DMU - Data Management Unit
// DTU - Data Transfer Unit

// RRU - 128x24b DPRAM, 24b x 40MHz data rate
// TRU - 24b x 40MHz in, 24b x 40MHz out

// DColumn --+--- Region --- RRU ---+---- TRU (32:1 MUX) ---- DMU ---- DTU
//           |                      |
// DColumn --+                      |
//           |                      |
// DColumn --+                      |
//   ...     |                      |
// DColumn --+                      |
//                                  |
//                                  |
//                                  |
// DColumn --+--- Region --- RRU ---+ 
//           |                      |
// DColumn --+                      |
//           |                      |
// DColumn --+                      |
//   ...     |                      |
// DColumn --+                      |
//                                  |
// ...                              |
//                                  |
// DColumn --+--- Region --- RRU ---+
//           |
// DColumn --+
//           |
// DColumn --+
//   ...     |
// DColumn --+ 


// Data transfer:
// Region header is only sent for regions with data
// 


class RegionReadoutUnit
{
  PixelRegion pixels;

  // Implement framing and stuff here.
  // Implement the 128x24b FIFO here. We can use 128 x 3 words FIFO, where we have a special object for the words,
  // similar to what Adam did. The words can be the valid "control words" for the ALPIDE data transfer, or they
  // can be hit data (which I guess is a control word, actually).

  // I think the TRU can be responsible for placing the Region Header word with the correct region number into the data stream.
  // Read out hits from the double columns, one by one, starting at column 0. Feed the data into the FIFO.
};

class TopRegionUnit
{
  // Essentially a 32:1 MUX, that picks data from the RRUs, starting at RRU 0.
  // The TRU can be responsible for putting the Region Header word (with the correct region ID) onto the data stream.
};
  

class Alpide
{
private:
  std::queue<hit_data> region_fifo[N_REGIONS];
  std::queue<hit_data> output_fifo;
  PixelMatrix matrix;
public:
  Alpide();
  setPixel(unsigned int col_num, unsigned int row_num) {
    matrix.setPixel(col_num, row_num);
  }
};



#endif
