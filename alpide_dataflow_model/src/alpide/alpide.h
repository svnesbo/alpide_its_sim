/**
 * @file   alpide.h
 * @author Simon Voigt Nesbo
 * @date   December 11, 2016
 * @brief  Header file for Alpide class.
 */

#ifndef ALPIDE_H
#define ALPIDE_H


#include "alpide_data_format.h"
#include "pixel_matrix.h"
#include "region_readout.h"
#include "top_readout.h"

// Ignore warnings about use of auto_ptr in SystemC library
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#include <systemc.h>
#pragma GCC diagnostic pop

#include <vector>
#include <list>
#include <string>


/// Alpide main class. Currently it only implements the MEBs, 
/// no RRU FIFOs, and no TRU FIFO. It will be used to run some initial
/// estimations for probability of MEB overflow (busy).
class Alpide : sc_core::sc_module, public PixelMatrix
{
public:
  ///@defgroup SystemC ports
  ///@{
  
  ///@brief 40MHz LHC clock
  sc_in_clk s_system_clk_in;
  sc_in<bool> s_strobe_n_in;

  ///@brief Indicates that the chip is ready to accept hits and setPixel() can be called.
  sc_in<bool> s_chip_ready_out;
  
  sc_out<sc_uint<24> > s_serial_data_output;
  ///@}
  
  ///@todo Should these signals be private maybe?
  ///@defgroup SystemC signals
  ///@{

  ///@brief Number of events stored in the chip at any given time
  sc_signal<sc_uint<8> > s_event_buffers_used;

  ///@brief Sum of all hits in all multi event buffers
  sc_signal<sc_uint<32> > s_total_number_of_hits;

  ///@brief Number of hits in oldest multi event buffer
  sc_signal<sc_uint<32> > s_oldest_event_number_of_hits;

  sc_signal<bool> s_region_empty[N_REGIONS];
  sc_signal<bool> s_region_valid[N_REGIONS];
  sc_signal<bool> s_region_data_read[N_REGIONS];
  sc_signal<bool> s_region_event_start;  
  sc_signal<bool> s_region_event_pop;
  sc_in<AlpideDataWord> s_region_data[N_REGIONS];

  ///@brief Frame Readout Managment Unit (FROMU) signals
  sc_signal<bool> s_frame_readout_start;
  sc_signal<bool> s_frame_readout_done[N_REGIONS];
  sc_signal<bool> s_frame_readout_done_all;
  sc_signal<bool> s_readout_abort;
  sc_signal<bool> s_tru_frame_fifo_busy;
  sc_signal<bool> s_tru_data_overrun_mode;
  sc_signal<bool> s_tru_frame_fifo_fatal_overflow;
  sc_signal<bool> s_multi_event_buffers_busy;
  sc_signal<bool> s_busy_violation;
  sc_signal<bool> s_busy_status;

  enum FROMU_readout_state_t {
    WAIT_FOR_EVENTS = 0,
    REGION_READOUT_START = 1,
    WAIT_FOR_REGION_READOUT = 2,
    REGION_READOUT_DONE = 3
  };

  sc_signal<FROMU_readout_state_t> s_fromu_readout_state;
  
  
  ///@brief Region FIFOs
  sc_fifo<AlpideDataWord> s_top_readout_fifo;
  sc_signal<sc_fifo_in_if<FrameStartFifoWord> > s_tru_frame_start_fifo_in;
  sc_signal<sc_fifo_in_if<FrameEndFifoWord> > s_tru_frame_end_fifo_in;
  
  sc_signal<sc_uint<8> > s_tru_fifo_size;
  ///@}
    
private:
  int mChipId;
  bool mEnableReadoutTraces;
  unsigned int mBunchCounter;

  std::vector<RegionReadoutUnit*> mRRUs;
  TopReadoutUnit* mTRU;


  void mainProcess(void);  
  void strobeProcess(void);
  void frameReadout(void); // FROMU    
  void dataTransmission(void);
  bool Alpide::getFrameReadoutDone(void);

public:
  Alpide(sc_core::sc_module_name name, int chip_id, int region_fifo_size,
         int tru_fifo_size, bool enable_clustering, bool continuous_mode,
         bool matrix_readout_speed);
  int getChipId(void) {return mChipId;}
  void addTraces(sc_trace_file *wf, std::string name_prefix) const;
};


#endif
