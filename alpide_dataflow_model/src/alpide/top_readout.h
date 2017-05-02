/**
 * @file   top_readout.h
 * @author Simon Voigt Nesbo
 * @date   February 20, 2017
 * @brief  Class for implementing the Top Readout Unit (TRU) in the Alpide chip.
 */


///@addtogroup alpide
///@{
#ifndef TOP_READOUT_H
#define TOP_READOUT_H

#include "region_readout.h"
#include "alpide_constants.h"
#include "alpide_data_format.h"
#include <string>

// Ignore warnings about use of auto_ptr in SystemC library
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#include <systemc.h>
#pragma GCC diagnostic pop

// Ignore warnings about functions with unused variables in TLM library
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#include <tlm.h>
#pragma GCC diagnostic pop


/// The TopReadoutUnit (TRU) class is a simple representation of the TRU in the Alpide chip.
/// It should be connected to the Region Readout Unit (RRU) in the Alpide object,
/// and will be responsible for reading out from the RRUs with the topRegionReadoutProcess,
/// which should run at the system clock (40MHz).
class TopReadoutUnit : sc_core::sc_module
{
public:
  ///@brief Alpide chip clock (typically 40MHz)
  sc_in_clk s_clk_in; 
  
  sc_in<bool> s_readout_abort_in;
  sc_in<bool> s_fatal_state_in;  
  sc_in<bool> s_region_fifo_empty_in[N_REGIONS];
  sc_in<bool> s_region_valid_in[N_REGIONS];
  sc_in<AlpideDataWord> s_region_data_in[N_REGIONS];

  sc_out<bool> s_region_event_pop_out;
  sc_out<bool> s_region_event_start_out;
  sc_out<bool> s_region_data_read_out[N_REGIONS];
  
  // Outputs from the FIFO, not outputs from the TRU module/class
  sc_port<tlm::tlm_nonblocking_get_peek_if<FrameStartFifoWord>> s_frame_start_fifo_output;
  sc_port<tlm::tlm_nonblocking_get_peek_if<FrameEndFifoWord>> s_frame_end_fifo_output;
  
  
  ///@brief Output from TRU
  sc_port<sc_fifo_out_if<AlpideDataWord>> s_dmu_fifo_input;

private:
  sc_signal<sc_uint<8> > s_tru_state;
  sc_signal<sc_uint<8> > s_previous_region;

  ///@brief Signal copy of all_regions_empty variable, 1 cycle delayed
  sc_signal<bool> s_all_regions_empty_debug;

  ///@brief Signal copy of no_regions_valid variable, 1 cycle delayed  
  sc_signal<bool> s_no_regions_valid_debug;

  // Standard C++ members
  unsigned int mChipId;
  FrameStartFifoWord mCurrentFrameStartWord;
  FrameEndFifoWord mCurrentFrameEndWord;  

  enum TRU_state_t {
    EMPTY = 0,
    IDLE = 1,
    WAIT_REGION_DATA = 2,
    CHIP_HEADER = 3,    
    BUSY_VIOLATION = 4,
    REGION_DATA = 5,
    WAIT = 6,
    CHIP_TRAILER = 7
  };

  bool getNextRegion(unsigned int& region_out);
  bool getAllRegionsEmpty(void);
  
public:
  TopReadoutUnit(sc_core::sc_module_name name, unsigned int chip_id);
  void topRegionReadoutProcess(void);
  void addTraces(sc_trace_file *wf, std::string name_prefix) const;
};


#endif
///@}