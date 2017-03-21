/**
 * @file   top_readout.h
 * @author Simon Voigt Nesbo
 * @date   February 20, 2017
 * @brief  Class for implementing the Top Readout Unit (TRU) in the Alpide chip.
 */

#ifndef TOP_READOUT_H
#define TOP_READOUT_H

#include "region_readout.h"
#include "alpide_constants.h"
#include <string>

// Ignore warnings about use of auto_ptr in SystemC library
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#include <systemc.h>
#pragma GCC diagnostic pop



struct FrameStartFifoWord {
  sc_uint<1> busy_violation;
  sc_uint<8> BC_for_frame; // Bunch counter
};


struct FrameEndFifoWord {
  sc_uint<1> flushed_incomplete;
  sc_uint<1> strobe_extended;
  sc_uint<1> busy_transition;
};  

/// The TopReadoutUnit (TRU) class is a simple representation of the TRU in the Alpide chip.
/// It should be connected to the Region Readout Unit (RRU) in the Alpide object,
/// and will be responsible for reading out from the RRUs with the topRegionReadoutProcess,
/// which should run at the system clock (40MHz).
class TopReadoutUnit : sc_core::sc_module
{
public:
  // SystemC signals
  sc_port<sc_fifo_in_if<AlpideDataWord> > s_region_fifo_in[N_REGIONS];
  sc_in<bool> s_region_empty_in[N_REGIONS];
  sc_in<bool> s_busy_status_in;
  sc_in<sc_uint<32> > s_current_event_hits_left_in;
  sc_in<sc_uint<8> > s_event_buffers_used_in;

  ///@brief Output from TRU
  sc_port<sc_fifo_out_if<AlpideDataWord> > s_tru_fifo_out;

  ///@brief Alpide chip clock (typically 40MHz)
  sc_in_clk s_clk_in;

private:
  sc_signal<sc_uint<8> > s_tru_state;
  sc_signal<sc_uint<8> > s_current_region;
  sc_signal<bool> s_busy_on_signalled;
  sc_signal<bool> s_busy_off_signalled;
  //sc_fifo<FrameStartFifoWord> s_frame_start_fifo;
  //sc_fifo<FrameEndFIfoWord> s_frame_end_fifo;
  
private:

  unsigned int mChipId;
  unsigned int mBunchCounter;

  enum TRU_state_t {
    CHIP_HEADER = 0,
    CHIP_EMPTY_FRAME = 1,
    REGION_HEADER = 2,
    CHIP_TRAILER = 3,
    REGION_DATA = 4,
    IDLE = 5
  };  
  
public:
  TopReadoutUnit(sc_core::sc_module_name name, unsigned int chip_id);
  void topRegionReadoutProcess(void);
  void addTraces(sc_trace_file *wf, std::string name_prefix) const;
};



#endif
