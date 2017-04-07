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
                                     bool matrix_readout_speed,
                                     bool cluster_enable)
  : sc_core::sc_module(name)
  , s_region_fifo(fifo_size)
  , mRegionId(region_num)
  , mFifoSizeLimit(fifo_size)
  , mMatrixReadoutSpeed(matrix_readout_speed)
  , mClusteringEnabled(cluster_enable)
{
  s_region_fifo_in(s_region_fifo);
  s_region_fifo_out(s_region_fifo);
  
  mFifoSizeLimitEnabled = (mFifoSizeLimit > 0);
  mClusterStarted = false;

  SC_METHOD(regionReadoutProcess);
  sensitive_pos << s_system_clk_in;  
}

///@brief SystemC process/method that implements the logic in the
///       Region Readout Unit (RRU). NOTE: Should run at system clock frequency (40MHz).
void RegionReadoutUnit::regionReadoutProcess(void)
{
  AlpideDataWord data_out;

  // Is TRU requesting to read out next word from FIFO?
  if(s_region_data_read_in) {
    s_region_fifo_in->nb_read(data_out);
    s_region_data_out = data_out;
  }

  s_region_fifo_empty_out = s_region_fifo_in->is_empty();

  matrixReadoutFSM();
  regionValidFSM();

  // Update region data output
  if(s_region_data_read_in || s_region_event_pop_in) {
    if(s_region_fifo_out.nb_get(data_out)
       s_region_data_out = data_out;
  }
}


void RegionReadoutUnit::matrixReadoutFSM(void)
{
  bool region_fifo_full = s_region_fifo_in->is_full();
  bool matrix_readout_ready = false;

  if(mMatrixReadoutSpeed && s_matrix_readout_delay_counter > 0)
    matrix_readout_ready = true;
  else if(!mMatrixReadoutSpeed && s_matrix_readout_delay_counter >= 2)
    matrix_readout_ready = true;

  
  switch(s_rru_readout_state) {
  case IDLE:
    if(s_frame_readout_start && !s_region_matrix_empty) {
      s_matrix_readout_delay_counter = 0;
      s_rru_readout_state = START_READOUT;
    } else if(s_frame_readout_start && s_region_matrix_empty)
      s_rru_readout_state = REGION_TRAILER;
    else
      s_rru_readout_state = IDLE;
    break;
    
  case START_READOUT:
    if(s_readout_abort)
      s_rru_readout_state = IDLE;
    else if(matrix_readout_ready) // Wait for matrix readout delay
      s_rru_readout_state = READOUT_AND_CLUSTERING;
    else
      s_matrix_readout_delay_counter++;
    
    break;
    
  case READOUT_AND_CLUSTERING:
    if(s_readout_abort)
      s_rru_readout_state = IDLE;
    else if(matrix_readout_ready) { // Wait for matrix readout delay
      if(!s_region_matrix_empty) {
        if(!region_fifo_full) {
          readoutNextPixel(matrix, time_now);          
          s_matrix_delay_counter = 0;
        }
      } else {
        s_rru_readout_state = REGION_TRAILER;
      }
    } else {
      s_matrix_readout_delay_counter++;
    }
    break;
    
  case REGION_TRAILER:
    if(s_readout_abort)
      s_rru_readout_state = IDLE;
    else if(!region_fifo_full) {
      // Put REGION_TRAILER word on RRU FIFO
      s_region_fifo_in.nb_put(AlpideRegionTrailer(););
      s_rru_readout_state = IDLE;
    }    
    break;
  }
}

///@brief SystemC process/method that implements the state machine that
///       determines if the region is valid (has data this frame)
///       Note: should run on Alpide system clock frequency.
void RegionReadoutUnit::regionValidFSM(void)
{
  AlpideDataWord dw;
  bool region_fifo_empty = s_region_fifo_out->is_empty();
  bool region_data_is_trailer = false;

  if(!region_fifo_empty) {
    s_region_fifo_out->nb_peek(dw);
    if(dw.data[0] == DW_REGION_TRAILER)
      region_data_is_trailer = true;
  }
  
  switch(s_rru_valid_state) {
  case IDLE:
    s_region_valid_out = false;
    
    if(s_region_event_start && !readout_abort)
      s_rru_valid_state = EMPTY;
    break;
    
  case EMPTY:
    s_region_valid_out = (!region_fifo_empty && region_data_is_trailer);
    
    if(s_readout_abort)
      s_rru_valid_state = IDLE;
    else if(!region_fifo_empty && region_data_is_trailer)
      s_rru_valid_state = POP;
    else if(region_fifo_empty && !region_data_is_trailer)
      s_rru_valid_state = VALID;
    }
    break;
    
  case VALID:
    s_region_valid_out = (!fifo_empty && region_data_is_trailer);
    
    if(s_readout_abort)
      s_rru_valid_state = IDLE;
    else if(region_data_is_trailer) {
      s_rru_valid_state = POP;
    }
    break;
    
  case POP:
    s_region_valid_out = false;
    
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
        s_region_matrix_empty.write(true);
      else {
        mClusterStarted = true;
        mPixelHitEncoderId = p.getPriEncNumInRegion();
        mPixelHitBaseAddr = p.getPriEncPixelAddress();
        mPixelHitmap = 0;
        s_region_matrix_empty.write(false);
      }
    } else { // Cluster already started
      if(p == NoPixelHit) { // Indicates that we already read out all pixels from this region          
        if(mPixelHitmap == 0)
          s_region_fifo_in->nb_put(AlpideDataShort(mPixelHitEncoderId, mPixelHitBaseAddr));
        else
          s_region_fifo_in->nb_put(AlpideDataLong(mPixelHitEncoderId, mPixelHitBaseAddr, mPixelHitmap));
          
        mClusterStarted = false;
        s_region_matrix_empty.write(true);          
      }
      // Is the pixel within cluster that was started by a pixel that was read out previously?
      else if(p.getPriEncNumInRegion() == mPixelHitEncoderId &&
              p.getPriEncPixelAddress() <= (mPixelHitBaseAddr+DATA_LONG_PIXMAP_SIZE)) {
        // Calculate its location in the pixel map argument used in DATA LONG
        unsigned int hitmap_pixel_num = (p.getPriEncPixelAddress() - mPixelHitBaseAddr - 1);
        mPixelHitmap |= 1 << hitmap_pixel_num;

        // Last pixel in cluster?
        if(hitmap_pixel_num == DATA_LONG_PIXMAP_SIZE-1) {
          s_region_fifo_in->nb_put(AlpideDataLong(mPixelHitEncoderId, mPixelHitBaseAddr, mPixelHitmap));
          mClusterStarted = false;
        }
        s_region_matrix_empty.write(false);
      } else { // New pixel not in same cluster as previous pixels
        if(mPixelHitmap == 0)
          s_region_fifo_in->nb_put(AlpideDataShort(mPixelHitEncoderId, mPixelHitBaseAddr));
        else
          s_region_fifo_in->nb_put(AlpideDataLong(mPixelHitEncoderId, mPixelHitBaseAddr, mPixelHitmap));

        // Get base address, priority encoder number, etc. for the new pixel (cluster)
        mClusterStarted = true;
        mPixelHitEncoderId = p.getPriEncNumInRegion();
        mPixelHitBaseAddr = p.getPriEncPixelAddress();
        mPixelHitmap = 0;
        s_region_matrix_empty.write(false);          
      }
    }
  } else { // Clustering not enabled
    if(p == NoPixelHit) {
      s_region_matrix_empty.write(true);
    } else {
      unsigned int encoder_id = p.getPriEncNumInRegion();        
      unsigned int base_addr = p.getPriEncPixelAddress();
      s_region_fifo_in->nb_put(AlpideDataShort(encoder_id, base_addr));
      s_region_matrix_empty.write(false);        
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
  addTrace(wf, region_name_prefix, "region_matrix_empty", s_region_matrix_empty);  
  addTrace(wf, region_name_prefix, "region_fifo_size", s_region_fifo_size);  
}

