/**
 * @file   Alpide.cpp
 * @author Simon Voigt Nesbo
 * @date   December 12, 2016
 * @brief  Source file for Alpide class.
 */

#include "Alpide.hpp"
#include "alpide_constants.hpp"
#include "../misc/vcd_trace.hpp"
#include <string>
#include <sstream>


SC_HAS_PROCESS(Alpide);
///@brief Constructor for Alpide.
///@param[in] name    SystemC module name
///@param[in] chip_id Desired chip id
///@param[in] region_fifo_size Depth of Region Readout Unit (RRU) FIFOs
///@param[in] dmu_fifo_size Depth of DMU (Data Management Unit) FIFO.
///@param[in] dtu_delay_cycles Number of clock cycle delays associated with Data Transfer Unit (DTU)
///@param[in] strobe_length_ns Strobe length (in nanoseconds)
///@param[in] strobe_extension Enable/disable strobe extension
///           (if new strobe received before the previous strobe interval ended)
///@param[in] enable_clustering Enable clustering and use of DATA LONG words
///@param[in] continuous_mode Enable continuous mode (triggered mode if false)
///@param[in] matrix_readout_speed True for fast readout (2 clock cycles), false is slow (4 cycles).
Alpide::Alpide(sc_core::sc_module_name name, int chip_id, int region_fifo_size,
               int dmu_fifo_size, int dtu_delay_cycles, int strobe_length_ns,
               bool strobe_extension, bool enable_clustering, bool continuous_mode,
               bool matrix_readout_speed)
  : sc_core::sc_module(name)
  , PixelMatrix(continuous_mode)
  , s_control_input("s_control_input")
  , s_data_output("s_data_output")
  , s_chip_ready_out("chip_ready_out")
  , s_dmu_fifo(dmu_fifo_size)
  , s_dtu_delay_fifo(dtu_delay_cycles+1)
  , s_frame_start_fifo(TRU_FRAME_FIFO_SIZE)
  , s_frame_end_fifo(TRU_FRAME_FIFO_SIZE)
  , mStrobeExtensionEnable(strobe_extension)
  , mStrobeLengthNs(strobe_length_ns)
{
  mChipId = chip_id;
  mEnableDtuDelay = dtu_delay_cycles > 0;

  s_chip_ready_out(s_chip_ready_internal);

  s_serial_data_out_exp(s_serial_data_out);

  s_event_buffers_used_debug = 0;
  s_total_number_of_hits = 0;
  s_oldest_event_number_of_hits = 0;

  s_frame_fifo_busy = false;
  s_fatal_state = false;
  s_multi_event_buffers_busy = false;
  s_flushed_incomplete = false;
  s_busy_violation = false;
  s_busy_status = false;
  s_readout_abort = false;
  s_chip_ready_internal = false;
  s_strobe_n = true;

  mStrobeActive = false;

  mTRU = new TopReadoutUnit("TRU", chip_id);

  // Allocate/create/name SystemC FIFOs for the regions and connect the
  // Region Readout Units (RRU) FIFO outputs to Top Readout Unit (TRU) FIFO inputs
  mRRUs.reserve(N_REGIONS);
  for(int i = 0; i < N_REGIONS; i++) {
    std::stringstream ss;
    ss << "RRU_" << i;
    mRRUs[i] = new RegionReadoutUnit(ss.str().c_str(),
                                     this,
                                     i,
                                     region_fifo_size,
                                     matrix_readout_speed,
                                     enable_clustering);

    mRRUs[i]->s_system_clk_in(s_system_clk_in);
    mRRUs[i]->s_frame_readout_start_in(s_frame_readout_start);
    mRRUs[i]->s_readout_abort_in(s_readout_abort);
    mRRUs[i]->s_region_event_start_in(s_region_event_start);
    mRRUs[i]->s_region_event_pop_in(s_region_event_pop);
    mRRUs[i]->s_region_data_read_in(s_region_data_read[i]);

    mRRUs[i]->s_frame_readout_done_out(s_frame_readout_done[i]);
    mRRUs[i]->s_region_fifo_empty_out(s_region_fifo_empty[i]);
    mRRUs[i]->s_region_valid_out(s_region_valid[i]);
    mRRUs[i]->s_region_data_out(s_region_data[i]);

    mTRU->s_region_fifo_empty_in[i](s_region_fifo_empty[i]);
    mTRU->s_region_valid_in[i](s_region_valid[i]);
    mTRU->s_region_data_in[i](s_region_data[i]);
    mTRU->s_region_data_read_out[i](s_region_data_read[i]);
  }

  mTRU->s_clk_in(s_system_clk_in);
  mTRU->s_readout_abort_in(s_readout_abort);
  mTRU->s_fatal_state_in(s_fatal_state);
  mTRU->s_region_event_start_out(s_region_event_start);
  mTRU->s_region_event_pop_out(s_region_event_pop);
  mTRU->s_frame_start_fifo_output(s_frame_start_fifo);
  mTRU->s_frame_end_fifo_output(s_frame_end_fifo);
  mTRU->s_dmu_fifo_input(s_dmu_fifo);

  // Initialize DTU delay FIFO with comma words
  AlpideDataWord dw = AlpideComma();
  while(s_dtu_delay_fifo.num_free() > 0)
    s_dtu_delay_fifo.nb_write(dw);

  s_control_input.register_transport(std::bind(&Alpide::processCommand,
                                               this, std::placeholders::_1));

  SC_METHOD(mainMethod);
  sensitive_pos << s_system_clk_in;

  SC_METHOD(triggerMethod);
  sensitive << E_trigger;
  dont_initialize();

  SC_METHOD(strobeDurationMethod);
  sensitive << E_strobe_interval_done;
  dont_initialize();

  SC_METHOD(strobeAndFramingMethod);
  sensitive << s_strobe_n;
  dont_initialize();

}


void Alpide::newEvent(uint64_t event_time)
{
  PixelMatrix::newEvent(event_time);
  // Set chip ready signal here??
}


///@brief Data transmission SystemC method. Currently runs on 40MHz clock.
///@todo Implement more advanced data transmission method.
void Alpide::mainMethod(void)
{
  frameReadout();
  dataTransmission();
  updateBusyStatus();

  // For the stimuli class to work properly this needs to be delayed one clock cycle
  //s_chip_ready_out = s_chip_ready_internal;
}


ControlResponsePayload Alpide::processCommand(ControlRequestPayload const &request)
{
  if (request.opcode == 0x55) {
    SC_REPORT_INFO_VERB(name(), "Received Trigger", sc_core::SC_DEBUG);
    E_trigger.notify();
  } else {
    SC_REPORT_ERROR(name(), "Invalid opcode received");
  }
  // do nothing
  return {};
}


///@brief Called on trigger input - initiates strobing intervals
///       All triggers have to be supplied externally to the Alpide module.
///       There is not automatic trigger/strobe synthesizer implemented here.
void Alpide::triggerMethod(void)
{
  uint64_t time_now = sc_time_stamp().value();

  ///@todo What happens if I get one trigger at the exact time the strobe goes inactive???

  std::cout << "@" << time_now << ": Alpide with ID " << mChipId << " triggered." << std::endl;

  if(s_strobe_n.read() == true) {
    // Strobe not active - start new interval
    //s_strobe_n = false;
    E_strobe_interval_done.notify(0, SC_NS);
    E_strobe_interval_done.notify(mStrobeLengthNs, SC_NS);
  } else if(s_strobe_n.read() == false) {
    // Strobe already active
    if(mStrobeExtensionEnable) {
      E_strobe_interval_done.cancel();

      // If an E_strobe_interval_done event already happened the same cycle, then
      // strobeDurationMethod() could have written false to s_strobe_n already, but it would not
      // have updated yet, so make sure it stays active (false/low).
      //s_strobe_n = false;
      E_strobe_interval_done.notify(mStrobeLengthNs, SC_NS);
    } else {
      mTriggersRejected++;
    }
  }
}


void Alpide::strobeDurationMethod(void)
{
  s_strobe_n = !s_strobe_n;
}


///@brief This SystemC method handles framing of events according to the strobe intervals.
///       Controls creation of new Multi Event Buffers (MEBs). Together with the frameReadout
///       function, this process essentially does the same as the FROMU (Frame Read Out Management
///       Unit) in the Alpide chip.
///       Note: it is assumed that STROBE is synchronous to the clock.
///       It will not be "dangerous" if it is not, but it will deviate from the real chip implementation.
void Alpide::strobeAndFramingMethod(void)
{
  int64_t time_now = sc_time_stamp().value();

  if(s_strobe_n.read() == false && mStrobeActive == false) {   // Strobe falling edge - start of frame/event, strobe is active low
    mStrobeActive = true;
    mStrobeStartTime = time_now;

    // Remove "expired" hits from hit list in the pixel front end
    removeInactiveHits(time_now);

    ///@todo What should I do in data overrun mode (when readout_abort is set)?
    ///      Should I still accept events? I need the frame end word to be added, for the normal
    ///      transmission of CHIP HEADER/TRAILER words. This is currently done by the frameReadout()
    ///      method, which requires there to be events in the MEB.
    ///@todo Should rejected event frame count be increased in data overrun mode?
    if(mContinuousMode) {
      if(getNumEvents() == 3) {
        // Reject events if all MEBs are full in continuous.
        // And yes, this can happen! Also in the real chip..
        mTriggersRejected++;
        s_busy_violation = true;
        //s_flushed_incomplete = false;
        s_chip_ready_internal = false;
      } else if(getNumEvents() == 2) {
        // Flush oldest event to make room if we are becoming full in continuous
        flushOldestEvent();
        newEvent(time_now);

        mEventFramesFlushed++;
        mTriggersAccepted++;
        s_busy_violation = false;
        s_flushed_incomplete = true;
        s_chip_ready_internal = true;
      } else {
        // Normal operation in continuous, with at least 2 free buffers
        newEvent(time_now);

        mTriggersAccepted++;
        s_busy_violation = false;
        s_flushed_incomplete = false;
        s_chip_ready_internal = true;
      }
    }
    else if(!mContinuousMode) {
      s_flushed_incomplete = false; // No flushing in triggered mode

      if(getNumEvents() == 3) {
        s_chip_ready_internal = false;
        mTriggersRejected++;
        s_busy_violation = true;
      } else {
        newEvent(time_now);
        mTriggersAccepted++;
        s_chip_ready_internal = true;
        s_busy_violation = false;
      }
    }
  }
  // Strobe rising edge - end of frame/event
  // Make sure we can't trigger first on the wrong end of strobe by checking chip_ready signal
  else if(s_strobe_n.read() == true && mStrobeActive == true) {
    // Latch event/pixels if chip was ready, ie. there was a free MEB for this strobe
    if(s_chip_ready_internal) {
      this->getEventFrame(mStrobeStartTime, time_now, mEventIdCount).feedHitsToPixelMatrix(*this);
      mEventIdCount++;
    }

    s_chip_ready_internal = false;
    mStrobeActive = false;

    FrameStartFifoWord frame_start_data = {s_busy_violation, mBunchCounter};
    int frame_start_fifo_size = s_frame_start_fifo.used();
    bool frame_start_fifo_empty = !s_frame_start_fifo.nb_can_get();
    bool frame_start_fifo_full = !s_frame_start_fifo.nb_can_put();
    bool frame_end_fifo_empty = !s_frame_end_fifo.nb_can_get();

    s_busy_violation = false;

    // Once set, we are only allowed to clear the readout_abort signal
    // (ie go out of data overrun mode) when the frame fifo has been cleared.
    if(frame_start_fifo_empty && frame_end_fifo_empty) {
      if(s_readout_abort == true) {
        std::cout << "@ " << time_now << " ns:\t" << "Alpide chip ID: " << mChipId;
        std::cout << " exited data overrun mode." << std::endl;
      }

      s_frame_fifo_busy = false;
      s_readout_abort = false;
    } else if(frame_start_fifo_full) {
      // FATAL, TRU FRAME FIFO will now overflow
      s_frame_fifo_busy = true;
      s_readout_abort = true;

      if(s_fatal_state == false) {
        std::cout << "@ " << time_now << " ns:\t" << "Alpide chip ID: " << mChipId;
        std::cout << " entered fatal mode." << std::endl;
      }

      ///@todo The FATAL overflow bit/signal has to be cleared by a RORST/GRST command
      ///      in the Alpide chip, it will not be cleared by automatically.
      s_fatal_state = true;
    } else if(frame_start_fifo_size > TRU_FRAME_FIFO_ALMOST_FULL2) {
      // DATA OVERRUN MODE
      ///@todo Need to clear RRU FIFOs, and MEBs when entering this state

      if(s_readout_abort == false) {
        std::cout << "@ " << time_now << " ns:\t" << "Alpide chip ID: " << mChipId;
        std::cout << " entered data overrun mode." << std::endl;
      }

      s_frame_fifo_busy = true;
      s_readout_abort = true;
    } else if(frame_start_fifo_size > TRU_FRAME_FIFO_ALMOST_FULL1) {
      // BUSY
      s_frame_fifo_busy = true;
    } else if(!s_readout_abort) {
      s_frame_fifo_busy = false;
    }

    s_frame_start_fifo.nb_put(frame_start_data);
  }
}


///@brief Frame readout SystemC method @ 40MHz (system clock).
///       Together with the strobeProcess, this function essentially does the same job as the
///       FROMU (Frame Read Out Management Unit) in the Alpide chip.
void Alpide::frameReadout(void)
{
  uint64_t time_now = sc_time_stamp().value();
  int MEBs_in_use = getNumEvents();
  int frame_start_fifo_size = s_frame_start_fifo.used();
  int frame_end_fifo_size = s_frame_end_fifo.used();
  s_frame_start_fifo_size_debug = frame_start_fifo_size;
  s_frame_end_fifo_size_debug = frame_end_fifo_size;

  // Bunch counter wraps around each orbit
  mBunchCounter++;
  if(mBunchCounter == LHC_ORBIT_BUNCH_COUNT)
    mBunchCounter = 0;

  // Update signal with number of event buffers
  s_event_buffers_used_debug = MEBs_in_use;

  // Update signal with total number of hits in all event buffers
  s_total_number_of_hits = getHitTotalAllEvents();

  s_oldest_event_number_of_hits = getHitsRemainingInOldestEvent();


  switch(s_fromu_readout_state.read()) {
  case WAIT_FOR_EVENTS:
    s_frame_readout_start = false;
    s_frame_readout_done_all = false;

    // If there is only 1 MEB in use, but strobe is still active,
    // then this event is not ready to be read out yet.
    if(MEBs_in_use > 1 || (MEBs_in_use == 1 && mStrobeActive == false))
      s_fromu_readout_state = REGION_READOUT_START;
    break;

  case REGION_READOUT_START:
    s_frame_readout_start = true;
    s_frame_readout_done_all = false;
    s_fromu_readout_state = WAIT_FOR_REGION_READOUT;
    break;

  case WAIT_FOR_REGION_READOUT:
    s_frame_readout_start = false;

    // Inhibit done signal the cycle we are giving out the start signal
    s_frame_readout_done_all = getFrameReadoutDone() && !s_frame_readout_start;

    // Go straigth to REGION_READOUT_DONE in data overrun mode,
    // so that we can clear the multi event buffers.
    if(s_readout_abort) {
      s_fromu_readout_state = REGION_READOUT_DONE;
      s_flushed_incomplete = false;
    } else if(s_frame_readout_done_all) {
      mNextFrameEndWord.flushed_incomplete = s_flushed_incomplete;

      ///@todo Strobe extended not implemented yet
      mNextFrameEndWord.strobe_extended = false;

      ///@todo Should the busy_transition flag always be set like this when chip is busy,
      ///      or should it only happen when the chip goes into or out of busy state?
      mNextFrameEndWord.busy_transition = s_busy_status;

      s_flushed_incomplete = false;
      s_fromu_readout_state = REGION_READOUT_DONE;
    }
    break;

  case REGION_READOUT_DONE:
    s_frame_readout_start = false;
    s_frame_readout_done_all = false;

    s_frame_end_fifo.nb_put(mNextFrameEndWord);

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


///@brief Read out data from Data Management Unit (DMU) FIFO, feed data through
///       Data Transfer Unit (DTU) FIFO, and output data on "serial" line.
///       Data is not actually serialized here, it is transmitted as 24-bit words.
///
///       DMU FIFO --> DTU FIFO --> Data output
///
///       The Data Transfer Unit (DTU), which normally serializes data, is here represented with
///       a dummy FIFO to implement a delay element. The DTU Delay FIFO will always be filled, and
///       should be configured to have a size equivalent to the delay in terms of number of clock
///       cycles that the DTU in the Alpide chip adds to data transmission.
///
///       Should be called one time per clock cycle.
///@todo  There needs to be a Busy FIFO, and this method needs to pick words from either
///       that FIFO or from the DMU FIFO.
void Alpide::dataTransmission(void)
{
  AlpideDataWord dw_dtu = AlpideComma();
  AlpideDataWord dw_dmu = AlpideComma();
  sc_uint<24> data_out;

  if(mEnableDtuDelay) {
    s_dmu_fifo_size = s_dmu_fifo.num_available();

    // DTU FIFO should always be filled,
    // but in case it is not we will output a comma instead
    if(s_dtu_delay_fifo.nb_read(dw_dtu) == false) {
      dw_dtu = AlpideComma();
    }

    data_out = dw_dtu.data[2] << 16 | dw_dtu.data[1] << 8 | dw_dtu.data[0];
    s_serial_data_out = data_out;

    // Get next dataword from DMU FIFO, or use COMMA word instead if nothing was read from  DMU FIFO
    if(s_dmu_fifo.nb_read(dw_dmu) == false) {
      dw_dmu = AlpideComma();
    }

    s_dtu_delay_fifo.nb_write(dw_dmu);
    sc_uint<24> data_dtu_input = dw_dmu.data[2] << 16 | dw_dmu.data[1] << 8 | dw_dmu.data[0];
    s_serial_data_dtu_input_debug = data_dtu_input;
  }
  // With dtu_delay_cycles = 0 the dtu delay fifo is only 1 word deep, and you get into this mode
  // where it is full, empty, full, and you lose every 2nd word.. so we just bypass that
  // delay fifo in this event..
  else {
    // Get next dataword from DMU FIFO, or use COMMA word instead if nothing was read from  DMU FIFO
    if(s_dmu_fifo.nb_read(dw_dmu) == false) {
      dw_dmu = AlpideComma();
    }
    data_out = dw_dmu.data[2] << 16 | dw_dmu.data[1] << 8 | dw_dmu.data[0];
    s_serial_data_dtu_input_debug = data_out;
    s_serial_data_out = data_out;
  }

  // Data initiator socket output
  DataPayload socket_dw;
  socket_dw.data.push_back(dw_dmu.data[2]);
  socket_dw.data.push_back(dw_dmu.data[1]);
  socket_dw.data.push_back(dw_dmu.data[0]);
  s_data_output->put(socket_dw);
}


///@brief Get logical AND/product of all regions' frame_readout_done signals.
///@return True when frame_readout_done is set in all regions
bool Alpide::getFrameReadoutDone(void)
{
  bool done = true;

  for(int i = 0; i < N_REGIONS; i++)
    done = done && s_frame_readout_done[i];

  return done;
}


///@brief Update internal busy status signals
void Alpide::updateBusyStatus(void)
{
  if(mContinuousMode) {
    if(getNumEvents() > 1) {
      s_multi_event_buffers_busy = true;
    } else {
      s_multi_event_buffers_busy = false;
    }
  } else { // Triggered mode
    if(getNumEvents() == 3)
      s_multi_event_buffers_busy = true;
    else
      s_multi_event_buffers_busy = false;
  }

  s_busy_status = (s_frame_fifo_busy || s_multi_event_buffers_busy);
}


///@brief Add SystemC signals to log in VCD trace file.
///@param[in,out] wf Pointer to VCD trace file object
///@param[in] name_prefix Name prefix to be added to all the trace names
void Alpide::addTraces(sc_trace_file *wf, std::string name_prefix) const
{
  std::stringstream ss;
  ss << name_prefix << "alpide_" << mChipId << ".";
  std::string alpide_name_prefix = ss.str();

  //addTrace(wf, alpide_name_prefix, "chip_ready_out", s_chip_ready_out);
  addTrace(wf, alpide_name_prefix, "chip_ready_internal", s_chip_ready_internal);
  addTrace(wf, alpide_name_prefix, "serial_data_out", s_serial_data_out);
  addTrace(wf, alpide_name_prefix, "event_buffers_used_debug", s_event_buffers_used_debug);
  addTrace(wf, alpide_name_prefix, "frame_start_fifo_size_debug", s_frame_start_fifo_size_debug);
  addTrace(wf, alpide_name_prefix, "frame_end_fifo_size_debug", s_frame_end_fifo_size_debug);
  addTrace(wf, alpide_name_prefix, "total_number_of_hits", s_total_number_of_hits);
  addTrace(wf, alpide_name_prefix, "oldest_event_number_of_hits", s_oldest_event_number_of_hits);

  addTrace(wf, alpide_name_prefix, "region_event_start", s_region_event_start);
  addTrace(wf, alpide_name_prefix, "region_event_pop", s_region_event_pop);

  addTrace(wf, alpide_name_prefix, "frame_readout_start", s_frame_readout_start);
  addTrace(wf, alpide_name_prefix, "frame_readout_done_all", s_frame_readout_done_all);
  addTrace(wf, alpide_name_prefix, "flushed_incomplete", s_flushed_incomplete);
  addTrace(wf, alpide_name_prefix, "busy_violation", s_busy_violation);
  addTrace(wf, alpide_name_prefix, "busy_status", s_busy_status);
  addTrace(wf, alpide_name_prefix, "frame_fifo_busy", s_frame_fifo_busy);
  addTrace(wf, alpide_name_prefix, "multi_event_buffers_busy", s_multi_event_buffers_busy);
  addTrace(wf, alpide_name_prefix, "readout_abort", s_readout_abort);
  addTrace(wf, alpide_name_prefix, "fatal_state", s_fatal_state);

  addTrace(wf, alpide_name_prefix, "fromu_readout_state", s_fromu_readout_state);
  addTrace(wf, alpide_name_prefix, "dmu_fifo_size", s_dmu_fifo_size);

//  addTrace(wf, alpide_name_prefix, "frame_start_fifo", s_frame_start_fifo);
//  addTrace(wf, alpide_name_prefix, "frame_end_fifo", s_frame_end_fifo);
  addTrace(wf, alpide_name_prefix, "serial_data_dtu_input_debug", s_serial_data_dtu_input_debug);

  mTRU->addTraces(wf, alpide_name_prefix);

  for(int i = 0; i < N_REGIONS; i++)
    mRRUs[i]->addTraces(wf, alpide_name_prefix);

}
