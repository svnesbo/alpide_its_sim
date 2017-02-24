/**
 * @file   top_readout.cpp
 * @Author Simon Voigt Nesbo
 * @date   February 20, 2017
 * @brief  Class for implementing the Top Readout Unit (TRU) in the Alpide chip.
 */


#include "top_readout.h"


TopReadoutUnit::TopReadoutUnit()
  : mCurrentRegion(0)
  , mTRUState(IDLE)
{
  SC_METHOD(topRegionReadoutProcess);
  sensitive_pos << s_clk_in;  
}


///@brief SystemC method that controls readout from regions, should run on the 40MHz clock.
///       The regions are read out in ascending order, and each event is encapsulated with
///       a CHIP_HEADER and CHIP_TRAILER word. 
void TopReadoutUnit::topRegionReadoutProcess(void)
{ 
  if(s_tru_fifo_out.num_free() > 0) {
    switch(mTRUState) {
    case CHIP_HEADER:
      s_tru_fifo_out.write(AlpideChipHeader(mChipId, mBunchCounter));
      if(getHitsRemainingInOldestEvent() == 0)
        mTRUState = CHIP_EMPTY_FRAME;
      else {
        mCurrentRegion = 0;
        mTRUState = REGION_HEADER;
      }
      break;
      
    case CHIP_EMPTY_FRAME:
      s_tru_fifo_out.write(AlpideChipEmptyFrame(mChipId, mBunchCounter));
      mTRUState = CHIP_TRAILER;
      break;
      
    case REGION_HEADER:
      // Find the next region that has data
      while(s_region_empty[mCurrentRegion].read() == false &&
            s_region_fifo_in[mCurrentRegion].num_available() == 0 &&
            mCurrentRegion < N_REGIONS)
      {
        mCurrentRegion++;
      }

      
      if(mCurrentRegion < N_REGIONS) {
        mTRUState = REGION_DATA;        
        s_tru_fifo_out.write(AlpideRegionHeader(mCurrentRegion));
        break;
      } else {
        // No break here - Allow program to continue into
        // CHIP_TRAILER state if we have read out all regions
        mTRUState = CHIP_TRAILER;
      }

    case CHIP_TRAILER:
      ///@todo Implement some readout flags here?
      int readout_flags = 0;
      s_tru_fifo_out.write(AlpideChipTrailer(readout_flags));

      if(getNumEvents() > 0)
        mTRUState = CHIP_HEADER;
      else
        mTRUState = IDLE;
      break;      

    case REGION_DATA:
      if(s_region_fifo_in[mCurrentRegion].num_available() > 0) {
        s_tru_fifo_out.write(s_region_fifo_in[mCurrentRegion].read_nb());
      } else {
        // Insert IDLE if the region FIFO is currently empty
        s_tru_fifo_out.write(AlpideIdle());
      }

      // Is the region empty, and was this the last word in the region FIFO?
      if(s_region_empty[mCurrentRegion].read() == false &&
            s_region_fifo_in[mCurrentRegion].num_available() == 0)
      {
        if(mCurrentRegion == (N_REGIONS-1))
          mTRUState = CHIP_TRAILER;
        else
          mTRUState = REGION_HEADER;
      }
      break;
      
    case IDLE:
      s_tru_fifo_out.write(AlpideIdle());

      if(getNumEvents() > 0)
        mTRUState = CHIP_HEADER;
      
      break;
      
    default:
      mTRUState = IDLE;
      break;
    }
    
  } else { // TRU FIFO Full
    // Do something smart here.. do we need to signal busy?
  }
}
