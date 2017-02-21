/**
 * @file   region_readout.cpp
 * @Author Simon Voigt Nesbo
 * @date   February 20, 2017
 * @brief  Class for implementing the Region Readout Unit (RRU) in the Alpide chip.
 *
 */

#include <iostream>
#include "region_readout.h"

///@brief Constructor for RegionReadoutUnit class
///@param region_num The region number that this RRU is assigned to
///@param fifo_size  Size limit on the RRU's FIFO
///@param cluster_enable Enable/disable clustering and use of DATA LONG data words
RegionReadoutUnit::RegionReadoutUnit(unsigned int region_num,
                                     unsigned int fifo_size,
                                     bool cluster_enable)
  : mRegion(region_num)
  , mFifoSizeLimit(fifo_size)
  , mClusteringEnabled(cluster_enable)
{
  mFifoSizeLimitEnabled = (mFifoSizeLimit > 0);
}


///@brief Read out the next pixel from this region's priority encoder.
///       This function should be called from a process that runs at
///       the priority encoder readout clock.
///@param matrix Reference to pixel matrix
///@param time_now Current simulation time
void RegionReadoutUnit::readoutNextPixel(PixelMatrix& matrix, uint64_t time_now)
{
  s_fifo_size.write(RRU_FIFO.num_available());
  if(RRU_FIFO.num_available() == 0)
    s_fifo_empty.write(true);
  else
    s_fifo_empty.write(false);

  
  if(RRU_FIFO.num_free() > 0) {
    s_busy_out.write(false);
    
    PixelData p = readPixelRegion(region_num, time_now);

    if(mClusteringEnabled) {
      if(mClusterStarted) {
        if(p == NoPixelHit) { // Indicates that we already read out all pixels from this region          
          if(mPixelHitmap == 0)
            RRU_FIFO.write_nb(AlpideDataShort(mEncoderId, mPixelHitBaseAddr));
          else
            RRU_FIFO.write_nb(AlpideDataLong(mEncoderId, mPixelHitBaseAddr, mPixelHitmap));
          
          mClusterStarted = false;
          s_region_empty.write(true);          
        }
        // Is the pixel within cluster that was started by a pixel that was read out previously?
        else if(p.getPriEncPixelAddress() <= mPixelHitBaseAddr+DATA_LONG_PIXMAP_SIZE) {          
          // Calculate its location in the pixel map argument used in DATA LONG
          unsigned int hitmap_pixel_num = (p.getPriEncPixelAddress() - mPixelHitBaseAddr - 1);
          mPixelHitmap |= 1 << hitmap_pixel_num;

          // Last pixel in cluster?
          if(hitmap_pixel_num == DATA_LONG_PIXMAP_SIZE-1) {
            RRU_FIFO.write_nb(AlpideDataLong(mEncoderId, mPixelHitBaseAddr, mPixelHitmap));
            mClusterStarted = false;
          }
          s_region_empty.write(false);
        } else { // New pixel not in same cluster as previous pixels
          if(mPixelHitmap == 0)
            RRU_FIFO.write_nb(AlpideDataShort(mEncoderId, mPixelHitBaseAddr));
          else
            RRU_FIFO.write_nb(AlpideDataLong(mEncoderId, mPixelHitBaseAddr, mPixelHitmap));
          
          mClusterStarted = true;
          mPixelHitBaseAddr = p.getPriEncPixelAddress();
          mPixelHitmap = 0;
          s_region_empty.write(false);          
        }
      } else { // Cluster not started
        if(p == NoPixelHit)
          s_region_empty.write(true);
        else {
          mClusterStarted = true;
          mPixelHitBaseAddr = p.getPriEncPixelAddress();
          mPixelHitmap = 0;
          s_region_empty.write(false);
        }
      }      
    } else { // Clustering not enabled
      if(p == NoPixelHit) {
        s_region_empty.write(true);
      } else {
        unsigned int addr = p.getPriEncPixelAddress();
        ///@todo EXTRACT ENCODER ID AND ADDRESS AND FILL IN FUNCTION HERE
        RRU_FIFO.write_nb(AlpideDataShort(encoder_id, base_addr));
        s_region_empty.write(false);        
      }
    }
  } else { // FIFO is full
    s_busy_out.write(true);
  }
}








// Old stuff

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
