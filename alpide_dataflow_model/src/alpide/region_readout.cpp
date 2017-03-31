/**
 * @file   region_readout.cpp
 * @author Simon Voigt Nesbo
 * @date   February 20, 2017
 * @brief  Class for implementing the Region Readout Unit (RRU) in the Alpide chip.
 *
 */

#include <iostream>
#include "region_readout.h"
#include "../misc/vcd_trace.h"

///@brief Constructor for RegionReadoutUnit class
///@param name SystemC module name
///@param region_num The region number that this RRU is assigned to
///@param fifo_size  Size limit on the RRU's FIFO. 0 for no limit.
///@param cluster_enable Enable/disable clustering and use of DATA LONG data words
RegionReadoutUnit::RegionReadoutUnit(sc_core::sc_module_name name,
                                     unsigned int region_num,
                                     unsigned int fifo_size,
                                     bool cluster_enable)
  : sc_core::sc_module(name)
  , s_region_fifo(fifo_size)
  , mRegionId(region_num)
  , mFifoSizeLimit(fifo_size)
  , mClusteringEnabled(cluster_enable)
{
  s_region_fifo_out(s_region_fifo);
  mFifoSizeLimitEnabled = (mFifoSizeLimit > 0);
  mClusterStarted = false;
}

///@brief SystemC process/method that implements the Region Readout Unit
///       state machine for reading out hits from Multi Event Buffer in
///       the pixel matrix.
///       NOTE: Should run at priority encoder clock frequency.
void RegionReadoutUnit::regionMatrixReadoutProcess(void)
{
  AlpideDataWord data_out;
  
  switch(s_rru_readout_state) {
  case IDLE:
    if(s_frame_readout_start)
      s_rru_readout_state = START_READOUT;
    break;
    
  case START_READOUT:
    if(s_region_fifo_out->num_free() > 0) {
      // Put REGION_HEADER word on RRU FIFO
      // NOTE: REGION_HEADER is not put on RRU FIFO.....
      ////////////data_out = AlpideRegionHeader(mRegionId);
      s_region_fifo_out->nb_write(data_out);
      
      if(s_readout_abort) {
        ///@todo Implement abort handling
        s_rru_readout_state = IDLE;
      } else {
        s_rru_readout_state = READOUT_AND_CLUSTERING;
      }
    }
    break;
    
  case READOUT_AND_CLUSTERING:
    if(s_region_fifo_out->num_free() > 0) {
      readoutNextPixel(matrix, time_now);
    }

    if(s_readout_abort)
      ///@todo Implement abort handling
      s_rru_readout_state = IDLE;
    else if(s_region_empty_out)
      s_rru_readout_state = REGION_TRAILER;
    
    break;
    
  case REGION_TRAILER:
    if(s_region_fifo_out->num_free() > 0) {
      // Put REGION_TRAILER word on RRU FIFO
      data_out = AlpideRegionTrailer();
      s_rru_readout_state = REGION_IDLE;
    }    
    break;
  }
}


///@brief SystemC process/method that implements the state machine that
///       determines if the region is valid (has data this frame)
///       Note: should run on Alpide system clock frequency.
void RegionReadoutUnit::regionValidProcess(void)
{
  switch(s_rru_valid_state) {
  case IDLE:
    s_region_valid_out = false;
    
    if(s_region_event_start)
      s_rru_valid_state = EMPTY;
    break;
    
  case EMPTY:
    bool fifo_empty = s_region_fifo_out->num_available() == 0;
    s_region_valid_out = (!fifo_empty /* && Region trailer word */);

    
    if(s_readout_abort)
      s_rru_valid_state = IDLE;
    else if(s_region_fifo_out->num_available() > 0) {
      // Get next word from FIFO (peek, not read??)
      // Or should this process sort of monitor what is being read from FIFO??

      // If trailer -> go to POP

      // If not trailer -> go to VALID
    }
    break;
    
  case VALID:
    s_region_valid_out = (!fifo_empty /* && Region trailer word */);
    
    if(s_readout_abort)
      s_rru_valid_state = IDLE;
    else {
    }
    break;
    
  case POP:
    s_region_valid = false;
    
    if(s_region_event_pop_in || s_readout_abort)
      s_rru_valid_state = IDLE;
    break;
    
  default:
    s_rru_valid_state = IDLE;
    break;
  }
}


///@brief Read out the next pixel from this region's priority encoder.
///       NOTE: This function should be called from a process that runs at
///             the priority encoder readout clock.
///       The function here will look for pixel clusters and generate DATA LONG
///       words when possible if clustering is enabled, otherwise it will only
///       send DATA SHORT words. See the flowchart for a better explanation of
///       how this function works.
///@image html RRU_pixel_readout.png
///@param matrix Reference to pixel matrix
///@param time_now Current simulation time
void RegionReadoutUnit::readoutNextPixel(PixelMatrix& matrix, uint64_t time_now)
{
  s_region_fifo_size = s_region_fifo.num_available();

  PixelData p = matrix.readPixelRegion(mRegionId, time_now);

  if(mClusteringEnabled) {
    if(mClusterStarted == false) {
      if(p == NoPixelHit)
        s_region_empty_out.write(true);
      else {
        mClusterStarted = true;
        mPixelHitEncoderId = p.getPriEncNumInRegion();
        mPixelHitBaseAddr = p.getPriEncPixelAddress();
        mPixelHitmap = 0;
        s_region_empty_out.write(false);
      }
    } else { // Cluster already started
      if(p == NoPixelHit) { // Indicates that we already read out all pixels from this region          
        if(mPixelHitmap == 0)
          s_region_fifo_out->nb_write(AlpideDataShort(mPixelHitEncoderId, mPixelHitBaseAddr));
        else
          s_region_fifo_out->nb_write(AlpideDataLong(mPixelHitEncoderId, mPixelHitBaseAddr, mPixelHitmap));
          
        mClusterStarted = false;
        s_region_empty_out.write(true);          
      }
      // Is the pixel within cluster that was started by a pixel that was read out previously?
      else if(p.getPriEncNumInRegion() == mPixelHitEncoderId &&
              p.getPriEncPixelAddress() <= (mPixelHitBaseAddr+DATA_LONG_PIXMAP_SIZE)) {
        // Calculate its location in the pixel map argument used in DATA LONG
        unsigned int hitmap_pixel_num = (p.getPriEncPixelAddress() - mPixelHitBaseAddr - 1);
        mPixelHitmap |= 1 << hitmap_pixel_num;

        // Last pixel in cluster?
        if(hitmap_pixel_num == DATA_LONG_PIXMAP_SIZE-1) {
          s_region_fifo_out->nb_write(AlpideDataLong(mPixelHitEncoderId, mPixelHitBaseAddr, mPixelHitmap));
          mClusterStarted = false;
        }
        s_region_empty_out.write(false);
      } else { // New pixel not in same cluster as previous pixels
        if(mPixelHitmap == 0)
          s_region_fifo_out->nb_write(AlpideDataShort(mPixelHitEncoderId, mPixelHitBaseAddr));
        else
          s_region_fifo_out->nb_write(AlpideDataLong(mPixelHitEncoderId, mPixelHitBaseAddr, mPixelHitmap));

        // Get base address, priority encoder number, etc. for the new pixel (cluster)
        mClusterStarted = true;
        mPixelHitEncoderId = p.getPriEncNumInRegion();
        mPixelHitBaseAddr = p.getPriEncPixelAddress();
        mPixelHitmap = 0;
        s_region_empty_out.write(false);          
      }
    }
  } else { // Clustering not enabled
    if(p == NoPixelHit) {
      s_region_empty_out.write(true);
    } else {
      unsigned int encoder_id = p.getPriEncNumInRegion();        
      unsigned int base_addr = p.getPriEncPixelAddress();
      s_region_fifo_out->nb_write(AlpideDataShort(encoder_id, base_addr));
      s_region_empty_out.write(false);        
    }
  }
}


///@brief Add SystemC signals to log in VCD trace file.
///@param wf Pointer to VCD trace file object
///@param name_prefix Name prefix to be added to all the trace names
void RegionReadoutUnit::addTraces(sc_trace_file *wf, std::string name_prefix) const
{
  std::stringstream ss;
  ss << name_prefix << "RRU_" << mRegionId << ".";
  std::string region_name_prefix = ss.str();

  addTrace(wf, region_name_prefix, "region_empty_out", s_region_empty_out);
  addTrace(wf, region_name_prefix, "region_fifo_size", s_region_fifo_size);  
}

