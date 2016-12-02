/**
 * @file   top_readout.cpp
 * @Author Simon Voigt Nesbo
 * @date   November 29, 2016
 * @brief  Class for implementing the Top Readout Unit (TRU) in the Alpide chip.
 *
 * Detailed description of file.
 */


#include "top_readout.h"

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
