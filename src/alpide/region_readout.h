/**
 * @file   region_readout.h
 * @Author Simon Voigt Nesbo
 * @date   November 27, 2016
 * @brief  Class for implementing the Region Readout Unit (RRU) in the Alpide chip.
 *
 * Detailed description of file.
 */

#ifndef REGION_READOUT_H
#define REGION_READOUT_H

#include "pixel_region.h"


class RegionReadoutUnit
{
private:
  PixelRegion* region;
  unsigned int current_region;
  bool fifo_size_limit;
  bool busy_signaled;
  
  sc_core::sc_fifo<DataWordBase> RRU_FIFO;

  //DataWordBase getNextFifoWord(unsigned int region_id);
  DataWordBase getNextFifoWord(void);
  void updateFifo(void);

public:
  RegionReadoutUnit(PixelRegion* r);
  unsigned int getFifoSize(void) { return RRU_FIFO.size(); }

  // Implement framing and stuff here.
  // Implement the 128x24b FIFO here. We can use 128 x 3 words FIFO, where we have a special object for the words,
  // similar to what Adam did. The words can be the valid "control words" for the ALPIDE data transfer, or they
  // can be hit data (which I guess is a control word, actually).

  // I think the TRU can be responsible for placing the Region Header word with the correct region number into the data stream.
  // Read out hits from the double columns, one by one, starting at column 0. Feed the data into the FIFO.
};



#endif
