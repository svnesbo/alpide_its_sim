/**
 * @file   TopReadoutUnit.cpp
 * @author Simon Voigt Nesbo
 * @date   February 20, 2017
 * @brief  Class for implementing the Top Readout Unit (TRU) in the Alpide chip.
 */

#include "TopReadoutUnit.hpp"
#include "../misc/vcd_trace.hpp"


SC_HAS_PROCESS(TopReadoutUnit);
///@brief Constructor for TopReadoutUnit
///@param[in] name SystemC module name
//////@param[in] global_chip_id Global chip ID that uniquely identifies chip in simulation
///@param[in] local_chip_id Chip ID that identifies chip in the stave or module
TopReadoutUnit::TopReadoutUnit(sc_core::sc_module_name name,
                               const unsigned int global_chip_id,
                               const unsigned int local_chip_id,
                               std::shared_ptr<std::map<AlpideDataType, uint64_t>> data_word_count)
  : sc_core::sc_module(name)
  , mGlobalChipId(global_chip_id)
  , mLocalChipId(local_chip_id)
  , mIdle(false)
  , mDataWordCount(data_word_count)
{
  s_tru_current_state = IDLE;
  s_tru_next_state = IDLE;
  s_write_dmu_fifo = false;
  s_frame_start_fifo_empty = true;
  s_frame_end_fifo_empty = true;
  s_tru_data = AlpideIdle();

  SC_METHOD(topRegionReadoutOutputNextState);
  sensitive << E_update_fsm;

  SC_METHOD(topRegionReadoutStateUpdate);
  sensitive_pos << s_clk_in;
}


///@brief Find the first valid region, and return its region id.
///@param[out] region_out Reference to an integer that will hold the region id.
///@return True if a valid region was found.
bool TopReadoutUnit::getNextRegion(unsigned int& region_out)
{
  for(int i = 0; i < N_REGIONS; i++) {
    if(s_region_valid_in[i] == true) {
      region_out = i;
      return true;
    }
  }
  region_out = 0;
  return false;
}


///@brief OR all region empty signals together and return inverse
///@return true if no regions are empty
bool TopReadoutUnit::getNoRegionsEmpty(void)
{
  bool or_empty = false;

  for(int i = 0; i < N_REGIONS; i++) {
    or_empty = or_empty || s_region_fifo_empty_in[i];
  }

  return !or_empty;
}


///@brief SystemC method for updating the current state of the TRU's FSM
void TopReadoutUnit::topRegionReadoutStateUpdate(void)
{
  s_tru_current_state = s_tru_next_state;

  E_update_fsm.notify(12.5, SC_NS);


  AlpideDataWord data_out;
  std::uint64_t time_now = sc_time_stamp().value();
  if(s_write_dmu_fifo) {
    s_dmu_fifo_input->nb_write(s_tru_data.read());

    data_out = s_tru_data.read();

    (*mDataWordCount)[data_out.data_type]++;

#ifdef PIXEL_DEBUG
    if(data_out.data_type == ALPIDE_REGION_TRAILER) {
      std::cerr << "@" << time_now << "ns: Global chip ID " << mGlobalChipId;
      std::cerr << " TRU: Oops, just read out REGION_TRAILER" << std::endl;
    } else if(data_out.data_type == ALPIDE_DATA_SHORT) {
      auto data_short = static_cast<AlpideDataShort*>(&data_out);
      data_short->mPixel->mTRU = true;
      data_short->mPixel->mTRUTime = time_now;
    } else if(data_out.data_type == ALPIDE_DATA_LONG) {
      auto data_long = static_cast<AlpideDataLong*>(&data_out);

      for(auto pix_it = data_long->mPixels.begin(); pix_it != data_long->mPixels.end(); pix_it++) {
        (*pix_it)->mTRU = true;
        (*pix_it)->mTRUTime = time_now;
      }
    }
#endif
  }
}


///@brief SystemC method that controls readout from regions, should run on the 40MHz clock.
///       The regions are read out in ascending order, and each event is encapsulated with
///       a CHIP_HEADER and CHIP_TRAILER word. See the state machine diagram for a better
///       explanation.
///       This methods controls outputs based on the current state and inputs, and calculates
///       the next state.
///@todo Update state machine pictures with Alpide documentation + simplified FSM diagram
///@image html TRU_state_machine.png
void TopReadoutUnit::topRegionReadoutOutputNextState(void)
{
  std::uint64_t time_now = sc_time_stamp().value();
  // If we were idle with dynamic sensitivity enabled,
  // revert back to static sensitivity now that something happened.
  // Skip one clock cycle since dynamic sensitivity would make us
  // trigger the cycle before we would have registered the change
  // if we were sensitive to the clock.
  if(mIdle) {
    next_trigger(); // Revert to static sensitivity
    mIdle = false;
    return;
  }

  AlpideDataWord data_out;

  // Busy violation bit is included in frame start word
  // The bits in the frame end word are all false in busy violation
  const FrameEndFifoWord busyv_frame_end_word = {false, false, false};

  unsigned int current_region;
  bool no_regions_valid = !getNextRegion(current_region);
  bool no_regions_empty = getNoRegionsEmpty();
  bool dmu_data_fifo_full = s_dmu_fifo_input->num_free() <= 1;
  bool dmu_data_fifo_empty = s_dmu_fifo_input->num_free() == DMU_FIFO_SIZE;

  bool frame_start_fifo_empty = !s_frame_start_fifo_output->nb_can_get();
  bool frame_end_fifo_empty = !s_frame_end_fifo_output->nb_can_get();

  s_frame_start_fifo_empty = frame_start_fifo_empty;
  s_frame_end_fifo_empty = frame_end_fifo_empty;

  bool region_readout_allowed =
    !dmu_data_fifo_full &&
    !no_regions_valid &&
    !s_region_fifo_empty_in[current_region] &&
    s_region_valid_in[current_region];

  s_no_regions_empty_debug = no_regions_empty;
  s_no_regions_valid_debug = no_regions_valid;
  s_dmu_data_fifo_full = dmu_data_fifo_full;
  s_dmu_data_fifo_empty = dmu_data_fifo_empty;

  // New region? Make sure region data read signal for previous region was set low then
  if(current_region != s_previous_region.read())
    s_region_data_read_out[s_previous_region.read()] = false;

  s_write_dmu_fifo = false;

  // Next state logic etc.
  switch(s_tru_current_state.read()) {
  case EMPTY:
    if(!frame_end_fifo_empty) {
      // "Pop" the frame from the frame FIFO
      s_frame_start_fifo_output->nb_get(mCurrentFrameStartWord);
      s_frame_end_fifo_output->nb_get(mCurrentFrameEndWord);

      s_tru_next_state = IDLE;
    }
    s_region_event_pop_out = !frame_end_fifo_empty;
    s_region_event_start_out = false;
    s_region_data_read_out[current_region] = false;
    s_region_data_read_debug = false;
    s_write_dmu_fifo = false;
    break;

  case IDLE:
    if(!frame_start_fifo_empty) {
      s_frame_start_fifo_output->nb_peek(mCurrentFrameStartWord);
      s_tru_next_state = WAIT_REGION_DATA;
    } else if(!s_frame_start_fifo_output->nb_can_get()){
      // If we are idle, and will remain idle, change to dynamic sensitivity
      // and wait for something to be added to the frame start fifo,
      // and save simulation time by not triggering on every clock cycle.
      next_trigger(s_frame_start_fifo_output->ok_to_peek());
      mIdle = true;
    }
    s_region_event_start_out = !frame_start_fifo_empty;
    s_region_event_pop_out = false;
    s_region_data_read_out[current_region] = false;
    s_region_data_read_debug = false;
    s_write_dmu_fifo = false;
    break;

  case WAIT_REGION_DATA:
    if(!no_regions_empty && !s_readout_abort_in)
      s_tru_next_state = WAIT_REGION_DATA;
    else
      s_tru_next_state = CHIP_HEADER;

    s_region_event_pop_out = false;
    s_region_event_start_out = false;
    s_region_data_read_out[current_region] = false;
    s_region_data_read_debug = false;
    s_write_dmu_fifo = false;
    break;

  case CHIP_HEADER:
    if(!dmu_data_fifo_full) {
      if(mCurrentFrameStartWord.busy_violation) {
        // Busy violation frame
        // Since no frame end word is added in busy violation, we always
        // have to visit this state, and not the normal chip trailer state,
        // even in readout abort (data overrun) mode.
        s_tru_data = AlpideChipHeader(mLocalChipId, mCurrentFrameStartWord);
        s_write_dmu_fifo = true;
        s_tru_next_state = BUSY_VIOLATION;
      } else if(s_readout_abort_in) {
        s_tru_data = AlpideChipHeader(mLocalChipId, mCurrentFrameStartWord);
        s_write_dmu_fifo = true;
        s_tru_next_state = CHIP_TRAILER;
      } else if(!no_regions_valid && no_regions_empty) {
        // Normal data frame
        s_tru_data = AlpideChipHeader(mLocalChipId, mCurrentFrameStartWord);
        s_write_dmu_fifo = true;
        s_tru_next_state = REGION_DATA;
      } else if(no_regions_valid){
        // Empty frame
        s_tru_data = AlpideChipEmptyFrame(mLocalChipId, mCurrentFrameStartWord);
        s_write_dmu_fifo = true;
        s_tru_next_state = EMPTY;
      }
    }

    s_region_event_pop_out = false;
    s_region_event_start_out = false;

    s_region_data_read_out[current_region] =
      !dmu_data_fifo_full &&
      !no_regions_valid &&
      no_regions_empty;

    s_region_data_read_debug =
      !dmu_data_fifo_full &&
      !no_regions_valid &&
      no_regions_empty;
    break;


  case BUSY_VIOLATION:
    s_tru_next_state = IDLE;

    s_frame_start_fifo_output->nb_get(mCurrentFrameStartWord);
    s_tru_data = AlpideChipTrailer(mCurrentFrameStartWord,
                                   busyv_frame_end_word,
                                   s_fatal_state_in,
                                   s_readout_abort_in);

    s_region_event_pop_out = false;
    s_region_event_start_out = false;
    s_region_data_read_out[current_region] = false;
    s_region_data_read_debug = false;
    s_write_dmu_fifo = true;
    break;

  case REGION_DATA:
    if(s_readout_abort_in || no_regions_valid) {
      s_tru_next_state = CHIP_TRAILER;
    } else if(dmu_data_fifo_full || s_region_fifo_empty_in[current_region]) {
      s_tru_next_state = WAIT;
    }

    s_tru_data = s_region_data_in[current_region];
    s_region_event_pop_out = false;
    s_region_event_start_out = false;
    s_region_data_read_out[current_region] = region_readout_allowed;
    s_region_data_read_debug = region_readout_allowed;
    //s_write_dmu_fifo = true;
    s_write_dmu_fifo = region_readout_allowed;
    break;


  case WAIT: // Data FIFO full or waiting for more region data
    if(s_readout_abort_in || no_regions_valid)
      s_tru_next_state = CHIP_TRAILER;
    else if(dmu_data_fifo_full || s_region_fifo_empty_in[current_region])
      s_tru_next_state = WAIT;
    else
      s_tru_next_state = REGION_DATA;

    s_tru_data = s_region_data_in[current_region];
    s_region_event_pop_out = false;
    s_region_event_start_out = false;
    s_region_data_read_out[current_region] = region_readout_allowed;
    s_region_data_read_debug = region_readout_allowed;
    s_write_dmu_fifo = region_readout_allowed;
    break;



  case CHIP_TRAILER:
    if(!frame_end_fifo_empty && !dmu_data_fifo_full) {
      // "Pop" the frame from the frame FIFO
      s_frame_start_fifo_output->nb_get(mCurrentFrameStartWord);
      s_frame_end_fifo_output->nb_get(mCurrentFrameEndWord);

      // The fatal and abort parameters tell the AlpideChipTrailer constructor
      // to overwrite the readout flags with the special combination of readout
      // flags that indicate abort/fatal (see Alpide manual)
      s_tru_data = AlpideChipTrailer(mCurrentFrameStartWord,
                                     mCurrentFrameEndWord,
                                     s_fatal_state_in,
                                     s_readout_abort_in);
      s_tru_next_state = IDLE;
    }

    s_region_event_pop_out = !frame_end_fifo_empty && !dmu_data_fifo_full;
    s_region_event_start_out = false;
    s_region_data_read_out[current_region] = false;
    s_region_data_read_debug = false;
    s_write_dmu_fifo = !dmu_data_fifo_full && !frame_end_fifo_empty;
    break;
  }

  // "Reset" the previous region counter when all regions are read out
  if(no_regions_valid)
    s_previous_region = 0;
  else
    s_previous_region = current_region;
}


///@brief Add SystemC signals to log in VCD trace file.
///@param[in,out] wf Pointer to VCD trace file object
///@param[in] name_prefix Name prefix to be added to all the trace names
void TopReadoutUnit::addTraces(sc_trace_file *wf, std::string name_prefix) const
{
  std::stringstream ss;
  ss << name_prefix << "TRU.";
  std::string tru_name_prefix = ss.str();

  addTrace(wf, tru_name_prefix, "readout_abort_in", s_readout_abort_in);
  addTrace(wf, tru_name_prefix, "fatal_state_in", s_fatal_state_in);
  addTrace(wf, tru_name_prefix, "region_event_pop_out", s_region_event_pop_out);
  addTrace(wf, tru_name_prefix, "region_event_start_out", s_region_event_start_out);
  addTrace(wf, tru_name_prefix, "region_data_read_debug", s_region_data_read_debug);

  addTrace(wf, tru_name_prefix, "no_regions_empty_debug", s_no_regions_empty_debug);
  addTrace(wf, tru_name_prefix, "no_regions_valid_debug", s_no_regions_valid_debug);

  addTrace(wf, tru_name_prefix, "frame_start_fifo_empty", s_frame_start_fifo_empty);
  addTrace(wf, tru_name_prefix, "frame_end_fifo_empty", s_frame_end_fifo_empty);

  addTrace(wf, tru_name_prefix, "dmu_data_fifo_full", s_dmu_data_fifo_full);
  addTrace(wf, tru_name_prefix, "dmu_data_fifo_empty", s_dmu_data_fifo_empty);
  addTrace(wf, tru_name_prefix, "write_dmu_fifo", s_write_dmu_fifo);
  addTrace(wf, tru_name_prefix, "tru_data", s_tru_data);

  addTrace(wf, tru_name_prefix, "tru_current_state", s_tru_current_state);
  addTrace(wf, tru_name_prefix, "tru_next_state", s_tru_next_state);
  addTrace(wf, tru_name_prefix, "previous_region", s_previous_region);
}
