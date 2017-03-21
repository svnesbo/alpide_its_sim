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
  s_current_region = 0;

  s_tru_state = IDLE;

  s_busy_on_signalled = false;

  // Avoid sending BUSY OFF upon startup
  s_busy_off_signalled = true;
  
  SC_METHOD(topRegionReadoutProcess);
  sensitive_pos << s_clk_in;  
}


///@brief SystemC method that controls readout from regions, should run on the 40MHz clock.
///       The regions are read out in ascending order, and each event is encapsulated with
///       a CHIP_HEADER and CHIP_TRAILER word. See the state machine diagram for a better
///       explanation.
///@image html TRU_state_machine.png
void TopReadoutUnit::topRegionReadoutProcess(void)
{
  int readout_flags;
  unsigned int region;
  AlpideDataWord data_out;
  
  // Bunch counter wraps around each orbit
  mBunchCounter++;
  if(mBunchCounter == LHC_ORBIT_BUNCH_COUNT)
    mBunchCounter = 0;


  // See the block diagrams mentioned in the brief for this function for a better
  // explanation of how this state machine works.
  if(s_tru_fifo_out->num_free() > 0) {
    switch(s_tru_state.read()) {
    case EMPTY:
      if(s_frame_end_fifo.num_available() > 0) {
        // Read out 
        s_tru_state = IDLE;
      }
      break;
      
    case IDLE:
      if(s_frame_start_fifo.num_available() > 0)
        s_tru_state = WAIT_REGION_DATA;
      break;
      
    case WAIT_REGION_DATA:
      // Wait for data to become available from regions...
      if(/* data available, or empty chip? */)
        s_tru_state = CHIP_HEADER;
      break;
      
    case CHIP_HEADER:
      // num_free() > 0, or a threshold??
      if(s_tru_fifo_out.num_free() > 0) {
        if(/* Busy violation */) {
          // Do something smart here...?
          s_tru_state = BUSY_VIOLATION;
        } if(/* There's data: Normal Chip Header */) {
          // Which BC count should be used??? Should match when event occured, not when it is read out
          // Get the header/trailer bits from FRAME START/END FIFO HERE..
          data_out = AlpideChipHeader(mChipId, mBunchCounter);
          s_tru_state = REGION_DATA;
        } else {
          /* No data: Chip Empty Header/Frame */
          // Which BC count should be used??? Should match when event occured, not when it is read out
          // Get the header/trailer bits from FRAME START/END FIFO HERE..          
          data_out = AlpideChipEmptyFrame(mChipId, mBunchCounter);
          s_tru_state = EMPTY;
        }
        s_tru_fifo_out->nb_write(data_out);
      }
      break;

      
    case BUSY_VIOLATION:
      // Are we outputting something here???
      s_tru_state = IDLE;
      break;
      
    case REGION_DATA:
      if(/* New region */) {
        data_out = AlpideRegionHeader(region);
      } else {
        s_region_fifo_in[region]->nb_read(data_out);        
      }

      if(/*TRU Data FIFO full - num_free() == 0, or a threshold??*/) {
        s_tru_state = WAIT;
      } else if(/* Regions fully read out */) {
        s_tru_state = CHIP_TRAILER;
      }
      break;
      
    case WAIT:
      // num_free() > 0, or a threshold??
      if(s_tru_fifo_out.num_free() > 0) {
        if(/* More region data */)
          s_tru_state = REGION_DATA;
        else /* No more region data - output chip trailer */
          s_tru_state = CHIP_TRAILER;
      }
      break;
      
    case CHIP_TRAILER:
      // num_free() == 0, or a threshold??
      if(s_tru_fifo_out.num_free() == 0 || s_frame_end_fifo.num_available() == 0) {
        ///@todo Read  something from s_frame_end_fifo here..
        ///@todo Implement some readout flags here?
        readout_flags = 0; 
        data_out = AlpideChipTrailer(readout_flags);
        s_tru_state = IDLE;
      }
      break;
    }










    
    // -------------------------------------------------------------
    // Old TRU implementation
    case CHIP_HEADER:
      data_out = AlpideChipHeader(mChipId, mBunchCounter);
      if(s_current_event_hits_left_in.read() == 0)
        s_tru_state = CHIP_EMPTY_FRAME;
      else {
        s_current_region = 0;
        s_tru_state = REGION_HEADER;
      }
      break;
      
    case CHIP_EMPTY_FRAME:
      data_out = AlpideChipEmptyFrame(mChipId, mBunchCounter);
      s_tru_state = CHIP_TRAILER;
      break;
      
    case REGION_HEADER:
      region = s_current_region.read();
      
      // Does current region have data? Then progress to region data state
      if(s_region_empty_in[region].read() == false ||
         s_region_fifo_in[region]->num_available() > 0)
      {
        s_tru_state = REGION_DATA;
        data_out = AlpideRegionHeader(region);
        break;        
      } else if(region < (N_REGIONS-1)) { // Search for region with data
        region++;
        s_current_region = region;
        data_out = AlpideIdle();
        break;
      } else { // No more data
        // No break here - Allow program to continue into
        // CHIP_TRAILER state if we have read out all regions
        s_tru_state = CHIP_TRAILER;
      }

    case CHIP_TRAILER:
      ///@todo Implement some readout flags here?
      readout_flags = 0;
      data_out = AlpideChipTrailer(readout_flags);

      if(s_event_buffers_used_in.read() > 0)
        s_tru_state = CHIP_HEADER;
      else
        s_tru_state = IDLE;
      break;      

    case REGION_DATA:
      region = s_current_region.read();
      
      if(s_region_fifo_in[region]->num_available() > 0) {
        s_region_fifo_in[region]->nb_read(data_out);
      } else {
        // Insert IDLE if the region FIFO is currently empty
        data_out = AlpideIdle();
      }

      // Is the region empty, and was this the last word in the region FIFO?
      if(s_region_empty_in[region].read() == true &&
            s_region_fifo_in[region]->num_available() == 0)
      {
        region++;
        s_current_region = region;
        
        if(region == N_REGIONS)
          s_tru_state = CHIP_TRAILER;
        else
          s_tru_state = REGION_HEADER;
      }
      break;
      
    case IDLE:
      data_out = AlpideIdle();

      if(s_event_buffers_used_in.read() > 0)
        s_tru_state = CHIP_HEADER;
      
      break;
      
    default:
      s_tru_state = IDLE;
      break;
    }

    // Check if busy status has changed, and whether it has been signalled yet
    if(s_busy_status_in == true && s_busy_on_signalled == false) {
      // Check if there is an IDLE word, and replace it with BUSY ON
      if(data_out.signalBusyOn() == true) {
        std::cout << "BUSY ON signalled" << std::endl;
        s_busy_on_signalled = true;
        s_busy_off_signalled = false;
      }
    } else if(s_busy_status_in == false && s_busy_off_signalled == false) {
      // Check if there is an IDLE word, and replace it with BUSY OFF
      if(data_out.signalBusyOff() == true) {
        std::cout << "BUSY OFF signalled" << std::endl;
        s_busy_off_signalled = true;
        s_busy_on_signalled = false;
      }      
    }
    
    s_tru_fifo_out->nb_write(data_out);
    
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
  ss << name_prefix << "TRU.";
  std::string tru_name_prefix = ss.str();
  
  addTrace(wf, tru_name_prefix, "tru_state", s_tru_state);   
  addTrace(wf, tru_name_prefix, "current_region", s_current_region);     
  addTrace(wf, tru_name_prefix, "current_event_hits_left_in", s_current_event_hits_left_in);       
  addTrace(wf, tru_name_prefix, "event_buffers_used_in", s_event_buffers_used_in);         
  
  for(int i = 0; i < N_REGIONS; i++) {
    addTrace(wf, tru_name_prefix, "region_empty_in_" + std::to_string(i), s_region_empty_in[i]);             
  }
}
