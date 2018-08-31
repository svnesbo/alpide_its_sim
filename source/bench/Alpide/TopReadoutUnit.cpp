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
///@param[in] chip_id Chip ID number
TopReadoutUnit::TopReadoutUnit(sc_core::sc_module_name name, unsigned int chip_id)
  : sc_core::sc_module(name)
  , mChipId(chip_id)
  , mIdle(false)
{
  s_tru_state = IDLE;

  s_frame_start_fifo_empty = true;
  s_frame_end_fifo_empty = true;

  SC_METHOD(topRegionReadoutMethod);
  sensitive_pos << s_clk_in;
}

void TopReadoutUnit::end_of_elaboration(void)
{
  SC_METHOD(topRegionReadoutOutputMethod);
  sensitive << s_tru_state;
  sensitive << s_frame_start_fifo_output->ok_to_peek();
  sensitive << s_frame_end_fifo_output->ok_to_peek();
  sensitive << s_dmu_data_fifo_full;

  for(unsigned int region = 0; region < N_REGIONS; region++) {
    sensitive << s_region_fifo_empty_in[region];
    sensitive << s_region_valid_in[region];
  }
  dont_initialize();
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


///@brief SystemC method that controls readout from regions, should run on the 40MHz clock.
///       The regions are read out in ascending order, and each event is encapsulated with
///       a CHIP_HEADER and CHIP_TRAILER word. See the state machine diagram for a better
///       explanation.
///@todo Update state machine pictures with Alpide documentation + simplified FSM diagram
///@image html TRU_state_machine.png
void TopReadoutUnit::topRegionReadoutMethod(void)
{
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

  std::uint8_t current_state = s_tru_state.read();
  std::uint8_t next_state = current_state;

  unsigned int current_region;
  bool no_regions_valid = !getNextRegion(current_region);
  bool no_regions_empty = getNoRegionsEmpty();
  bool dmu_data_fifo_full = s_dmu_fifo_input->num_free() == 0;

  bool frame_start_fifo_empty = !s_frame_start_fifo_output->nb_can_get();
  bool frame_end_fifo_empty = !s_frame_end_fifo_output->nb_can_get();

  s_dmu_data_fifo_full = dmu_data_fifo_full;

  bool region_readout_allowed =
    !dmu_data_fifo_full &&
    !no_regions_valid &&
    !s_region_fifo_empty_in[current_region] &&
    s_region_valid_in[current_region];

  s_no_regions_empty_debug = no_regions_empty;
  s_no_regions_valid_debug = no_regions_valid;

  // Next state logic etc.
  switch(current_state) {
  case EMPTY:
    if(!frame_end_fifo_empty) {
      // "Pop" the frame from the frame FIFO
      s_frame_start_fifo_output->nb_get(mCurrentFrameStartWord);
      s_frame_end_fifo_output->nb_get(mCurrentFrameEndWord);

      next_state = IDLE;
    }
    break;

  case IDLE:
    if(!frame_start_fifo_empty) {
      s_frame_start_fifo_output->nb_peek(mCurrentFrameStartWord);
      next_state = WAIT_REGION_DATA;
    } else if(!s_frame_start_fifo_output->nb_can_get()){
      // If we are idle, and will remain idle, change to dynamic sensitivity
      // and wait for something to be added to the frame start fifo,
      // and save simulation time by not triggering on every clock cycle.
      next_trigger(s_frame_start_fifo_output->ok_to_peek());
      mIdle = true;
    }
    break;

  case WAIT_REGION_DATA:
    if(!no_regions_empty && !s_readout_abort_in)
      next_state = WAIT_REGION_DATA;
    else
      next_state = CHIP_HEADER;
    break;

  case CHIP_HEADER:
    if(!dmu_data_fifo_full) {
      if(mCurrentFrameStartWord.busy_violation) {
        // Busy violation frame
        // Since no frame end word is added in busy violation, we always
        // have to visit this state, and not the normal chip trailer state,
        // even in readout abort (data overrun) mode.
        data_out = AlpideChipHeader(mChipId, mCurrentFrameStartWord);
        next_state = BUSY_VIOLATION;
      } else if(s_readout_abort_in) {
        data_out = AlpideChipHeader(mChipId, mCurrentFrameStartWord);
        next_state = CHIP_TRAILER;
      } else if(!no_regions_valid) {
        // Normal data frame
        data_out = AlpideChipHeader(mChipId, mCurrentFrameStartWord);
        next_state = REGION_DATA;
      } else {
        // Empty frame
        data_out = AlpideChipEmptyFrame(mChipId, mCurrentFrameStartWord);
        next_state = EMPTY;
      }
      s_dmu_fifo_input->nb_write(data_out);
    }
    break;


  case BUSY_VIOLATION:
    s_frame_start_fifo_output->nb_get(mCurrentFrameStartWord);

    data_out = AlpideChipTrailer(mCurrentFrameStartWord,
                                 busyv_frame_end_word,
                                 s_fatal_state_in,
                                 s_readout_abort_in);

    s_dmu_fifo_input->nb_write(data_out);
    next_state = IDLE;
    break;

  case REGION_DATA:
    if(region_readout_allowed) {
      data_out = s_region_data_in[current_region];
      s_dmu_fifo_input->nb_write(data_out);
    }

    if(dmu_data_fifo_full) {
      next_state = WAIT;
    } else if(no_regions_valid) {
      next_state = CHIP_TRAILER;
    }
    break;


  case WAIT: // Data FIFO full or waiting for more region data
    if(s_readout_abort_in || no_regions_valid)
      next_state = CHIP_TRAILER;
    else if(dmu_data_fifo_full || s_region_fifo_empty_in[current_region])
      next_state = WAIT;
    else
      next_state = REGION_DATA;
    break;

  case CHIP_TRAILER:
    if(!frame_end_fifo_empty && !dmu_data_fifo_full) {
      // "Pop" the frame from the frame FIFO
      s_frame_start_fifo_output->nb_get(mCurrentFrameStartWord);
      s_frame_end_fifo_output->nb_get(mCurrentFrameEndWord);

      // The fatal and abort parameters tell the AlpideChipTrailer constructor
      // to overwrite the readout flags with the special combination of readout
      // flags that indicate abort/fatal (see Alpide manual)
      data_out = AlpideChipTrailer(mCurrentFrameStartWord,
                                   mCurrentFrameEndWord,
                                   s_fatal_state_in,
                                   s_readout_abort_in);

      s_dmu_fifo_input->nb_write(data_out);

      next_state = IDLE;
    }
    break;
  }

  // nb_can_get() returns immediately if we can get something off the fifo, without delay,
  // since it's a nonblocking interface. We need the empty signal to be delayed by a cycle
  // in next state logic though, so using a signal here.
  s_frame_start_fifo_empty = !s_frame_start_fifo_output->nb_can_get();
  s_frame_end_fifo_empty = !s_frame_end_fifo_output->nb_can_get();

  s_tru_state = next_state;

  // "Reset" the previous region counter when all regions are read out
  if(no_regions_valid)
    s_previous_region = 0;
  else
    s_previous_region = current_region;
}


///@brief Moore FSM style output method for the TRU FSM. Sensitive to s_tru_state
void TopReadoutUnit::topRegionReadoutOutputMethod(void)
{
  unsigned int current_region;
  bool no_regions_valid = !getNextRegion(current_region);
  bool frame_start_fifo_empty = !s_frame_start_fifo_output->nb_can_get();
  bool frame_end_fifo_empty = !s_frame_end_fifo_output->nb_can_get();

  bool region_readout_allowed =
    !s_dmu_data_fifo_full.read() &&
    !no_regions_valid &&
    !s_region_fifo_empty_in[current_region] &&
    s_region_valid_in[current_region];

  // New region? Make sure region data read signal for previous region was set low then
  if(current_region != s_previous_region.read())
    s_region_data_read_out[s_previous_region.read()] = false;

  // Output logic
  // Based on next state to achieve combinatorial output based on
  // the state we're in, without delaying the output with a clock cycle
  switch(s_tru_state.read()) {
  case EMPTY:
    // Using frame_end_fifo_empty, not s_frame_end_fifo_empty, to know ahead in time
    // the status of s_frame_end_fifo_empty in next state, so we can set the output
    s_region_event_pop_out = !frame_end_fifo_empty;
    s_region_event_start_out = false;
    s_region_data_read_out[current_region] = false;
    break;

  case IDLE:
    // Using frame_start_fifo_empty, not s_frame_start_fifo_empty, to know ahead in time
    // the status of s_frame_start_fifo_empty in next state, so we can set the output
    s_region_event_start_out = !frame_start_fifo_empty;
    s_region_event_pop_out = false;
    s_region_data_read_out[current_region] = false;
    break;

  case WAIT_REGION_DATA:
    s_region_event_pop_out = false;
    s_region_event_start_out = false;
    s_region_data_read_out[current_region] = false;
    break;

  case CHIP_HEADER:
    s_region_event_pop_out = false;
    s_region_event_start_out = false;
    s_region_data_read_out[current_region] =
      !s_dmu_data_fifo_full &&
      !no_regions_valid &&
      !s_region_fifo_empty_in[current_region];
    break;


  case BUSY_VIOLATION:
    s_region_event_pop_out = false;
    s_region_event_start_out = false;
    s_region_data_read_out[current_region] = false;
    break;

  case REGION_DATA:
    s_region_event_pop_out = false;
    s_region_event_start_out = false;
    s_region_data_read_out[current_region] = region_readout_allowed;
    break;


  case WAIT: // Data FIFO full or waiting for more region data
    s_region_event_pop_out = false;
    s_region_event_start_out = false;
    s_region_data_read_out[current_region] =
      !s_dmu_data_fifo_full &&
      !no_regions_valid &&
      !s_region_fifo_empty_in[current_region];
    break;

  case CHIP_TRAILER:
    // Using frame_end_fifo_empty, not s_frame_end_fifo_empty, to know ahead in time
    // the status of s_frame_end_fifo_empty in next state, so we can set the output
    s_region_event_pop_out = !s_frame_end_fifo_empty && !s_dmu_data_fifo_full;
    s_region_event_start_out = false;
    s_region_data_read_out[current_region] = false;
    break;
  }
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
//  addTrace(wf, tru_name_prefix, "dmu_fifo_input", s_dmu_fifo_input);

  addTrace(wf, tru_name_prefix, "no_regions_empty_debug", s_no_regions_empty_debug);
  addTrace(wf, tru_name_prefix, "no_regions_valid_debug", s_no_regions_valid_debug);

  addTrace(wf, tru_name_prefix, "frame_start_fifo_empty", s_frame_start_fifo_empty);
  addTrace(wf, tru_name_prefix, "frame_end_fifo_empty", s_frame_end_fifo_empty);

  addTrace(wf, tru_name_prefix, "dmu_data_fifo_full", s_dmu_data_fifo_full);

  addTrace(wf, tru_name_prefix, "tru_state", s_tru_state);
  addTrace(wf, tru_name_prefix, "previous_region", s_previous_region);

  // This is an array, have to iterate over it.. but the same signal is already added in the RRUs
  //addTrace(wf, tru_name_prefix, "region_data_read_out", s_region_data_read_out);
}
