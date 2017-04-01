/**
 * @file   top_readout.cpp
 * @author Simon Voigt Nesbo
 * @date   February 20, 2017
 * @brief  Class for implementing the Top Readout Unit (TRU) in the Alpide chip.
 */

#include "top_readout.h"
#include "../misc/vcd_trace.h"


SC_HAS_PROCESS(TopReadoutUnit);
TopReadoutUnit::TopReadoutUnit(sc_core::sc_module_name name, unsigned int chip_id)
  : sc_core::sc_module(name)
  , mChipId(chip_id)
{
  s_frame_start_fifo_out(s_frame_start_fifo);
  s_frame_end_fifo_out(s_frame_end_fifo);

  s_current_region = 0;

  s_tru_state = IDLE;

  SC_METHOD(topRegionReadoutProcess);
  sensitive_pos << s_clk_in;  
}


///@brief Find the first region that is not empty, and return its region id.
///@return ID of first non-empty region. Returns -1 if all regions are empty.
int TopReadoutUnit::getNextRegion(void)
{
  for(int i = 0; i < N_REGIONS; i++) {
    if(s_region_empty_in[i] == false)
      return i;
  }
  
  return -1;
}


///@brief SystemC method that controls readout from regions, should run on the 40MHz clock.
///       The regions are read out in ascending order, and each event is encapsulated with
///       a CHIP_HEADER and CHIP_TRAILER word. See the state machine diagram for a better
///       explanation.
///@todo Update state machine pictures with Alpide documentation + simplified FSM diagram
///@image html TRU_state_machine.png
void TopReadoutUnit::topRegionReadoutProcess(void)
{
  int readout_flags;
  unsigned int region;
  AlpideDataWord data_out;

  int current_region = getNextRegion();

  bool all_regions_empty = current_region == -1 ? true : false;
  bool tru_data_fifo_full = s_tru_fifo_out.num_free() == 0;
  bool frame_start_fifo_empty = frame_start_fifo.num_available() == 0;
  bool frame_end_fifo_empty = s_frame_end_fifo.num_available() == 0;

  switch(s_tru_state.read()) {
  case EMPTY:
    s_region_event_pop_out = !frame_end_fifo_empty;
    s_region_event_start = false;
    s_region_data_read_out[current_region] = false;
        
    if(!frame_end_fifo_empty) {
      s_tru_state = IDLE;
    }
    break;
      
  case IDLE:
    s_region_event_pop_out = false;    
    s_region_event_start = !frame_start_fifo_empty;
    s_region_data_read_out[current_region] = false;    
    
    if(!frame_start_fifo_empty) {
      s_frame_start_fifo.nb_read(mCurrentFrameStartWord);

      if (s_busy_violation_in || s_readout_abort_in)
        s_tru_state = CHIP_HEADER;
      else
        s_tru_state = WAIT_REGION_DATA;
    }
    break;
      
  case WAIT_REGION_DATA:
    s_region_event_pop_out = false;
    s_region_event_start = false;    
    s_region_data_read_out[current_region] = false;    

    // Wait for data to become available from regions...
    if(all_regions_empty && !s_readout_abort_in)
      s_tru_state = WAIT_REGION_DATA;
    else
      s_tru_state = CHIP_HEADER;    
    break;
      
  case CHIP_HEADER:
    s_region_event_pop_out = false;
    s_region_event_start = false;    
    s_region_data_read_out[current_region] =
      !tru_data_fifo_full &&
      s_region_valid_in[current_region] &&
      !s_region_empty_in[current_region];
    
    if(!tru_data_fifo_full) {
      if(mCurrentFrameStartWord.busy_violation) {
        // Busy violation frame
        data_out = AlpideChipHeader(mChipId, mCurrentFrameStartWord);
        s_tru_state = BUSY_VIOLATION;
      } if(!all_regions_empty) {
        // Normal data frame
        data_out = AlpideChipHeader(mChipId, mCurrentFrameStartWord);
        s_tru_state = REGION_DATA;
      } else {
        // Empty frame
        data_out = AlpideChipEmptyFrame(mChipId, mCurrentFrameStartWord);
        s_tru_state = EMPTY;
      }
      s_tru_fifo_out->nb_write(data_out);
    }
    break;

      
  case BUSY_VIOLATION:
    s_region_event_pop_out = false;
    s_region_event_start = false;
    s_region_data_read_out[current_region] = false;

    ///@todo Set busy violation flag in readout_flags
    data_out = AlpideChipTrailer(mCurrentFrameStartWord, mCurrentFrameEndWord);

    s_tru_state = IDLE;
    break;
      
  case REGION_DATA:
    s_region_event_pop_out = false;
    s_region_event_start = false;

    // New region? Output region header
    if(current_region != s_previous_region) {
      data_out = AlpideRegionHeader(region);
    } else {
      s_region_fifo_in[region]->nb_read(data_out);        
    }

    if(tru_data_fifo_full) {
      s_tru_state = WAIT;
    } else if(all_regions_empty) {
      s_tru_state = CHIP_TRAILER;
    }
    break;
      
  case WAIT:
    s_region_event_pop_out = false;
    s_region_event_start = false;
    s_region_data_read_out[current_region] =
      !tru_data_fifo_full &&
      !s_region_valid_in[current_region] &&
      !s_region_empty_in[current_region];
    
    if(!tru_data_fifo_full) {
      if(/* More region data */)
        s_tru_state = REGION_DATA;
      else /* No more region data - output chip trailer */
        s_tru_state = CHIP_TRAILER;
    }
    break;
      
  case CHIP_TRAILER:
    s_region_event_pop_out = !frame_end_fifo_empty && !tru_data_fifo_full;
    s_region_event_start = false;
    s_region_data_read_out[current_region] = false;    
    
    if(!tru_data_fifo_full || !frame_end_fifo_empty) {
      ///@todo Read  something from s_frame_end_fifo here..
      ///@todo Implement some readout flags here?
      readout_flags = 0; 
      data_out = AlpideChipTrailer(mCurrentFrameStartWord, mCurrentFrameEndWord);
      s_tru_state = IDLE;
    }
    break;
  }

  // "Reset" the previous region counter when all regions are read out
  if(all_regions_empty)
    s_previous_region = 0;
  else
    s_previous_region = current_region;
}


///@brief Add SystemC signals to log in VCD trace file.
///@param wf Pointer to VCD trace file object
///@param name_prefix Name prefix to be added to all the trace names
void TopReadoutUnit::addTraces(sc_trace_file *wf, std::string name_prefix) const
{
  std::stringstream ss;
  ss << name_prefix << "TRU.";
  std::string tru_name_prefix = ss.str();
  
  addTrace(wf, tru_name_prefix, "tru_state", s_tru_state);   
  addTrace(wf, tru_name_prefix, "current_region", s_current_region);     
}
