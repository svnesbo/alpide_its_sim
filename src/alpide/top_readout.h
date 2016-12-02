/**
 * @file   top_readout.h
 * @Author Simon Voigt Nesbo
 * @date   November 29, 2016
 * @brief  Class for implementing the Top Readout Unit (TRU) in the Alpide chip.
 *
 * Detailed description of file.
 */

#ifndef TOP_READOUT_H
#define TOP_READOUT_H

#include "region_readout.h"


class TopReadoutUnit
{
private:
  RegionReadoutUnit RRU[N_REGIONS];
  sc_core::sc_fifo<DataWordBase> RRU_FIFO;

  unsigned int current_region;
  
  DataWordBase getNextFifoWord(void);
  
public:
  TopReadoutUnit();


}



#endif
