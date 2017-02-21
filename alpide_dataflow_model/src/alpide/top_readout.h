/**
 * @file   top_readout.h
 * @Author Simon Voigt Nesbo
 * @date   February 20, 2017
 * @brief  Class for implementing the Top Readout Unit (TRU) in the Alpide chip.
 */

#ifndef TOP_READOUT_H
#define TOP_READOUT_H

#include "region_readout.h"


class TopReadoutUnit
{
public:
  // SystemC signals
  sc_core::sc_fifo_in_if<AlpideDataWord> s_region_fifo_in[N_REGIONS];
  sc_core::sc_in<bool> s_region_empty[N_REGIONS];

  ///@brief Output from TRU
  sc_core::sc_fifo_out_if<AlpideDataWord> s_tru_fifo_out;

  ///@brief Alpide chip clock (typically 40MHz)
  sc_in_clk s_clk_in;
  
private:
  unsigned int mCurrentRegion;
  
public:
  TopReadoutUnit();
  void topRegionReadoutProcess(void);
}



#endif
