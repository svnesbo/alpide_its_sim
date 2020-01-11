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
///@param[in] global_chip_id Global chip ID that uniquely identifies chip in simulation
///@param[in] local_chip_id Chip ID that identifies chip in the stave or module
///@param[in] chip_cfg Chip configuration
///@param[in] outer_barrel_mode True: outer barrel mode. False: inner barrel mode
///@param[in] outer_barrel_master Only relevant if in OB mode.
///           True: OB master. False: OB slave
///@param[in] outer_barrel_slave_count Number of slave chips connected to outer barrel master
///@param[in] min_busy_cycles Minimum number of cycles that the internal busy signal has to be
///           asserted before the chip transmits BUSY_ON
Alpide::Alpide(sc_core::sc_module_name name, const int global_chip_id, const int local_chip_id,
               const AlpideConfig& chip_cfg, bool outer_barrel_mode, bool outer_barrel_master,
               int outer_barrel_slave_count)
  : sc_core::sc_module(name)
  , s_control_input("s_control_input")
  , s_data_output("s_data_output")
  , s_chip_ready_out("chip_ready_out")
  , s_local_bus_data_in(outer_barrel_slave_count)
  , s_local_busy_in(outer_barrel_slave_count)
  , s_dmu_fifo(DMU_FIFO_SIZE)
  , s_dtu_delay_fifo(chip_cfg.dtu_delay_cycles+1)
  , s_dtu_delay_fifo_trig(chip_cfg.dtu_delay_cycles+1)
  , s_busy_fifo(BUSY_FIFO_SIZE)
  , s_frame_start_fifo(TRU_FRAME_FIFO_SIZE)
  , s_frame_end_fifo(TRU_FRAME_FIFO_SIZE)
  , mGlobalChipId(global_chip_id)
  , mLocalChipId(local_chip_id)
  , mChipContinuousMode(chip_cfg.chip_continuous_mode)
  , mStrobeExtensionEnable(chip_cfg.strobe_extension)
  , mStrobeLengthNs(chip_cfg.strobe_length_ns)
  , mMinBusyCycles(chip_cfg.min_busy_cycles)
  , mObMode(outer_barrel_mode)
  , mObMaster(outer_barrel_master)
  , mObSlaveCount(outer_barrel_slave_count)
{
  mEnableDtuDelay = chip_cfg.dtu_delay_cycles > 0;

  s_chip_ready_out(s_chip_ready_internal);
  s_local_busy_out(s_busy_status);

  s_serial_data_out_exp(s_serial_data_out);
  s_serial_data_trig_id_exp(s_serial_data_trig_id);

  s_local_bus_data_out(s_dmu_fifo);

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

  mTRU = new TopReadoutUnit("TRU", global_chip_id, local_chip_id);

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
                                     chip_cfg.matrix_readout_speed,
                                     chip_cfg.data_long_en);

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
    s_dtu_delay_fifo_trig.nb_write(0);
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

  mTriggersReceived++;

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

    if(mChipContinuousMode) {
      if(s_frame_fifo_busy.read() == true) {
        // Reject events if frame FIFO is at or above ALMOST_FULL1 watermark

        // End strobe interval immediately in this case
        E_strobe_interval_done.cancel();
        E_strobe_interval_done.notify(0, SC_NS);

        mTriggersRejected++;
        mBusyViolations++;
        s_chip_ready_internal = false;
        s_busy_violation = true;
      } else if(getNumEvents() == 3) {
        // End strobe interval immediately in this case
        E_strobe_interval_done.cancel();
        E_strobe_interval_done.notify(0, SC_NS);

        // Reject events if all MEBs are full in continuous mode.
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
        // Flush oldest event to make room if we are becoming full in continuous mode
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
    else if(!mChipContinuousMode) {
      s_flushed_incomplete = false; // No flushing in triggered mode

      if(s_frame_fifo_busy.read() == true) {
        // Reject events if frame FIFO is at or above ALMOST_FULL1 watermark

        // End strobe interval immediately in this case
        E_strobe_interval_done.cancel();
        E_strobe_interval_done.notify(0, SC_NS);

        mTriggersRejected++;
        mBusyViolations++;
        s_chip_ready_internal = false;
        s_busy_violation = true;
      } else if(getNumEvents() == 3) {
        // All MEBs are full - busy violation
        //
        // End strobe interval immediately in this case
        E_strobe_interval_done.cancel();
        E_strobe_interval_done.notify(0, SC_NS);

        mTriggersRejected++;
        mBusyViolations++;
        s_chip_ready_internal = false;
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
        std::cout << "@ " << time_now << " ns:\t" << "Alpide global chip ID: " << mGlobalChipId;
        std::cout << " exited data overrun mode." << std::endl;
      }

      s_frame_fifo_busy = false;
      s_readout_abort = false;
    } else if(frame_start_fifo_full) {
      // FATAL, TRU FRAME FIFO will now overflow
      s_frame_fifo_busy = true;
      s_readout_abort = true;

      if(s_fatal_state == false) {
        std::cout << "@ " << time_now << " ns:\t" << "Alpide global chip ID: " << mGlobalChipId;
        std::cout << " entered fatal mode." << std::endl;
      }

      ///@todo The FATAL overflow bit/signal has to be cleared by a RORST/GRST command
      ///      in the Alpide chip, it will not be cleared by automatically.
      s_fatal_state = true;
    } else if(frame_start_fifo_size >= TRU_FRAME_FIFO_ALMOST_FULL2) {
      // DATA OVERRUN MODE
      ///@todo Need to clear RRU FIFOs, and MEBs when entering this state

      if(s_readout_abort == false) {
        std::cout << "@ " << time_now << " ns:\t" << "Alpide global chip ID: " << mGlobalChipId;
        std::cout << " entered data overrun mode." << std::endl;
      }

      s_frame_fifo_busy = true;
      s_readout_abort = true;
    } else if(frame_start_fifo_size >= TRU_FRAME_FIFO_ALMOST_FULL1) {
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

      ///@todo Should the busy_transition flag should only be set when
      ///      the chip goes into or out of busy state.. not implemented yet
      mNextFrameEndWord.busy_transition = false;

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
  uint64_t time_now = sc_time_stamp().value();

  // Trace signals for fifo sizes
  s_dmu_fifo_size = s_dmu_fifo.num_available();
  s_busy_fifo_size = s_busy_fifo.num_available();



  if(mObMode && !mObMaster) {
    return; // Outer barrel slave does not transmit data
  }

  sc_uint<24> dw_dtu_fifo_input;
  sc_uint<24> dw_dtu_fifo_output;
  uint64_t    trig_dtu_delay_fifo_output;



  // -------------------
  // Outer barrel master
  // -------------------
  if(mObMode && mObMaster) {

    // Prioritize busy words over data words, but don't break up data words
    if((s_busy_fifo.num_available() > 0) && mObDwBytesRemaining == 0) {
      AlpideDataWord data_word = AlpideIdle();

      s_busy_fifo.nb_read(data_word);
      dw_dtu_fifo_input = data_word.data[2] << 16;
    }
    else { // Data

      // If we're not currently processing a 24-bit word,
      // read a new 24-bit word from chip that currently has the "token"
      if(mObDwBytesRemaining == 0) {
        mObDwByteIndex = 2; // Data words are transmitted with most significant byte first
        mObChipSel = mObNextChipSel;

        if(mObChipSel < mObSlaveCount) {
          if(s_local_bus_data_in[mObChipSel]->num_available() > 0) {
            s_local_bus_data_in[mObChipSel]->nb_read(mObDataWord);
          } else {
            // Send out one IDLE if we have no data
            mObDataWord = AlpideIdle();
          }
        } else {
          // Transmit our own data (from this master chip)
          // when mObChipSel == mObSlaveCount
          if(s_dmu_fifo.num_available() > 0) {
            s_dmu_fifo.nb_read(mObDataWord);
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
          // Slave chips: mObSlaveCount > mObChipSel >= 0
          // Master chip: mObChipSel == mObSlaveCount
          if(mObChipSel == mObSlaveCount) {
            mObNextChipSel = 0;
          } else {
            mObNextChipSel = mObChipSel+1;
          }
        }

        if(mObDataWord.data_type == ALPIDE_CHIP_HEADER ||
           mObDataWord.data_type == ALPIDE_CHIP_EMPTY_FRAME) {
          // Update trigger id signal used by AlpideDataParser to know
          // which trigger ID the data belongs to. Delay with DTU cycles
          // so that it comes out at the same time as the corresponding data
          mDataOutTrigId = mObDataWord.trigger_id;
        } else if(mObDataWord.data_type == ALPIDE_DATA_SHORT) {
          // When DATA_SHORT/LONG are finally put out on the DTU FIFO, we can be sure that
          // the pixels in the data word was read out, and can increase readout counters.
          ///@todo Use dynamic cast here?
          static_cast<AlpideDataShort*>(&mObDataWord)->increasePixelReadoutCount();
#ifdef PIXEL_DEBUG
          mObDataWord.mPixel->mAlpideDataOut = true;
          mObDataWord.mPixel->mAlpideDataOutTime = time_now;
#endif
        } else if(mObDataWord.data_type == ALPIDE_DATA_LONG) {
          ///@todo Use dynamic cast here?
          static_cast<AlpideDataLong*>(&mObDataWord)->increasePixelReadoutCount();
#ifdef PIXEL_DEBUG
          for(auto pix_it = mObDataWord.mPixels.begin(); pix_it != mObDataWord.mPixels.end(); pix_it++) {
            (*pix_it)->mAlpideDataOut = true;
            (*pix_it)->mAlpideDataOutTime = time_now;
          }
#endif
        }
      }

      // In outer barrel mode the data link is 400 Mbps, as opposed to 1200 Mbps
      // for inner barrel. To simplify the code we still send 24 bit each 40 MHz
      // clock cycle, but for OB we only fill 8 of the 24 bits (the MSB ones),
      // effectively leading to 1200/3 = 400 Mbps data rate.
      // The AlpideDataParser object that receives the data knows if it is an IB
      // or OB link, whether to expect data in all 24 bits or only the 8 MSB ones
      dw_dtu_fifo_input = mObDataWord.data[mObDwByteIndex] << 16;

      mObDwByteIndex--;
      mObDwBytesRemaining--;
    }
  }
  // --------------------------
  // Inner barrel chip (master)
  // --------------------------
  else if(mObMode == false) {
    AlpideDataWord data_word = AlpideIdle();

    // Prioritize busy words over data words
    if(s_busy_fifo.num_available() > 0) {
      s_busy_fifo.nb_read(data_word);
    } else if(s_dmu_fifo.num_available() > 0) {
      s_dmu_fifo.nb_read(data_word);
    }

    if(data_word.data_type == ALPIDE_CHIP_HEADER ||
       data_word.data_type == ALPIDE_CHIP_EMPTY_FRAME) {
      // Update trigger id signal used by AlpideDataParser to know
      // which trigger ID the data belongs to
      mDataOutTrigId = data_word.trigger_id;
    } else if(data_word.data_type == ALPIDE_DATA_SHORT) {
      // When DATA_SHORT/LONG are finally put out on the DTU FIFO, we can be sure that
      // the pixels in the data word was read out, and can increase readout counters.
      ///@todo Use dynamic cast here?
      auto data_short = static_cast<AlpideDataShort*>(&data_word);
      data_short->increasePixelReadoutCount();
#ifdef PIXEL_DEBUG
      data_short->mPixel->mAlpideDataOut = true;
      data_short->mPixel->mAlpideDataOutTime = time_now;
#endif
    } else if(data_word.data_type == ALPIDE_DATA_LONG) {
      ///@todo Use dynamic cast here?
      auto data_long = static_cast<AlpideDataLong*>(&data_word);
      data_long->increasePixelReadoutCount();
#ifdef PIXEL_DEBUG
      for(auto pix_it = data_long->mPixels.begin(); pix_it != data_long->mPixels.end(); pix_it++) {
        (*pix_it)->mAlpideDataOut = true;
        (*pix_it)->mAlpideDataOutTime = time_now;
      }
#endif
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

    s_dtu_delay_fifo_trig.nb_write(mDataOutTrigId);
    if(s_dtu_delay_fifo_trig.nb_read(trig_dtu_delay_fifo_output) == false) {
      trig_dtu_delay_fifo_output = 0;
    }

  } else {
    dw_dtu_fifo_output = dw_dtu_fifo_input;
    trig_dtu_delay_fifo_output = mDataOutTrigId;
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

  // Only output data to socket for IB chips and OB master chips
  // Socket not used by OB slave chips, and can be left unbound
  if(!mObMode || (mObMode && mObMaster))
    s_data_output->put(socket_dw);

  // Debug signal of DTU FIFO input just for adding to VCD trace
  s_serial_data_dtu_input_debug = dw_dtu_fifo_input;

  s_serial_data_out = dw_dtu_fifo_output;
  s_serial_data_trig_id = trig_dtu_delay_fifo_output;
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
  if(mChipContinuousMode) {
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

  bool internal_busy_status = (s_frame_fifo_busy || s_multi_event_buffers_busy);
  bool slave_busy_status = false;

  // For OB masters: Also check the slave chips' busy lines,
  // and assert the busy status if any of the slaves are busy
  if(mObMode && mObMaster) {
    // Check busy status of slave chips in OB
    for(auto busy_it = s_local_busy_in.begin(); busy_it != s_local_busy_in.end(); busy_it++)
      slave_busy_status = slave_busy_status || busy_it->read();
  }

  bool new_busy_status = internal_busy_status || slave_busy_status;


  if(new_busy_status == true && new_busy_status != s_busy_status) {
    if(slave_busy_status == true) {
      // Assert busy status leading to BUSY_ON transmission immediately
      // if a slave indicates that it is busy
      mBusyTransitions++;
      s_busy_status = true;
    } else if(internal_busy_status == true && mBusyCycleCount == mMinBusyCycles) {
      // For internal busy status, wait for the internal busy signal to be
      // asserted for a minimum number of cycles (mMinBusyCycles, equivalent
      // to reg 0x001B BUSY min width in real chip), before the busy signal
      // is asserted and BUSY_ON is transmitted.
      mBusyTransitions++;
      s_busy_status = true;
    }
    mBusyCycleCount++;
  } else if(new_busy_status == false) {
    mBusyCycleCount = 0;
    s_busy_status = false;
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
  ss << name_prefix << "alpide_" << mGlobalChipId << ".";
  std::string alpide_name_prefix = ss.str();

  //addTrace(wf, alpide_name_prefix, "chip_ready_out", s_chip_ready_out);
  addTrace(wf, alpide_name_prefix, "strobe_n", s_strobe_n);
  addTrace(wf, alpide_name_prefix, "chip_ready_internal", s_chip_ready_internal);
  addTrace(wf, alpide_name_prefix, "serial_data_out", s_serial_data_out);
  addTrace(wf, alpide_name_prefix, "serial_data_trig_id", s_serial_data_trig_id);
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
