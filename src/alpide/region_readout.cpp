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


RegionReadoutUnit::RegionReadoutUnit(PixelRegion* r, unsigned int fifo_size_limit, unsigned int fifo_busy_threshold) {
  region = r;
  current_region = 0;
  busy_signaled = false;

  if(fifo_busy_threshold > fifo_size_limit)
    throw("FIFO Busy threshold higher than FIFO size limit");

  fifo_size_busy_thr = fifo_busy_threshold;
  fifo_size = fifo_size_limit;
  if(fifo_size_limit == 0)
    fifo_size_limit_en = true;
  else
    fifo_size_limit_en = false;

  //@todo Throw an exception here maybe?
  if(r == NULL) {
    std::cout << "Error. Pixel row address > number of cols. Hit ignored." << std::endl
  }
}

void RegionReadoutUnit::updateFifo(void) {
  DataWordBase dw = (DataWordBase) DataWordNoData;

  // With busy signaling enabled and FIFO size limit enabled
  if(fifo_size_limit_en) {
    // If FIFO is full, don't put anything on FIFO
    if(fifo_size_limit_en && (RRU_FIFO.size() >= fifo_size_limit)) 
      ;

  // Put BUSY_ON on the FIFO if we just got above busy threshold
    else if (RRU_FIFO.size() >= fifo_size_busy_thr) {
      if(busy_signaled == false) {
        busy_signaled = true;
        dw = (DataWordBase) DataWordBusyOn;
      } else {
        dw = getNextFifoWord();
      }
    }

  // Put BUSY_OFF on the FIFO if we just got below busy threshold
    else if(RRU_FIFO.size() >= fifo_size_busy_thr < fifo_size_busy_thr) {
      if(busy_signaled == true) {
        busy_signaled = false;
        dw = (DataWordBase) DataWordBusyOff;
      } else {
        dw = getNextFifoWord();
      }
    }
  }

  // Without busy signaling and no FIFO size limit - put words on FIFO regardless of current size
  else {
    DataWordBase dw = getNextFifoWord();
  }

  // Put data on FIFO (unless it's the NO_DATA "empty data word")
  if(dw.data_word != NO_DATA)
    RRU_FIFO.push(dw);      
}

DataWordBase RegionReadoutUnit::getFifoWord(void) {
  PixelData data = NoPixelHit;
  
  if(region->pixelHitsRemaining() > 0) {
    return DataWordShort(data);
  }

  return DataWordNoData();
}
