/**
 * @file   top_readout.h
 * @Author Simon Voigt Nesbo
 * @date   February 20, 2017
 * @brief  Class for implementing the Top Readout Unit (TRU) in the Alpide chip.
 */

#ifndef TOP_READOUT_H
#define TOP_READOUT_H

#include "region_readout.h"
#include "alpide_constants.h"
#include <systemc.h>
#include <string>


enum TRU_state_t {CHIP_HEADER, CHIP_EMPTY_FRAME, REGION_HEADER, CHIP_TRAILER, REGION_DATA, IDLE};

class TopReadoutUnit : sc_core::sc_module
{
public:
  // SystemC signals
  sc_port<sc_fifo_in_if<AlpideDataWord> > s_region_fifo_in[N_REGIONS];
  sc_in<bool> s_region_empty_in[N_REGIONS];
  sc_in<sc_uint<32> > s_current_event_hits_left_in;
  sc_in<sc_uint<8> > s_event_buffers_used_in;

  ///@brief Output from TRU
  sc_port<sc_fifo_out_if<AlpideDataWord> > s_tru_fifo_out;

  ///@brief Alpide chip clock (typically 40MHz)
  sc_in_clk s_clk_in;
  
private:
  unsigned int mCurrentRegion;
  unsigned int mChipId;
  unsigned int mBunchCounter;
  TRU_state_t mTRUState;
  
public:
  TopReadoutUnit(sc_core::sc_module_name name, unsigned int chip_id);
  void topRegionReadoutProcess(void);
  void addTraces(sc_trace_file *wf, std::string name_prefix) const;
};



#endif
