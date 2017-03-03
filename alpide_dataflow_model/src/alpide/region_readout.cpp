/**
 * @file   region_readout.cpp
 * @author Simon Voigt Nesbo
 * @date   February 20, 2017
 * @brief  Class for implementing the Region Readout Unit (RRU) in the Alpide chip.
 *
 */

#include <iostream>
#include "region_readout.h"

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


///@brief Read out the next pixel from this region's priority encoder.
///       This function should be called from a process that runs at
///       the priority encoder readout clock.
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

  // See the flowchart mentioned in this function's brief for a
  // better understanding of how this is implemented
  if(s_region_fifo_out->num_free() > 0) {
    s_busy_out.write(false);
    
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
        else if(p.getPriEncPixelAddress() <= (mPixelHitBaseAddr+DATA_LONG_PIXMAP_SIZE)) {
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
          
          mClusterStarted = true;
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
  } else { // FIFO is full
    s_busy_out.write(true);
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

  ss.str("");
  ss << region_name_prefix << "region_empty_out";
  std::string str_region_empty_out(ss.str());
  sc_trace(wf, s_region_empty_out, str_region_empty_out);
  
  ss.str("");
  ss << region_name_prefix << "busy_out";
  std::string str_busy_out(ss.str());
  sc_trace(wf, s_busy_out, str_busy_out);

  ss.str("");
  ss << region_name_prefix << "region_fifo_size";
  std::string str_region_fifo_size(ss.str());
  sc_trace(wf, s_region_fifo_size, str_region_fifo_size);  
}
