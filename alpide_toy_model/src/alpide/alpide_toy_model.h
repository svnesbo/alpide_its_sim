/**
 * @file   alpide_toy_model.h
 * @author Simon Voigt Nesbo
 * @date   December 11, 2016
 * @brief  Header file for AlpideToyModel class.
 */

#ifndef ALPIDE_TOY_MODEL_H
#define ALPIDE_TOY_MODEL_H


#include "pixel_matrix.h"
#include <systemc.h>


/// Alpide "toy model" class. It only implements the MEBs, 
/// no RRU FIFOs, and no TRU FIFO. It will be used to run some initial
/// estimations for probability of MEB overflow (busy).
class AlpideToyModel : public PixelMatrix, sc_core::sc_module
{
public: // SystemC signals  
  ///@brief 40MHz clock input
  sc_in_clk s_clk_in;

  ///@brief Number of events stored in the chip at any given time
  sc_out<sc_uint<8> > s_event_buffers_used;

  ///@brief Sum of all hits in all multi event buffers
  sc_out<sc_uint<32> > s_total_number_of_hits;
    
private:
  int mChipId;
  void matrixReadout(void);

public:
  AlpideToyModel(sc_core::sc_module_name name, int chip_id);
  int getChipId(void) {return mChipId;}
};


#endif
