/**
 * @file   region_readout.cpp
 * @author Simon Voigt Nesbo
 * @date   February 20, 2017
 * @brief  Class for implementing the Region Readout Unit (RRU) in the Alpide chip.
 *
 */

#include <iostream>
#include "RegionReadoutUnit.hpp"
#include "../misc/vcd_trace.hpp"



#include "Alpide.hpp"


SC_HAS_PROCESS(RegionReadoutUnit);
///@brief Constructor for RegionReadoutUnit class
///@param[in] name SystemC module name
///@param[in] matrix Reference to pixel matrix
///@param[in] region_num The region number that this RRU is assigned to
///@param[in] fifo_size  Size limit on the RRU's FIFO. 0 for no limit.
///@param[in] matrix_readout_speed True for fast readout (2 clock cycles), false is slow (4 cycles).
///@param[in] cluster_enable Enable/disable clustering and use of DATA LONG data words
RegionReadoutUnit::RegionReadoutUnit(sc_core::sc_module_name name,
                                     PixelMatrix* matrix,
                                     unsigned int region_num,
                                     unsigned int fifo_size,
                                     bool matrix_readout_speed,
                                     bool cluster_enable)
  : sc_core::sc_module(name)
  , s_region_fifo(fifo_size)
  , mRegionHeader(region_num)
  , mRegionId(region_num)
  , mMatrixReadoutSpeed(matrix_readout_speed)
  , mIdle(false)
  , mFifoSizeLimit(fifo_size)
  , mClusteringEnabled(cluster_enable)
  , mPixelMatrix(matrix)
{
  s_rru_readout_state = RO_FSM::IDLE;
  s_rru_valid_state = VALID_FSM::IDLE;
  s_rru_header_state = HEADER_FSM::HEADER;

  mFifoSizeLimitEnabled = (mFifoSizeLimit > 0);
  mClusterStarted = false;
  s_cluster_started = false;

  SC_METHOD(regionUnitProcess);
  sensitive_pos << s_system_clk_in;

  SC_METHOD(regionHeaderFSMOutput);
  sensitive << s_rru_header_state;
}


///@brief SystemC process/method that implements the logic in the
///       Region Readout Unit (RRU). NOTE: Should run at system clock frequency (40MHz).
void RegionReadoutUnit::regionUnitProcess(void)
{
  if(mIdle) {
    // Revert to static sensitivity (clocked), and wait till next clock cycle
    // because dynamic sensitivity to signal changes triggers the method
    // before the signals would be clocked in
    next_trigger();
    mIdle = false;
    return;
  }

  updateRegionDataOut();

  s_region_fifo_size = s_region_fifo.used();
  s_region_fifo_empty_out = (s_region_fifo.used() == 0);


  mIdle =  regionMatrixReadoutFSM();
  mIdle &= regionValidFSM();
  regionHeaderFSM();


  // If the RRU is idle, use dynamic sensitivity to wake up on the
  // signals that would bring it out of idle, in order to save simulation time.
  if(mIdle) {
    next_trigger(s_readout_abort_in.value_changed_event() |
                 s_frame_readout_start_in.value_changed_event() |
                 s_region_event_start_in.value_changed_event());
  }
}


///@brief Update data output with region header, region data, or region trailer
///       Read out new data and pop region trailer when requested.
///       The region data output should have the last data word from the region fifo,
///       but it should not be read/popped from the fifo before the TRU gives the read/pop signal
void RegionReadoutUnit::updateRegionDataOut(void)
{
  // Condition for reading data from region fifo
  // The region valid condition is needed because of the 1 cycle delay from detecting REGION
  // TRAILER before s_region_valid_out actually goes low. The TRU might attempt to read out
  // the REGION TRAILER word as a normal data word, and this will prevent that.
  // AFAIK this condition is not in the ALPIDE chip (not indicated in EDR presentation slides),
  // but I don't see any other way of preventing this from happening.
  bool read_dataword = (s_region_data_read_in && s_region_valid_out);

  // Pop trailer when TRU requests it, but not in readout abort mode.
  // In RO abort mode the flushRegionFifo() function will take care of emptying the RRU FIFO
  bool pop_trailer = s_region_event_pop_in && !s_readout_abort_in;

  if((read_dataword && !s_generate_region_header) || pop_trailer) {
    AlpideDataWord data;
    s_region_fifo.nb_get(data);
    //s_region_fifo.nb_get(mRegionDataOut);

    int64_t time_now = sc_time_stamp().value();
    if(!pop_trailer && data.data[0] == DW_REGION_TRAILER) {
      std::cerr << "@" << time_now << " ns: ";
      std::cerr << "Global chip ID " << static_cast<Alpide*>(mPixelMatrix)->getGlobalChipId();
      std::cerr << ", region " << mRegionId;
      std::cerr << ": Oops read out REGION_TRAILER" << std::endl;
    } else if(pop_trailer && data.data[0] != DW_REGION_TRAILER) {
      std::cerr << "@" << time_now << " ns: ";
      std::cerr << "Global chip ID " << static_cast<Alpide*>(mPixelMatrix)->getGlobalChipId();
      std::cerr << ", region " << mRegionId;
      std::cerr << ": Oops popped something else than REGION_TRAILER" << std::endl;
    }
  }

  // Check if next data word is REGION TRAILER
  AlpideDataWord fifo_next_dw = AlpideIdle();
  //if(s_region_fifo.nb_peek(fifo_next_dw)) {
  if(s_region_fifo.nb_peek(mRegionDataOut)) {
    //if(fifo_next_dw.data[0] == DW_REGION_TRAILER)
    if(mRegionDataOut.data[0] == DW_REGION_TRAILER)
      mRegionDataOutIsTrailer = true;
    else
      mRegionDataOutIsTrailer = false;
  } else {
    mRegionDataOutIsTrailer = false;
  }

  // Update region data output
  if(s_generate_region_header && !read_dataword) {
    //if(s_generate_region_header) {
    // Output header
    s_region_data_out = mRegionHeader;
  } else {
    // Update region's data output with next data on fifo
    //s_region_data_out = fifo_next_dw;
    s_region_data_out = mRegionDataOut;
  }
}


///@brief SystemC process/method that implements the state machine that
///       controls readout from the multi event buffers into the region's FIFOs
///       Note: should run on Alpide system clock frequency.
///@return Idle state. True if FSM is in idle and will be idle the next state.
bool RegionReadoutUnit::regionMatrixReadoutFSM(void)
{
  bool matrix_readout_ready = false;
  bool region_matrix_empty = false;
  bool idle_state = false;
  std::uint8_t current_state = s_rru_readout_state.read();
  std::uint8_t next_state = current_state;

  // The Alpide has a choice of two different matrix priority encoder readout speeds:
  // 1/2 of 40MHz clock, or 1/4 of 40MHz clock
  if(mMatrixReadoutSpeed && (s_matrix_readout_delay_counter.read() > 0))
    matrix_readout_ready = true;
  else if(!mMatrixReadoutSpeed && (s_matrix_readout_delay_counter.read() >= 2))
    matrix_readout_ready = true;

  // Next state logic etc.
  switch(current_state) {
  case RO_FSM::IDLE:
    // Stay in this state and flush region fifo when in data overrun mode
    if(s_readout_abort_in) {
      flushRegionFifo();
      s_region_matrix_empty_debug = false;
      next_state = RO_FSM::IDLE;
      idle_state = true;
    }
    else if(s_frame_readout_start_in) {
      s_region_matrix_empty_debug = region_matrix_empty = mPixelMatrix->regionEmpty(mRegionId);

      // Start readout if this region has hits, otherwise just output region trailer
      if(!region_matrix_empty) {
        s_matrix_readout_delay_counter = 0;
        next_state = RO_FSM::START_READOUT;
      } else {
        next_state = RO_FSM::REGION_TRAILER;
      }
    }
    else {
      s_region_matrix_empty_debug = false;
      next_state = RO_FSM::IDLE;
      idle_state = true;
    }
    s_frame_readout_done_out = !s_frame_readout_start_in;
    break;

  case RO_FSM::START_READOUT:
    if(s_readout_abort_in)
      next_state = RO_FSM::IDLE;
    else if(matrix_readout_ready) // Wait for matrix readout delay
      next_state = RO_FSM::READOUT_AND_CLUSTERING;
    else
      s_matrix_readout_delay_counter = s_matrix_readout_delay_counter.read() + 1;

    s_frame_readout_done_out = false;
    break;

  case RO_FSM::READOUT_AND_CLUSTERING:
    if(s_readout_abort_in) {
      // Clear cluster started flag on readout abort, to prevent readoutNextPixel() from
      // continuing an old cluster after readout abort is done.
      mClusterStarted = false;
      mPixelClusterVec.clear();

      next_state = RO_FSM::IDLE;
    } else if(matrix_readout_ready) { // Wait for matrix readout delay
      //if(!region_fifo_full) {
      if(s_region_fifo.nb_can_put()) { // fifo not full?
        s_region_matrix_empty_debug = region_matrix_empty = readoutNextPixel(*mPixelMatrix);
        s_matrix_readout_delay_counter = 0;
        if(region_matrix_empty) {
          next_state = RO_FSM::REGION_TRAILER;
        }
      }
    } else {
      s_matrix_readout_delay_counter = s_matrix_readout_delay_counter.read() + 1;
    }
    s_frame_readout_done_out = false;
    break;

  case RO_FSM::REGION_TRAILER:
    if(s_readout_abort_in)
      next_state = RO_FSM::IDLE;
    //else if(!region_fifo_full) {
    else if(s_region_fifo.nb_can_put()) { // fifo not full?
      // Put REGION_TRAILER word on RRU FIFO
      //s_region_fifo.nb_put(AlpideRegionTrailer());

      if(s_region_fifo.nb_put(AlpideRegionTrailer()) == false) {
        int64_t time_now = sc_time_stamp().value();
        std::cerr << "@" << time_now << " ns: ";
        std::cerr << "Global chip ID " << static_cast<Alpide*>(mPixelMatrix)->getGlobalChipId();
        std::cerr << ", region " << mRegionId;
        std::cerr << ": Oops writing REGION_TRAILER to fifo failed" << std::endl;
      }

      next_state = RO_FSM::IDLE;
    }
    s_frame_readout_done_out = false;
    break;
  }


  // Output logic
  // Based on next state to achieve combinatorial output based on
  // the state we're in, without delaying the output with a clock cycle
  // Could alternatively have been done in a separate SystemC method,
  // sensitive to current state and s_frame_readout_start_in.
/*  switch(next_state) {
  case RO_FSM::IDLE:
    s_frame_readout_done_out = !s_frame_readout_start_in;
    break;

  case RO_FSM::START_READOUT:
    s_frame_readout_done_out = false;
    break;

  case RO_FSM::READOUT_AND_CLUSTERING:
    s_frame_readout_done_out = false;
    break;

  case RO_FSM::REGION_TRAILER:
    s_frame_readout_done_out = false;
    break;
    }*/


  s_rru_readout_state = next_state;

  return idle_state;
}


///@brief SystemC process/method that implements the state machine that
///       determines if the region is valid (has data this frame)
///       Note: should run on Alpide system clock frequency.
///@return Idle state. True if FSM is in idle and will be idle the next state.
bool RegionReadoutUnit::regionValidFSM(void)
{
// Ignore warning about uninitialized dw
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
  AlpideDataWord dw;
#pragma GCC diagnostic pop

  bool region_fifo_empty = s_region_fifo.used() == 0;
  bool idle_state = false;
  std::uint8_t current_state = s_rru_valid_state.read();
  std::uint8_t next_state = current_state;


  // Next state logic
  switch(current_state) {
  case VALID_FSM::IDLE:
    if(s_region_event_start_in && !s_readout_abort_in)
      next_state = VALID_FSM::EMPTY;
    else
      idle_state = true;

    s_region_valid_out = false;
    break;

  case VALID_FSM::EMPTY:
    if(s_readout_abort_in)
      next_state = VALID_FSM::IDLE;
    else if(!region_fifo_empty && mRegionDataOutIsTrailer)
      next_state = VALID_FSM::POP;
    else if(!region_fifo_empty && !mRegionDataOutIsTrailer)
      next_state = VALID_FSM::VALID;

    s_region_valid_out = ((!region_fifo_empty || s_cluster_started || s_rru_readout_state.read() == RO_FSM::READOUT_AND_CLUSTERING || s_rru_readout_state.read() == RO_FSM::START_READOUT) && !mRegionDataOutIsTrailer);
    break;

  case VALID_FSM::VALID:
    if(s_readout_abort_in)
      next_state = VALID_FSM::IDLE;
    else if(mRegionDataOutIsTrailer) {
      next_state = VALID_FSM::POP;
    }

    s_region_valid_out = !mRegionDataOutIsTrailer;
    break;

  case VALID_FSM::POP:
    if(s_region_event_pop_in || s_readout_abort_in)
      next_state = VALID_FSM::IDLE;

    s_region_valid_out = false;
    break;

  default:
    next_state = VALID_FSM::IDLE;
    break;
  }


  // Output logic
  // Based on next state to achieve combinatorial output based on
  // the state we're in, without delaying the output with a clock cycle
  /* switch(next_state) { */
  /* case VALID_FSM::IDLE: */
  /*   s_region_valid_out = false; */
  /*   break; */

  /* case VALID_FSM::EMPTY: */
  /*   // Slight modification from how valid signal is determined in the Valid FSM diagrams */
  /*   // in the Alpide Chip EDR presentation, the clustering may take some time and we */
  /*   // need to get the valid signal fast enough to prevent the TRU to going to the TRAILER */
  /*   // state too soon because it thinks no regions are valid... */
  /*   s_region_valid_out = ((!region_fifo_empty || s_cluster_started || s_rru_readout_state.read() == RO_FSM::READOUT_AND_CLUSTERING || s_rru_readout_state.read() == RO_FSM::START_READOUT) && !mRegionDataOutIsTrailer); */
  /*   break; */

  /* case VALID_FSM::VALID: */
  /*   s_region_valid_out = !mRegionDataOutIsTrailer; */
  /*   break; */

  /* case VALID_FSM::POP: */
  /*   s_region_valid_out = false; */
  /*   break; */

  /* default: */
  /*   break; */
  /* } */

  s_rru_valid_state = next_state;

  return idle_state;
}


///@brief SystemC process/method that implements the state machine that
///       determines when the region header should be outputted
///       Note: should run on Alpide system clock frequency.
void RegionReadoutUnit::regionHeaderFSM(void)
{
  std::uint8_t current_state = s_rru_header_state.read();
  std::uint8_t next_state = current_state;

  // Next state logic
  switch(current_state) {
  case HEADER_FSM::HEADER:
    if(!s_readout_abort_in && s_region_data_read_in)
      next_state = HEADER_FSM::DATA;

    //s_generate_region_header = true;
    break;

  case HEADER_FSM::DATA:
    if(s_readout_abort_in || s_region_event_pop_in)
      next_state = HEADER_FSM::HEADER;

    //s_generate_region_header = false;
    break;

  default:
    next_state = HEADER_FSM::HEADER;
    //s_generate_region_header = true;
    break;
  }


  // Output logic
  // Based on next state to achieve combinatorial output based on
  // the state we're in, without delaying the output with a clock cycle
  // Technically could have been implemented in a separate SystemC method
  // that is sensitive to current state.
  /* switch(next_state) { */
  /* case HEADER_FSM::HEADER: */
  /*   s_generate_region_header = true; */
  /*   break; */

  /* case HEADER_FSM::DATA: */
  /*   s_generate_region_header = false; */
  /*   break; */

  /* default: */
  /*   s_generate_region_header = true; */
  /*   break; */
  /* } */

  s_rru_header_state = next_state;
}


///@brief SystemC method to generate s_generate_region_header signal
///       Moore FSM style combinatorial output from region header FSM
void RegionReadoutUnit::regionHeaderFSMOutput(void)
{
  // Next state logic
  switch(s_rru_header_state.read()) {
  case HEADER_FSM::HEADER:
    s_generate_region_header = true;
    break;

  case HEADER_FSM::DATA:
    s_generate_region_header = false;
    break;

  default:
    s_generate_region_header = true;
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
///@image latex RRU_pixel_readout.png "Flowchart for pixel readout and clustering in readoutNextPixel()"
///@param[in] matrix Reference to pixel matrix
///@return False if matrix is empty and no pixel was read out
bool RegionReadoutUnit::readoutNextPixel(PixelMatrix& matrix)
{
  bool region_matrix_empty = false;
  int64_t time_now = sc_time_stamp().value();


  std::shared_ptr<PixelHit> p = matrix.readPixelRegion(mRegionId, time_now);

#ifdef EXCEPTION_CHECKS
  if(*p == NoPixelHit && matrix.regionEmpty(mRegionId) == false)
    throw std::runtime_error(std::string("Region: ") +
                             std::to_string(mRegionId) +
                             std::string("Got NoPixelHit but region not empty."));
#endif

#ifdef PIXEL_DEBUG
  if(*p != NoPixelHit) {
    p->mRRU = true;
    p->mRRUTime = time_now;
  }
#endif

  if(mClusteringEnabled) {
    if(mClusterStarted == false) {
      if(*p == NoPixelHit)
        region_matrix_empty = true;
      else {
        mClusterStarted = true;
        mPixelHitEncoderId = p->getPriEncNumInRegion();
        mPixelHitBaseAddr = p->getPriEncPixelAddress();
        mPixelHitmap = 0;
        mPixelClusterVec.clear();
        mPixelClusterVec.push_back(p);
        region_matrix_empty = false;
      }
    } else { // Cluster already started
      if(*p == NoPixelHit) {
        // No more hits? That means we have read out all pixels from this region,
        // and can transmit the current cluster
        if(mPixelHitmap == 0)
          s_region_fifo.nb_put(AlpideDataShort(mPixelHitEncoderId, mPixelHitBaseAddr,
                                               mPixelClusterVec[0]));
        else
          s_region_fifo.nb_put(AlpideDataLong(mPixelHitEncoderId, mPixelHitBaseAddr,
                                              mPixelHitmap, mPixelClusterVec));

        mPixelClusterVec.clear();
        mClusterStarted = false;
        region_matrix_empty = true;
      }
      // Is this pixel within the current cluster?
      else if(p->getPriEncNumInRegion() == mPixelHitEncoderId &&
              p->getPriEncPixelAddress() <= (mPixelHitBaseAddr+DATA_LONG_PIXMAP_SIZE)) {
        // Calculate its location in the pixel map argument used in DATA LONG
        unsigned int hitmap_pixel_num = (p->getPriEncPixelAddress() - mPixelHitBaseAddr) - 1;
        mPixelHitmap |= 1 << hitmap_pixel_num;

        mPixelClusterVec.push_back(p);

        // Transmit cluster if this was the last pixel in cluster
        if(hitmap_pixel_num == DATA_LONG_PIXMAP_SIZE-1) {
          s_region_fifo.nb_put(AlpideDataLong(mPixelHitEncoderId, mPixelHitBaseAddr,
                                              mPixelHitmap, mPixelClusterVec));

          mPixelClusterVec.clear();
          mClusterStarted = false;
        }
        region_matrix_empty = false;
      } else { // New pixel not in same cluster as previous pixels?
        // First send out DATA_SHORT or DATA_LONG for pixel(s) that were already read out..
        if(mPixelHitmap == 0)
          s_region_fifo.nb_put(AlpideDataShort(mPixelHitEncoderId, mPixelHitBaseAddr,
                                               mPixelClusterVec[0]));
        else
          s_region_fifo.nb_put(AlpideDataLong(mPixelHitEncoderId, mPixelHitBaseAddr,
                                              mPixelHitmap, mPixelClusterVec));

        // ..then start a new cluster.

        // Get base address, priority encoder number, etc. for the new pixel (cluster)
        mPixelClusterVec.clear();
        mPixelClusterVec.push_back(p);
        mClusterStarted = true;
        mPixelHitEncoderId = p->getPriEncNumInRegion();
        mPixelHitBaseAddr = p->getPriEncPixelAddress();
        mPixelHitmap = 0;
        region_matrix_empty = false;
      }
    }
  } else { // Clustering not enabled
    if(*p == NoPixelHit) {
      region_matrix_empty = true;
    } else {
      // Transmit DATA_SHORT with current pixel directly when clustering is disabled
      unsigned int encoder_id = p->getPriEncNumInRegion();
      unsigned int base_addr = p->getPriEncPixelAddress();
      s_region_fifo.nb_put(AlpideDataShort(encoder_id, base_addr, p));
      region_matrix_empty = false;
    }
  }

  // Delayed signal version of cluster started, needed by valid FSM.
  s_cluster_started = mClusterStarted;

  return region_matrix_empty;
}


///@brief Flush the region fifo. Used in data overrun mode. The function assumes that
///       the fifo can be flushed in one clock cycle.
void RegionReadoutUnit::flushRegionFifo(void)
{
  AlpideDataWord data;

  while(s_region_fifo.used() > 0) {
    s_region_fifo.nb_get(data);
  }
}


///@brief Add SystemC signals to log in VCD trace file.
///@param[in,out] wf Pointer to VCD trace file object
///@param[in] name_prefix Name prefix to be added to all the trace names
void RegionReadoutUnit::addTraces(sc_trace_file *wf, std::string name_prefix) const
{
  std::stringstream ss;
  ss << name_prefix << "RRU_" << mRegionId << ".";
  std::string region_name_prefix = ss.str();

  addTrace(wf, region_name_prefix, "region_matrix_empty_debug", s_region_matrix_empty_debug);
  addTrace(wf, region_name_prefix, "region_fifo_size", s_region_fifo_size);
  addTrace(wf, region_name_prefix, "frame_readout_start_in", s_frame_readout_start_in);
  addTrace(wf, region_name_prefix, "region_event_start_in", s_region_event_start_in);
  addTrace(wf, region_name_prefix, "region_event_pop_in", s_region_event_pop_in);
  addTrace(wf, region_name_prefix, "region_data_read_in", s_region_data_read_in);
  addTrace(wf, region_name_prefix, "frame_readout_done_out", s_frame_readout_done_out);
  addTrace(wf, region_name_prefix, "region_fifo_empty_out", s_region_fifo_empty_out);
  addTrace(wf, region_name_prefix, "region_valid_out", s_region_valid_out);
  addTrace(wf, region_name_prefix, "cluster_started", s_cluster_started);

///@todo Probably need to a stream << operator to allow values from fifo to be printed to trace file
//  addTrace(wf, region_name_prefix, "region_data_out", s_region_data_out);
  addTrace(wf, region_name_prefix, "rru_readout_state", s_rru_readout_state);
  addTrace(wf, region_name_prefix, "rru_valid_state", s_rru_valid_state);
  addTrace(wf, region_name_prefix, "rru_header_state", s_rru_header_state);
  addTrace(wf, region_name_prefix, "matrix_readout_delay_counter", s_matrix_readout_delay_counter);
  addTrace(wf, region_name_prefix, "generate_region_header", s_generate_region_header);

///@todo Probably need to a stream << operator to allow values from fifo to be printed to trace file
//  addTrace(wf, region_name_prefix, "region_fifo", s_region_fifo);
  addTrace(wf, region_name_prefix, "region_data_out", s_region_data_out);
}
