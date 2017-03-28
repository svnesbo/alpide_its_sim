/**
 * @file   alpide.h
 * @author Simon Voigt Nesbo
 * @date   December 12, 2016
 * @brief  Source file for Alpide class.
 */

#include "alpide.h"
#include "alpide_constants.h"
#include "../misc/vcd_trace.h"
#include <string>
#include <sstream>


SC_HAS_PROCESS(Alpide);
///@brief Constructor for Alpide.
///@param name    SystemC module name
///@param chip_id Desired chip id
///@param region_fifo_size Depth of Region Readout Unit (RRU) FIFOs
///@param tru_fifo_size Depth of Top Readout Unit (TRU) FIFO
///@param enable_clustering Enable clustering and use of DATA LONG words
///@param continuous_mode Enable continuous mode (triggered mode if false)
Alpide::Alpide(sc_core::sc_module_name name, int chip_id, int region_fifo_size,
               int tru_fifo_size, bool enable_clustering, bool continuous_mode)
  : sc_core::sc_module(name)
  , PixelMatrix(continuous_mode)
  , s_top_readout_fifo(tru_fifo_size)
{
  mChipId = chip_id;

  s_event_buffers_used = 0;
  s_total_number_of_hits = 0;
  s_oldest_event_number_of_hits = 0;
  s_busy_status = false;

  mTRU = new TopReadoutUnit("TRU", chip_id);

  // Allocate/create/name SystemC FIFOs for the regions and connect the
  // Region Readout Units (RRU) FIFO outputs to Top Readout Unit (TRU) FIFO inputs
  //s_region_fifos.reserve(N_REGIONS);
  mRRUs.reserve(N_REGIONS);
  for(int i = 0; i < N_REGIONS; i++) {
    std::stringstream ss;
    ss << "RRU_" << i;
    mRRUs[i] = new RegionReadoutUnit(ss.str().c_str(), i, region_fifo_size, enable_clustering);
    mTRU->s_region_fifo_in[i](mRRUs[i]->s_region_fifo);

    // Connect RRU->TRU region empty signals
    mTRU->s_region_empty_in[i](s_region_empty[i]);
    mRRUs[i]->s_region_empty_out(s_region_empty[i]);    
  }

  mTRU->s_clk_in(s_system_clk_in);
  mTRU->s_busy_status_in(s_busy_status);
  mTRU->s_event_buffers_used_in(s_event_buffers_used);
  mTRU->s_current_event_hits_left_in(s_oldest_event_number_of_hits);
  mTRU->s_tru_fifo_out(s_top_readout_fifo);

  s_tru_frame_start_fifo_in(mTRU->s_tru_frame_start_fifo);
  s_tru_frame_end_fifo_in(mTRU->s_tru_frame_end_fifo);
  
  SC_METHOD(matrixReadout);
  sensitive_pos << s_matrix_readout_clk_in;

  SC_METHOD(frameReadoutProcess);
  sensitive_pos << s_system_clk_in;

  SC_METHOD(dataTransmission);
  sensitive_pos << s_system_clk_in;
}


///@brief Indicate to the Alpide that we are starting on a new event. If the call is
///       successful a new MEB slice is created, and the next calls to setPixel will add pixels
///       to the new event.
///@param event_time Simulation time when the event is pushed/latched into MEB
///                  (use current simulation time).
///@return True if successful and a new MEB slice was created. Returns false if a new MEB slice
///        could not be created (happens only in triggered mode).
///@todo   Does this return value make sense??
bool Alpide::newEvent(uint64_t event_time)
{
  FrameStartFifoWord frame_start_data = {0, mBunchCounter};

  bool rv = PixelMatrix::newEvent(event_time);

  // Event successfully created - push a frame start word to the TRU's Frame Start FIFO
  if(rv == false) {
    ///@todo Is busy violation only in triggered mode, or also continuous?
    frame_start_data.busy_violation = 1;

    if(mContinuousMode) {
      s_readout_abort = true;
    }
  }

  if(s_tru_frame_start_fifo_in.num_free() == 0) {
    // FATAL, TRU FRAME FIFO has overflowed!
  } else if(s_tru_frame_start_fifo_in.num_available() > TRU_FRAME_FIFO_ALMOST_FULL2) {
    // DATA OVERRUN MODE
  } else if{s_tru_frame_start_fifo_in.num_available() > TRU_FRAME_FIFO_ALMOST_FULL1) {
    // BUSY
  }
  s_tru_frame_start_fifo_in.nb_write(frame_start_data);

  FrameEndFifoWord frame_end_data = {0, 0, 0};
  s_tru_frame_end_fifo_in.nb_write(frame_end_data);
  
  return rv;
}



///@todo GET RID OFF THIS FUNCTION! IMPLEMENT EACH RRU AS A SYSTEMC METHOD/PROCESS, WITH A STATE MACHINE!
///@todo Actually, keep this function, because I don't have a good way of passing a reference
///      to the pixel matrix to the RRUs without using this function... :(
///@brief Matrix readout SystemC method. This method is clocked by the matrix readout clock.
///       The matrix readout period can be specified by the user in a register in the Alpide,
///       and is intended to allow the priority encoder a little more time to "settle" because
///       it is a relatively slow asynchronous circuit.
///       The method here triggers readout of a pixel from each region, into region buffers,
///       and updates some status signals related to regions/event-buffers.
void Alpide::matrixPriEncReadout(void)
{
  uint64_t time_now = sc_time_stamp().value();
  int MEBs_in_use = getNumEvents();
  
  // Read out a pixel from each region in the matrix
  for(int region_num = 0; region_num < N_REGIONS; region_num++) {
    mRRUs[region_num]->readoutNextPixel(*this, time_now);
  }
}



///@brief Frame readout SystemC method @ 40MHz (system clock). 
///       Essentially does the same job as the FROMU (Frame Read Out Management Unit) in the
///       Alpide chip.
void Alpide::frameReadoutProcess(void)
{
  uint64_t time_now = sc_time_stamp().value();
  int MEBs_in_use = getNumEvents();

  // Bunch counter wraps around each orbit
  mBunchCounter++;
  if(mBunchCounter == LHC_ORBIT_BUNCH_COUNT)
    mBunchCounter = 0;
  
  // Update signal with number of event buffers
  s_event_buffers_used = MEBs_in_use;

  if(mContinuousMode) {
    if(MEBs_in_use > 1) {
      s_busy_status = true;
      // Notify TRU so BUSY ON word can be sent here?
    } else {
      s_busy_status = false;
      // Notify TRU so BUSY OFF word can be sent here?
    }
  } else { // Triggered mode
    if(MEBs_in_use > 2) { // All MEBs full
      s_busy_status = true;
      // Notify TRU so BUSY ON word can be sent here?      
    } else {
      s_busy_status = false;
      // Notify TRU so BUSY ON word can be sent here?      
    }
  }

  // Update signal with total number of hits in all event buffers
  s_total_number_of_hits = getHitTotalAllEvents();

  s_oldest_event_number_of_hits = getHitsRemainingInOldestEvent();

  ///@todo When and where should the events be deleted?
  ///      Currently it happens in PixelMatrix::readoutPixel() when there are no more hits in the matrix.
  ///      But perhaps it is more correct that we iterate over all regions, and when all the
  ///      region_empty signals from the RRUs have been set, then we delete the event from here,
  ///      and not automatically from PixelMatrix::readoutPixel()?
  
  // Read out a pixel from each region in the matrix
  for(int region_num = 0; region_num < N_REGIONS; region_num++) {
    mRRUs[region_num]->readoutNextPixel(*this, time_now);
  }
}


///@brief Data transmission SystemC method. Currently runs on 40MHz clock.
///@todo Implement more advanced data transmission method.
void Alpide::dataTransmission(void)
{
  AlpideDataWord dw;

  s_tru_fifo_size = s_top_readout_fifo.num_available();
  
  if(s_top_readout_fifo.nb_read(dw)) {
    sc_uint<24> data = dw.data[2] << 16 | dw.data[1] << 8 | dw.data[0];
    s_serial_data_output = data;
  }
}

///@brief Add SystemC signals to log in VCD trace file.
///@param wf Pointer to VCD trace file object
///@param name_prefix Name prefix to be added to all the trace names
void Alpide::addTraces(sc_trace_file *wf, std::string name_prefix) const
{
  std::stringstream ss;
  ss << name_prefix << "alpide_" << mChipId << ".";
  std::string alpide_name_prefix = ss.str();

  addTrace(wf, alpide_name_prefix, "event_buffers_used", s_event_buffers_used);
  addTrace(wf, alpide_name_prefix, "hits_in_matrix", s_total_number_of_hits);
  addTrace(wf, alpide_name_prefix, "serial_data_output", s_serial_data_output);
  addTrace(wf, alpide_name_prefix, "tru_fifo_size", s_tru_fifo_size);  

  mTRU->addTraces(wf, alpide_name_prefix);

  for(int i = 0; i < N_REGIONS; i++)
    mRRUs[i]->addTraces(wf, alpide_name_prefix);
}
