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
  RegionReadoutUnit mRRU[N_REGIONS];
  sc_core::sc_fifo<DataWordBase> mRRU_FIFO;

  unsigned int mCurrentRegion;
  
  DataWordBase getNextFifoWord(void);
  
public:
  TopReadoutUnit();


}



#endif
