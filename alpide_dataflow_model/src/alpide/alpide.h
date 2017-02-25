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
#include <systemc.h>
#include <vector>
#include <list>
#include <string>


/// Alpide main class. Currently it only implements the MEBs, 
/// no RRU FIFOs, and no TRU FIFO. It will be used to run some initial
/// estimations for probability of MEB overflow (busy).
class Alpide : sc_core::sc_module, public PixelMatrix
{
public:
  // SystemC signals
  
  ///@brief Matrix readout clock. Not the same as 40MHz, typically
  ///       50 ns period is used for reading out from the priority encoders,
  ///       too allow the asynchronous encoder logic time to settle..
  sc_in_clk s_matrix_readout_clk_in;

  ///@brief 40MHz LHC clock
  sc_in_clk s_system_clk_in;

  ///@brief Number of events stored in the chip at any given time
  sc_signal<sc_uint<8> > s_event_buffers_used;

  ///@brief Sum of all hits in all multi event buffers
  sc_signal<sc_uint<32> > s_total_number_of_hits;

  ///@brief Number of hits in oldest multi event buffer
  sc_signal<sc_uint<32> > s_oldest_event_number_of_hits;

  sc_signal<bool> s_region_empty[N_REGIONS];

  ///@brief Region FIFOs
  sc_fifo<AlpideDataWord> s_region_fifos[N_REGIONS];
  sc_fifo<AlpideDataWord> s_top_readout_fifo;
    
private:
  int mChipId;
  bool mEnableReadoutTraces;

  std::vector<RegionReadoutUnit*> mRRUs;
  TopReadoutUnit* mTRU;
  
  void matrixReadout(void);

public:
  Alpide(sc_core::sc_module_name name, int chip_id, int region_fifo_size,
         bool enable_readout_traces, bool enable_clustering, bool continuous_mode);
  int getChipId(void) {return mChipId;}
  void addTraces(sc_trace_file *wf, std::string name_prefix) const;
};


#endif
