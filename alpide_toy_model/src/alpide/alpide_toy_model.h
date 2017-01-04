/**
 * @file   alpide_toy_model.h
 * @Author Simon Voigt Nesbo
 * @date   December 11, 2016
 * @brief  "Toy model" for the ALPIDE chip. It only implements the MEBs, 
 *          no RRU FIFOs, and no TRU FIFO. It will be used to run some initial
 *          estimations for probability of MEB overflow (busy).
 *
 * Detailed description of file.
 */

#ifndef ALPIDE_TOY_MODEL_H
#define ALPIDE_TOY_MODEL_H

//@todo Remove?
//#include "../event/trigger_event.h"
#include "pixel_matrix.h"
#include <systemc.h>


class AlpideToyModel : public PixelMatrix, sc_core::sc_module
{
public: // SystemC signals  
  //sc_in<bool> s_trigger_in;

  //@brief 40MHz clock input
  sc_in_clk s_clk_in;

  //@brief 1.2Gbps or 0.4Gbps clock. For this model we supply it externally
  //sc_in_clk s_clk_serial_link_in;

  //@brief Number of events stored in the chip at any given time
  sc_out<sc_uint<8> > s_event_buffers_used;

  //@brief Sum of all hits in all multi event buffers
  sc_out<sc_uint<32> > s_total_number_of_hits;
    
private:
  int mChipId;
  void matrixReadout(void);

public:
  AlpideToyModel(sc_core::sc_module_name name, int chip_id);
  int getChipId(void) {return mChipId;}
};


#endif
