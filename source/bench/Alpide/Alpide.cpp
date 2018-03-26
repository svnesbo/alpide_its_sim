/**
 * @file   Alpide.cpp
 * @author Simon Voigt Nesbo
 * @date   December 12, 2016
 * @brief  Source file for Alpide class.
 * @todo   Finish strobe extension. See comments in triggerMethod()
 *         and in AlpideDataWord.hpp.
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
///@param[in] dtu_delay_cycles Number of clock cycle delays associated with Data Transfer Unit (DTU)
///@param[in] strobe_length_ns Strobe length (in nanoseconds)
///@param[in] strobe_extension Enable/disable strobe extension
///           (if new strobe received before the previous strobe interval ended)
///@param[in] enable_clustering Enable clustering and use of DATA LONG words
///@param[in] continuous_mode Enable continuous mode (triggered mode if false)
///@param[in] matrix_readout_speed True for fast readout (2 clock cycles), false is slow (4 cycles).
///@param[in] outer_barrel_mode True: outer barrel mode. False: inner barrel mode
///@param[in] outer_barrel_master Only relevant if in OB mode.
///           True: OB master. False: OB slave
///@param[in] outer_barrel_slave_count Number of slave chips connected to outer barrel master
///@param[in] min_busy_cycles Minimum number of cycles that the internal busy signal has to be
///           asserted before the chip transmits BUSY_ON
Alpide::Alpide(sc_core::sc_module_name name, int chip_id, int dtu_delay_cycles,
               int strobe_length_ns, bool strobe_extension, bool enable_clustering,
               bool continuous_mode, bool matrix_readout_speed, int min_busy_cycles,
               bool outer_barrel_mode, bool outer_barrel_master, int outer_barrel_slave_count)
  : sc_core::sc_module(name)
  , s_control_input("s_control_input")
  , s_data_output("s_data_output")
  , s_chip_ready_out("chip_ready_out")
  , s_local_bus_data_in(outer_barrel_slave_count)
  , s_local_busy_in(outer_barrel_slave_count)
  , s_dmu_fifo(DMU_FIFO_SIZE)
  , s_dtu_delay_fifo(dtu_delay_cycles+1)
  , s_busy_fifo(BUSY_FIFO_SIZE)
  , s_frame_start_fifo(TRU_FRAME_FIFO_SIZE)
  , s_frame_end_fifo(TRU_FRAME_FIFO_SIZE)
  , mContinuousMode(continuous_mode)
  , mStrobeExtensionEnable(strobe_extension)
  , mStrobeLengthNs(strobe_length_ns)
  , mMinBusyCycles(min_busy_cycles)
  , mObMode(outer_barrel_mode)
  , mObMaster(outer_barrel_master)
  , mObSlaveCount(outer_barrel_slave_count)
{
  mChipId = chip_id;
  mEnableDtuDelay = dtu_delay_cycles > 0;

  s_chip_ready_out(s_chip_ready_internal);

  s_serial_data_out_exp(s_serial_data_out);
  s_serial_data_trig_id_exp(s_serial_data_trig_id);

  // Initialize data out signal to all IDLEs
  s_serial_data_out = 0xFFFFFF;
  s_serial_data_trig_id = 0;

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
                                     REGION_FIFO_SIZE,
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

  // Initialize DTU delay FIFO with idle words
  sc_uint<24> dw_idle_data = ((uint32_t) DW_IDLE << 16) |
                             ((uint32_t) DW_IDLE << 8) |
                              (uint32_t) DW_IDLE;

  while(s_dtu_delay_fifo.num_free() > 0) {
    s_dtu_delay_fifo.nb_write(dw_idle_data);
  }

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

  // Only IB/OB-master chips need the busy FIFO method
  if(!outer_barrel_mode || (outer_barrel_mode && outer_barrel_master)) {
    SC_METHOD(busyFifoMethod);
    sensitive << s_busy_status;
    dont_initialize();
  }

  // SC_METHOD(strobeAndFramingMethod);
  // sensitive << s_strobe_n;
  // dont_initialize();

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
  strobeInput();
  frameReadout();
  dataTransmission();
  updateBusyStatus();
}


ControlResponsePayload Alpide::processCommand(ControlRequestPayload const &request)
{
  if (request.opcode == 0x55) {
    // Increase trigger ID counter. See sendTrigger() in ReadoutUnit.cpp for details.
    // This use of the data field in the control word is only used as a convenient
    // way of having a synchronized trigger ID in the Alpide and RU in these simulations,
    // it does not happen in the real system.
    mTrigIdCount += request.data;
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
    mStrobeExtended = false;
    E_strobe_interval_done.notify(0, SC_NS);
  } else if(s_strobe_n.read() == false) {
    // Strobe already active
    if(mStrobeExtensionEnable) {
      ///@todo Strobe extension must be tied to the readout flags
      ///      With the current architecture this is hard to do,
      ///      because the strobe_extended flag is correctly implemented
      ///      in FrameEndFifoWord, but the FrameEndFifoWord is not
      ///      created before the MEB has been read out by RRUs.
      ///      By then we could have gotten a new strobe, which would
      ///      overwrite this flag. If we simply cheat and move the
      ///      strobe_extended flag to FrameStartFifoWord, we can set it
      ///      in FrameStartFifoWord based on this flag at the end of
      ///      the strobe interval.
      mStrobeExtended = true;
      E_strobe_interval_done.cancel();
      E_strobe_interval_done.notify(mStrobeLengthNs, SC_NS);
    } else {
      mTriggersRejected++;
    }
  }
}


void Alpide::strobeDurationMethod(void)
{
  if(s_strobe_n.read() == true) {
    // Strobe was inactive - start of strobing interval
    s_strobe_n = false;
    mTrigIdForStrobe = mTrigIdCount;
    E_strobe_interval_done.notify(mStrobeLengthNs, SC_NS);
  } else {
    // Strobe was active - end of strobing interval
    s_strobe_n = true;
  }
}


///@brief This function handles framing of events according to the strobe intervals.
///       Controls creation of new Multi Event Buffers (MEBs). Together with the frameReadout
///       function, this process essentially does the same as the FROMU (Frame Read Out Management
///       Unit) in the Alpide chip.
///       Note: it is assumed that STROBE is synchronous to the clock.
///       It will not be "dangerous" if it is not, but it will deviate from the real chip implementation.
void Alpide::strobeInput(void)
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
        mBusyViolations++;
        s_chip_ready_internal = false;

        // Flushed incomplete flag doesn't matter, in a busy violation
        // you only get the busy violation flag (see Alpide manual).
        // The TRU code will set all the other readout flags to zero.
        // s_flushed_incomplete = false;
      } else if(getNumEvents() == 2) {
        // Flush oldest event to make room if we are becoming full in continuous
        flushOldestEvent();
        newEvent(time_now);

        mFlushedIncompleteCount++;
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
        mBusyViolations++;
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

    FrameStartFifoWord frame_start_data = {s_busy_violation,
                                           mBunchCounter,
                                           mTrigIdForStrobe};

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
///       Together with the strobeAndFramingMethod, this function essentially
///       does the same job as the FROMU (Frame Read Out Management Unit) in the Alpide chip.
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
void Alpide::dataTransmission(void)
{
  // Trace signals for fifo sizes
  s_dmu_fifo_size = s_dmu_fifo.num_available();
  s_busy_fifo_size = s_busy_fifo.num_available();



  if(mObMode && !mObMaster) {
    return; // Outer barrel slave does not transmit data
  }

  sc_uint<24> dw_dtu_fifo_input;
  sc_uint<24> dw_dtu_fifo_output;

  AlpideDataWord data_word = AlpideIdle();

  // -------------------
  // Outer barrel master
  // -------------------
  if(mObMode && mObMaster) {

    // Prioritize busy words over data words
    if(s_busy_fifo.num_available() > 0) {
      s_busy_fifo.nb_read(data_word);
      dw_dtu_fifo_input = data_word.data[2] << 16;
    }
    else { // Data

      // If we're not currently processing a 24-bit word,
      // read a new 24-bit word from chip that currently has the "token"
      if(mObDwBytesRemaining == 0) {
        // Data words are transmitted with most significant byte first
        mObDwByteIndex = 2;
        mObChipSel = mObNextChipSel;

        if(mObChipSel < mObSlaveCount) {
          if(s_local_bus_data_in[mObChipSel]->num_available() > 0) {
            s_local_bus_data_in[mObChipSel]->read(mObDataWord);
          } else {
            // Send out one IDLE if we have no data
            mObDataWord = AlpideIdle();
          }
        } else {
          if(s_dmu_fifo.num_available() > 0) {
            s_dmu_fifo.read(mObDataWord);
          } else {
            // Send out one IDLE if we have no data
            mObDataWord = AlpideIdle();
          }
        }
        mObDwBytesRemaining = mObDataWord.size;

        if(mObDataWord.data_type == ALPIDE_CHIP_EMPTY_FRAME ||
           mObDataWord.data_type == ALPIDE_CHIP_TRAILER) {
          // If this is a CHIP_TRAILER og CHIP_EMPTY_FRAME data word then we
          // should also transmit one of the "IDLE filler bytes" following the
          // actual data word
          mObDwBytesRemaining++;

          // And give away "token" (ie going to the next chip) after
          // transmitting the data word
          if(mObChipSel == mObSlaveCount) {
            mObNextChipSel = 0;
          } else {
            mObNextChipSel = mObChipSel+1;
          }
        } else if(mObDataWord.data_type == ALPIDE_CHIP_HEADER) {
          // Update trigger id signal used by AlpideDataParser to know
          // which trigger ID the data belongs to
          s_serial_data_trig_id = mObDataWord.trigger_id;
        }
      }

      dw_dtu_fifo_input = mObDataWord.data[mObDwByteIndex] << 16;

      mObDwByteIndex--;
      mObDwBytesRemaining--;
    }
  }
  // --------------------------
  // Inner barrel chip (master)
  // --------------------------
  else if(mObMode == false) {
    // Prioritize busy words over data words
    if(s_busy_fifo.num_available() > 0) {
      s_busy_fifo.nb_read(data_word);
    } else if(s_dmu_fifo.num_available() > 0) {
      s_dmu_fifo.nb_read(data_word);
    }

    dw_dtu_fifo_input = data_word.data[2] << 16 |
                        data_word.data[1] << 8 |
                        data_word.data[0];
  }


  // --------------------------
  // DTU encoding delay
  // --------------------------

  // If delaying of data through DTU FIFO is enabled (to simulate encoding
  // delay in DTU), then read data from DTU FIFO output, or IDLE if FIFO is
  // empty.
  if(mEnableDtuDelay) {
    s_dtu_delay_fifo.nb_write(dw_dtu_fifo_input);
    if(s_dtu_delay_fifo.nb_read(dw_dtu_fifo_output) == false) {
      sc_uint<24> dw_idle_data = ((uint32_t) DW_IDLE << 16) |
                                 ((uint32_t) DW_IDLE << 8) |
                                  (uint32_t) DW_IDLE;
      dw_dtu_fifo_output = dw_idle_data;
    }
  } else {
    dw_dtu_fifo_output = dw_dtu_fifo_input;
  }


  // --------------------------
  // Data output
  // --------------------------

  // Put data on data initiator socket output
  DataPayload socket_dw;

  // Send out 1 byte per 40MHz cycle in OB mode, 3 bytes in IB mode
  // Only the most significant byte is used in the FIFO in OB mode
  socket_dw.data.push_back(dw_dtu_fifo_output >> 16);
  if(mObMode == false) {
    socket_dw.data.push_back((dw_dtu_fifo_output >> 8) & 0xFF );
    socket_dw.data.push_back(dw_dtu_fifo_output & 0xFF);
  }

  s_data_output->put(socket_dw);

  // Debug signal of DTU FIFO input just for adding to VCD trace
  s_serial_data_dtu_input_debug = dw_dtu_fifo_input;

  s_serial_data_out = dw_dtu_fifo_output;
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

  bool new_busy_status = (s_frame_fifo_busy || s_multi_event_buffers_busy);

  // The internal busy signal needs to be asserted for a minimum number of cycles
  // (mMinBusyCycles, equivalent to reg 0x001B BUSY min width in real chip),
  // before the busy signal is asserted and BUSY_ON is transmitted.
  if(new_busy_status == true && new_busy_status != s_busy_status) {
    if(mBusyCycleCount == mMinBusyCycles) {
      mBusyTransitions++;
      s_busy_status = true;
    }
    mBusyCycleCount++;
  } else if(new_busy_status == false) {
    mBusyCycleCount = 0;
    s_busy_status = false;
  }

  // For OB masters: Also check the slave chips' busy lines,
  // and assert the busy status if any of the slaves are busy
  if(mObMode & mObMaster) {
    // Check busy status of slave chips in OB
    for(auto busy_it = s_local_busy_in.begin(); busy_it != s_local_busy_in.end(); busy_it++)
      s_busy_status = s_busy_status || busy_it->read();
  }
}


void Alpide::busyFifoMethod(void)
{
  AlpideDataWord dw_busy;

  if(s_busy_status) {
    dw_busy = AlpideBusyOn();
  } else {
    dw_busy = AlpideBusyOff();
  }

  // In the unlikely (should be impossible) case that
  // this FIFO is full, just read out and discard the oldest
  // word to make room for the new one.
  // In the real chip the busy FSM probably waits, but for the
  // sake of simulation speed it is simplified here.
  if(s_busy_fifo.num_free() == 0) {
    AlpideDataWord dummy_read;
    s_busy_fifo.nb_read(dummy_read);
  }

  s_busy_fifo.nb_write(dw_busy);
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
  addTrace(wf, alpide_name_prefix, "strobe_n", s_strobe_n);
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
  addTrace(wf, alpide_name_prefix, "busy_fifo_size", s_busy_fifo_size);

//  addTrace(wf, alpide_name_prefix, "frame_start_fifo", s_frame_start_fifo);
//  addTrace(wf, alpide_name_prefix, "frame_end_fifo", s_frame_end_fifo);
  addTrace(wf, alpide_name_prefix, "serial_data_dtu_input_debug", s_serial_data_dtu_input_debug);

  addTrace(wf, alpide_name_prefix, "busy_transition_count", mBusyTransitions);
  addTrace(wf, alpide_name_prefix, "busy_violation_count", mBusyViolations);
  addTrace(wf, alpide_name_prefix, "flushed_incomplete_count", mFlushedIncompleteCount);

  mTRU->addTraces(wf, alpide_name_prefix);

  for(int i = 0; i < N_REGIONS; i++)
    mRRUs[i]->addTraces(wf, alpide_name_prefix);

}
