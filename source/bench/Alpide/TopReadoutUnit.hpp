/**
 * @file   TopReadoutUnit.hpp
 * @author Simon Voigt Nesbo
 * @date   February 20, 2017
 * @brief  Class for implementing the Top Readout Unit (TRU) in the Alpide chip.
 */


///@addtogroup alpide
///@{
#ifndef TOP_READOUT_H
#define TOP_READOUT_H

#include "RegionReadoutUnit.hpp"
#include "AlpideDataWord.hpp"
#include "alpide_constants.hpp"
#include <string>

// Ignore warnings about use of auto_ptr and unused parameters in SystemC library
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#include <systemc.h>
#pragma GCC diagnostic pop

// Ignore certain warnings in TLM library
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Woverloaded-virtual"
#include <tlm.h>
#pragma GCC diagnostic pop


/// The TopReadoutUnit (TRU) class is a simple representation of the TRU in the Alpide chip.
/// It should be connected to the Region Readout Unit (RRU) in the Alpide object,
/// and is responsible for reading out from the RRUs.
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
  sc_buffer<sc_uint<8> > s_tru_current_state;
  sc_signal<sc_uint<8> > s_tru_next_state;
  sc_signal<sc_uint<8> > s_previous_region;

  ///@brief Data is read right from s_region_data_in into this register every cycle,
  ///       and data is written from this reg to dmu fifo
  sc_signal<AlpideDataWord> s_tru_data;

  sc_event E_update_fsm;

  ///@brief Signal copy of all_regions_empty variable, 1 cycle delayed
  sc_signal<bool> s_no_regions_empty_debug;

  ///@brief Matches read signal sent to active region
  sc_signal<bool> s_region_data_read_debug;

  ///@brief Signal copy of no_regions_valid variable, 1 cycle delayed
  sc_signal<bool> s_no_regions_valid_debug;

  sc_signal<bool> s_frame_start_fifo_empty;
  sc_signal<bool> s_frame_end_fifo_empty;

  sc_signal<bool> s_dmu_data_fifo_full;
  sc_signal<bool> s_dmu_data_fifo_empty;

  sc_signal<bool> s_write_dmu_fifo;

  // Standard C++ members
  unsigned int mChipId;
  FrameStartFifoWord mCurrentFrameStartWord;
  FrameEndFifoWord mCurrentFrameEndWord;

  /// Indicates that the TRU was/is IDLE
  /// (Used to disable sensitivity to clock to save simulation time);
  bool mIdle;

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

  void topRegionReadoutOutputNextState(void);
  void topRegionReadoutStateUpdate(void);
  //void topRegionReadoutOutputMethod(void);
  bool getNextRegion(unsigned int& region_out);
  bool getNoRegionsEmpty(void);

public:
  TopReadoutUnit(sc_core::sc_module_name name, unsigned int chip_id);
  void addTraces(sc_trace_file *wf, std::string name_prefix) const;
};


#endif
///@}
