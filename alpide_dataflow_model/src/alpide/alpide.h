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
  
  ///@brief Matrix readout clock. Not the same as 40MHz, typically
  ///       50 ns period is used for reading out from the priority encoders,
  ///       too allow the asynchronous encoder logic time to settle..
  sc_in_clk s_matrix_readout_clk_in;

  ///@brief 40MHz LHC clock
  sc_in_clk s_system_clk_in;

  sc_out<sc_uint<24> > s_serial_data_output;
  ///@}
  
  ///@todo Should these signals be private maybe?
  ///@defgroup SystemC signals
  ///@{
  sc_signal<bool> s_busy_status;

  sc_signal<bool> s_readout_abort;

  ///@brief Number of events stored in the chip at any given time
  sc_signal<sc_uint<8> > s_event_buffers_used;

  ///@brief Sum of all hits in all multi event buffers
  sc_signal<sc_uint<32> > s_total_number_of_hits;

  ///@brief Number of hits in oldest multi event buffer
  sc_signal<sc_uint<32> > s_oldest_event_number_of_hits;

  sc_signal<bool> s_region_empty[N_REGIONS];

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

  void strobeProcess(void);
  void matrixReadout(void);
  void dataTransmission(void);
  void frameReadoutProcess(void); // FROMU

public:
  Alpide(sc_core::sc_module_name name, int chip_id, int region_fifo_size,
         int tru_fifo_size, bool enable_clustering, bool continuous_mode);
  int getChipId(void) {return mChipId;}
  bool newEvent(uint64_t event_time);
  void addTraces(sc_trace_file *wf, std::string name_prefix) const;
};


#endif
