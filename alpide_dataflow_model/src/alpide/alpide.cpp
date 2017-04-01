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

  s_tru_frame_fifo_busy = false;
  s_tru_data_overrun_mode = false;
  s_tru_frame_fifo_fatal_overflow = false;
  s_multi_event_buffers_busy = false;
  s_busy_violation = false;
  s_busy_status = false;  
  s_readout_abort = false;

  mTRU = new TopReadoutUnit("TRU", chip_id);

  // Allocate/create/name SystemC FIFOs for the regions and connect the
  // Region Readout Units (RRU) FIFO outputs to Top Readout Unit (TRU) FIFO inputs
  //s_region_fifos.reserve(N_REGIONS);
  mRRUs.reserve(N_REGIONS);
  for(int i = 0; i < N_REGIONS; i++) {
    std::stringstream ss;
    ss << "RRU_" << i;
    mRRUs[i] = new RegionReadoutUnit(ss.str().c_str(), i, region_fifo_size, enable_clustering);

///@todo Replace with a signal, data_read signal is used for reading from region FIFO now
//    mTRU->s_region_fifo_in[i](mRRUs[i]->s_region_fifo);
    
    mRRUs[i]->s_frame_readout_start_in(s_frame_readout_start);
    mRRUs[i]->s_region_event_start_in(s_region_event_start);
    mRRUs[i]->s_region_event_pop_in(s_region_event_pop);
    mRRUs[i]->s_region_data_read_in(s_region_data_read);

    mRRUs[i]->s_region_empty_out(s_region_empty[i]);
    mRRUs[i]->s_region_valid_out(s_region_valid[i]);
    mRRUs[i]->s_region_data_out(s_region_data[i]);

    mTRU->s_region_empty_in(s_region_empty[i]);
    mTRU->s_region_valid_in(s_region_valid[i]);    
  }

  mTRU->s_clk_in(s_system_clk_in);
  mTRU->s_readout_abort_in(s_readout_abort);
  mTRU->s_data_overrun_mode_in(s_data_overrun_mode);

  ///@todo Rename TRU FIFO to DMU FIFO?
  mTRU->s_tru_fifo_out(s_top_readout_fifo);
  
  mTRU->s_region_event_start_out(s_region_event_start);
  mTRU->s_region_event_pop_out(s_region_event_pop);
  mTRU->s_region_data_in(s_region_data[i]);

  s_tru_frame_start_fifo_in(mTRU->s_tru_frame_start_fifo);
  s_tru_frame_end_fifo_in(mTRU->s_tru_frame_end_fifo);

  
  SC_METHOD(strobeProcess);
  sensitive << s_strobe_in;
  
  SC_METHOD(matrixReadout);
  sensitive_pos << s_matrix_readout_clk_in;

  SC_METHOD(mainProcess);
  sensitive_pos << s_system_clk_in;
}


///@brief SystemC process/method controlled by STROBE input.
///       Controls creation of new Multi Event Buffers (MEBs).
///       Note: it is assumed that STROBE is synchronous to the clock.
///       It will not be "dangerous" if it is not, but it will deviate from the real chip implementation.
void Alpide::strobeProcess(void)
{
  int64_t time_now = sc_time_stamp().value();

  if(s_strobe_in) {   // Strobe rising edge    
    if(mContinuousMode) {
      mChipReady = true; // A free event buffer is guaranteed in continuous mode..

      if(getNumEvents() == 2) {
        deleteEvent();

        // No change in number of accepted events, since previous 
        // event was accepted but is now being "rejected".
        mTriggerEventsRejected++;

        s_readout_abort = true;
        s_busy_violation = true;
      } else {
        mTriggerEventsAccepted++;
        s_readout_abort = false;
        s_busy_violation = false;
      }

      newEvent();

      // Are we, or did we become busy now?
      if(getNumEvents() == 2) {
        s_multi_event_buffers_busy = true;
      } else {
        s_multi_event_buffers_busy = false;
      }
    } 
    else if(!mContinuousMode) {
      s_busy_violation = false;
      s_readout_abort = false; // No abort in triggered mode

      if(getNumEvents() == 3) {
        mChipReady = false;
        mTriggerEventsRejected++;
        s_busy_violation = true;
      } else {
        newEvent();
        mTriggerEventsAccepted++;
        mChipReady = true;
        s_busy_violation = false;
      }

      // Are we, or did we become busy now?
      if(getNumEvents() == 3)
        s_multi_event_buffers_busy = true;
      else
        s_multi_event_buffers_busy = false;
    }
  }
  else {   // Strobe falling edge
    mChipReady = false;
    
    FrameStartFifoWord frame_start_data = {s_busy_violation, mBunchCounter};

    s_busy_violation = false;

    if(s_tru_frame_start_fifo_in.num_free() == 0) {
      // FATAL, TRU FRAME FIFO will now overflow
      s_tru_frame_fifo_busy = true;
      s_tru_data_overrun_mode = true;
      s_tru_frame_fifo_fatal_overflow = true;
    } else if(s_tru_frame_start_fifo_in.num_available() > TRU_FRAME_FIFO_ALMOST_FULL2) {
      // DATA OVERRUN MODE
      s_tru_frame_fifo_busy = true;
      s_tru_data_overrun_mode = true;
      s_tru_frame_fifo_fatal_false = true;
    } else if{s_tru_frame_start_fifo_in.num_available() > TRU_FRAME_FIFO_ALMOST_FULL1) {
      // BUSY
      s_tru_frame_fifo_busy = true;
      s_tru_data_overrun_mode = false;
      s_tru_frame_fifo_fatal_false = true;
    } else {
      s_tru_frame_fifo_busy = false;
      s_tru_data_overrun_mode = false;
      s_tru_frame_fifo_fatal_false = true;
    }

    s_tru_frame_start_fifo_in.nb_write(frame_start_data);
  }
}


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
void Alpide::frameReadout(void)
{
  uint64_t time_now = sc_time_stamp().value();
  int MEBs_in_use = getNumEvents();

  // Bunch counter wraps around each orbit
  mBunchCounter++;
  if(mBunchCounter == LHC_ORBIT_BUNCH_COUNT)
    mBunchCounter = 0;
  
  // Update signal with number of event buffers
  s_event_buffers_used = MEBs_in_use;

  // Update signal with total number of hits in all event buffers
  s_total_number_of_hits = getHitTotalAllEvents();

  s_oldest_event_number_of_hits = getHitsRemainingInOldestEvent();

  ///@todo When and where should the events be deleted?
  ///      Currently it happens in PixelMatrix::readoutPixel() when there are no more hits in the matrix.
  ///      But perhaps it is more correct that we iterate over all regions, and when all the
  ///      region_empty signals from the RRUs have been set, then we delete the event from here,
  ///      and not automatically from PixelMatrix::readoutPixel()?

  ///@todo Start readout here

  ///@todo Push frame end word to TRU FIFO when done with readout here
  FrameEndFifoWord frame_end_data = {0, 0, 0};
  s_tru_frame_end_fifo_in.nb_write(frame_end_data);    
}


///@brief Data transmission SystemC method. Currently runs on 40MHz clock.
///@todo Implement more advanced data transmission method.
void Alpide::mainProcess(void)
{
  frameReadout();  
  dataTransmission();

  s_busy_status = (s_tru_frame_fifo_busy || s_multi_event_buffers_busy)
}


///@brief Read out DMU FIFOs and output data on "serial" line.
///       Should be called one time per clock cycle.
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
