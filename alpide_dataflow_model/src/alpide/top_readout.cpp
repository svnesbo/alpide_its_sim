/**
 * @file   top_readout.cpp
 * @Author Simon Voigt Nesbo
 * @date   February 20, 2017
 * @brief  Class for implementing the Top Readout Unit (TRU) in the Alpide chip.
 */


#include "top_readout.h"




void TopReadoutUnit::topRegionReadoutProcess(void)
{
  // Find next region to read out, if we are done with current region
  while(s_region_empty[mCurrentRegion]) {
    mCurrentRegion++;
    if(mCurrentRegion == N_REGIONS) {
      ; // READOUT DONE!
      // Set some signal indicating that readout is done..
    }
  }

  AlpideDataWord data;
  if(RRU_FIFO[mCurrentRegion].read_nb(data)) {
    // Data successfully read out, process here
    // Put into TRU's output FIFO
    // Then add a DMU interface class, which depending on
    // parallel or serial output will strip of IDLE's etc.
    // ONLY IMPLEMENT SERIAL FOR NOW
  }
}




///@todo Old stuff, remove?

TopReadoutUnit::TopReadoutUnit()
{
  current_region = 0;
}


TopReadoutUnit::getNextFifoWord(void)
{
  // Find the next RRU FIFO that has data
  for(int i = 0; i < N_REGIONS; i++) {
    if(RRU[current_region].getFifoSize() > 0)
      break;
    
    current_region++;
    if(current_region >= N_REGIONS)
      current_region=0;
  }

  
}
