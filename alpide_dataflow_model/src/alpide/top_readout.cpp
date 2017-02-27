/**
 * @file   top_readout.cpp
 * @Author Simon Voigt Nesbo
 * @date   February 20, 2017
 * @brief  Class for implementing the Top Readout Unit (TRU) in the Alpide chip.
 */


#include "top_readout.h"

SC_HAS_PROCESS(TopReadoutUnit);
TopReadoutUnit::TopReadoutUnit(sc_core::sc_module_name name, unsigned int chip_id)
  : sc_core::sc_module(name)
  , mChipId(chip_id)
{
  mCurrentRegion = 0;

  s_tru_state = IDLE;
  
  SC_METHOD(topRegionReadoutProcess);
  sensitive_pos << s_clk_in;  
}


///@brief SystemC method that controls readout from regions, should run on the 40MHz clock.
///       The regions are read out in ascending order, and each event is encapsulated with
///       a CHIP_HEADER and CHIP_TRAILER word. 
void TopReadoutUnit::topRegionReadoutProcess(void)
{
  int readout_flags;
  AlpideDataWord data;
  
  // Bunch counter wraps around each orbit
  mBunchCounter++;
  if(mBunchCounter == LHC_ORBIT_BUNCH_COUNT)
    mBunchCounter = 0;
    
  if(s_tru_fifo_out->num_free() > 0) {
    switch(s_tru_state.read()) {
    case CHIP_HEADER:
      s_tru_fifo_out->nb_write(AlpideChipHeader(mChipId, mBunchCounter));
      if(s_current_event_hits_left_in.read() == 0)
        s_tru_state = CHIP_EMPTY_FRAME;
      else {
        mCurrentRegion = 0;
        s_tru_state = REGION_HEADER;
      }
      break;
      
    case CHIP_EMPTY_FRAME:
      s_tru_fifo_out->nb_write(AlpideChipEmptyFrame(mChipId, mBunchCounter));
      s_tru_state = CHIP_TRAILER;
      break;
      
    case REGION_HEADER:
      // Find the next region that has data
      while(s_region_empty_in[mCurrentRegion].read() == false &&
            s_region_fifo_in[mCurrentRegion]->num_available() == 0 &&
            mCurrentRegion < N_REGIONS)
      {
        mCurrentRegion++;
      }

      
      if(mCurrentRegion < N_REGIONS) {
        s_tru_state = REGION_DATA;        
        s_tru_fifo_out->nb_write(AlpideRegionHeader(mCurrentRegion));
        break;
      } else {
        // No break here - Allow program to continue into
        // CHIP_TRAILER state if we have read out all regions
        s_tru_state = CHIP_TRAILER;
      }

    case CHIP_TRAILER:
      ///@todo Implement some readout flags here?
      readout_flags = 0;
      s_tru_fifo_out->nb_write(AlpideChipTrailer(readout_flags));

      if(s_event_buffers_used_in.read() > 0)
        s_tru_state = CHIP_HEADER;
      else
        s_tru_state = IDLE;
      break;      

    case REGION_DATA:
      if(s_region_fifo_in[mCurrentRegion]->num_available() > 0) {
        s_region_fifo_in[mCurrentRegion]->nb_read(data);
        s_tru_fifo_out->nb_write(data);
      } else {
        // Insert IDLE if the region FIFO is currently empty
        s_tru_fifo_out->nb_write(AlpideIdle());
      }

      // Is the region empty, and was this the last word in the region FIFO?
      if(s_region_empty_in[mCurrentRegion].read() == false &&
            s_region_fifo_in[mCurrentRegion]->num_available() == 0)
      {
        if(mCurrentRegion == (N_REGIONS-1))
          s_tru_state = CHIP_TRAILER;
        else
          s_tru_state = REGION_HEADER;
      }
      break;
      
    case IDLE:
      s_tru_fifo_out->nb_write(AlpideIdle());

      if(s_event_buffers_used_in.read() > 0)
        s_tru_state = CHIP_HEADER;
      
      break;
      
    default:
      s_tru_state = IDLE;
      break;
    }
    
  } else { // TRU FIFO Full
    // Do something smart here.. do we need to signal busy?
  }
}


///@brief Add SystemC signals to log in VCD trace file.
///@param wf Pointer to VCD trace file object
///@param name_prefix Name prefix to be added to all the trace names
void TopReadoutUnit::addTraces(sc_trace_file *wf, std::string name_prefix) const
{
  std::stringstream ss;
  ss << name_prefix << "TRU/";
  std::string tru_name_prefix = ss.str();

  ss.str("");
  ss << tru_name_prefix << "tru_state";
  std::string str_tru_state(ss.str());
  sc_trace(wf, s_tru_state, str_tru_state);
  
  ss.str("");
  ss << tru_name_prefix << "current_event_hits_left_in";
  std::string str_current_event_hits_left_in(ss.str());
  sc_trace(wf, s_current_event_hits_left_in, str_current_event_hits_left_in);  

  ss.str("");
  ss << tru_name_prefix << "event_buffers_used_in";  
  std::string str_event_buffers_used_in(ss.str());
  sc_trace(wf, s_event_buffers_used_in, str_event_buffers_used_in);    
  
  for(int i = 0; i < N_REGIONS; i++) {
    ss.str("");
    ss << tru_name_prefix << "region_empty_in_" << i;
    std::string str_region_empty_in(ss.str());
    sc_trace(wf, s_region_empty_in[i], str_region_empty_in);
  }
}
