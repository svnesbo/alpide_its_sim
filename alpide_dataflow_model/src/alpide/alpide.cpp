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
               int tru_fifo_size, bool enable_clustering, bool continuous_mode,
               bool matrix_readout_speed)
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
  mRRUs.reserve(N_REGIONS);
  for(int i = 0; i < N_REGIONS; i++) {
    std::stringstream ss;
    ss << "RRU_" << i;
    mRRUs[i] = new RegionReadoutUnit(ss.str().c_str(),
                                     i,
                                     region_fifo_size,
                                     matrix_readout_speed,
                                     enable_clustering);

    mRRUs[i]->s_system_clk_in(s_system_clk_in);
    mRRUs[i]->s_frame_readout_start_in(s_frame_readout_start);
    mRRUs[i]->s_region_event_start_in(s_region_event_start);
    mRRUs[i]->s_region_event_pop_in(s_region_event_pop);
    mRRUs[i]->s_region_data_read_in(s_region_data_read);

    mRRUs[i]->s_frame_readout_done_out(s_frame_readout_done[i]);
    mRRUs[i]->s_region_fifo_empty_out(s_region_fifo_empty[i]);
    mRRUs[i]->s_region_valid_out(s_region_valid[i]);
    mRRUs[i]->s_region_data_out(s_region_data[i]);

    mTRU->s_region_fifo_empty_in(s_region_empty[i]);
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
  sensitive << s_strobe_n_in;

  SC_METHOD(mainProcess);
  sensitive_pos << s_system_clk_in;
}


///@brief SystemC process/method controlled by STROBE input.
///       Controls creation of new Multi Event Buffers (MEBs). Together with the frameReadout function, this
///       process essentially does the same as the FROMU (Frame Read Out Management Unit) in the Alpide chip.
///       Note: it is assumed that STROBE is synchronous to the clock.
///       It will not be "dangerous" if it is not, but it will deviate from the real chip implementation.
void Alpide::strobeProcess(void)
{
  int64_t time_now = sc_time_stamp().value();

  if(!s_strobe_n_in) {   // Strobe falling edge - start of frame/event, strobe is active low
    if(mContinuousMode) {
      s_chip_ready_out = true; // A free event buffer is guaranteed in continuous mode..

      if(getNumEvents() == 2) {
        deleteEvent(time_now);

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
        s_chip_ready_out = false;
        mTriggerEventsRejected++;
        s_busy_violation = true;
      } else {
        newEvent();
        mTriggerEventsAccepted++;
        s_chip_ready_out = true;
        s_busy_violation = false;
      }

      // Are we, or did we become busy now?
      if(getNumEvents() == 3)
        s_multi_event_buffers_busy = true;
      else
        s_multi_event_buffers_busy = false;
    }
  }
  else {   // Strobe rising edge - end of frame/event
    s_chip_ready_out = false;
    
    FrameStartFifoWord frame_start_data = {s_busy_violation, mBunchCounter};

    s_busy_violation = false;

    if(s_tru_frame_start_fifo_in.num_free() == 0) {
      // FATAL, TRU FRAME FIFO will now overflow
      s_tru_frame_fifo_busy = true;
      s_tru_data_overrun_mode = true;

      ///@todo The FATAL overflow bit/signal has to be cleared by a RORST/GRST command
      ///      in the Alpide chip, it will not be cleared by automatically.
      s_tru_frame_fifo_fatal_overflow = true;
    } else if(s_tru_frame_start_fifo_in.num_available() > TRU_FRAME_FIFO_ALMOST_FULL2) {
      // DATA OVERRUN MODE
      s_tru_frame_fifo_busy = true;
      s_tru_data_overrun_mode = true;
      ///@todo set readout abort signal here??
    } else if{s_tru_frame_start_fifo_in.num_available() > TRU_FRAME_FIFO_ALMOST_FULL1) {
      // BUSY
      s_tru_frame_fifo_busy = true;
      s_tru_data_overrun_mode = false;
    } else {
      s_tru_frame_fifo_busy = false;
      s_tru_data_overrun_mode = false;
    }

    s_tru_frame_start_fifo_in.nb_write(frame_start_data);
  }
}


///@brief Data transmission SystemC method. Currently runs on 40MHz clock.
///@todo Implement more advanced data transmission method.
void Alpide::mainProcess(void)
{
  frameReadout();  
  dataTransmission();

  s_busy_status = (s_tru_frame_fifo_busy || s_multi_event_buffers_busy)
}


///@brief Frame readout SystemC method @ 40MHz (system clock). 
///       Together with the strobeProcess, this function essentially does the same job as the
///       FROMU (Frame Read Out Management Unit) in the Alpide chip.
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


  switch(s_fromu_readout_state) {
  case WAIT_FOR_EVENTS:
    s_frame_readout_start = false;
    s_frame_readout_done_all = false;
    
    if(s_event_buffers_used > 0)
      s_fromu_readout_state = REGION_READOUT_START;
    break;
    
  case REGION_READOUT_START:
    s_frame_readout_start = true;
    s_frame_readout_done_all = false;
    s_fromu_readout_state = WAIT_FOR_REGION_READOUT;
    break;
    
  case WAIT_FOR_REGION_READOUT:
    s_frame_readout_start = false;
    s_frame_readout_done_all = getFrameReadoutDone();

    if(s_frame_readout_done_all)
      s_fromu_readout_state = REGION_READOUT_DONE;
    break;
    
  case REGION_READOUT_DONE:
    s_frame_readout_start = false;
    s_frame_readout_done_all = false;

    FrameEndFifoWord frame_end_data = {0, 0, 0};
    s_tru_frame_end_fifo_in.nb_write(frame_end_data);

    // Delete the event/frame in matrix/multi-event-buffer that has just been read out
    deleteEvent(time_now);
    s_fromu_readout_state = WAIT_FOR_EVENTS;
    break;
    
  default:
    s_frame_readout_start = false;
    s_frame_readout_done_all = false;    
    s_fromu_readout_state = WAIT_FOR_EVENTS;
    break;
  }

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


///@brief Get logical AND/product of all regions' frame_readout_done signals.
///@return True when frame_readout_done is set in all regions
bool Alpide::getFrameReadoutDone(void)
{
  bool done = true;
  
  for(int i = 0; i < N_REGIONS; i++)
    done &= s_frame_readout_done[i];

  return done;
}


///@brief Add SystemC signals to log in VCD trace file.
///@param wf Pointer to VCD trace file object
///@param name_prefix Name prefix to be added to all the trace names
void Alpide::addTraces(sc_trace_file *wf, std::string name_prefix) const
{
  std::stringstream ss;
  ss << name_prefix << "alpide_" << mChipId << ".";
  std::string alpide_name_prefix = ss.str();

  addTrace(wf, alpide_name_prefix, "chip_ready_out", s_chip_ready_out);
  addTrace(wf, alpide_name_prefix, "serial_data_output", s_serial_data_output);
  addTrace(wf, alpide_name_prefix, "event_buffers_used", s_event_buffers_used);
  addTrace(wf, alpide_name_prefix, "total_number_of_hits", s_total_number_of_hits);
  addTrace(wf, alpide_name_prefix, "oldest_event_number_of_hits", s_oldest_event_number_of_hits);
  addTrace(wf, alpide_name_prefix, "tru_fifo_size", s_tru_fifo_size);

  addTrace(wf, alpide_name_prefix, "region_event_start", s_region_event_start);
  addTrace(wf, alpide_name_prefix, "region_event_pop", s_region_event_pop);    

  addTrace(wf, alpide_name_prefix, "frame_readout_start", s_frame_readout_start);
  addTrace(wf, alpide_name_prefix, "frame_readout_done_all", s_frame_readout_done_all);
  addTrace(wf, alpide_name_prefix, "readout_abort", s_readout_abort);
  addTrace(wf, alpide_name_prefix, "tru_frame_fifo_busy", s_tru_frame_fifo_busy);
  addTrace(wf, alpide_name_prefix, "tru_data_overrun_mode", s_tru_data_overrun_mode);
  addTrace(wf, alpide_name_prefix, "tru_frame_fifo_fatal_overflow", s_tru_frame_fifo_fatal_overflow);
  addTrace(wf, alpide_name_prefix, "multi_event_buffers_busy", s_multi_event_buffers_busy);

  addTrace(wf, alpide_name_prefix, "busy_violation", s_busy_violation;);
  addTrace(wf, alpide_name_prefix, "busy_status", s_busy_status);

  addTrace(wf, alpide_name_prefix, "fromu_readout_state", s_fromu_readout_state);
  addTrace(wf, alpide_name_prefix, "tru_fifo_size", s_tru_fifo_size);

  mTRU->addTraces(wf, alpide_name_prefix);
  
  for(int i = 0; i < N_REGIONS; i++)
    mRRUs[i]->addTraces(wf, alpide_name_prefix);    

}
