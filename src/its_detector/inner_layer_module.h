/**
 * @file   inner_layer_module.h
 * @Author Simon Voigt Nesbo
 * @date   December 5, 2016
 * @brief  Header file for inner layer module class
 *
 * Detailed description of file.
 */
#ifndef INNER_LAYER_MODULE_H
#define INNER_LAYER_MODULE_H

#include "../alpide/alpide.h"

const int n_chips_in_inner_module = 8;

class InnerLayerModule {
public: // SystemC signals
  sc_fifo <DataByte> s_serial_lines[n_chips_in_inner_module]
  sc_in_clk s_clk_in;
  sc_in<bool> s_trigger_in;
  
private:
  // Maybe this should be a pointer, and I should create the chips
  // with new and call the right constructor etc.?
  Alpide mAlpideChips[n_chips_in_inner_module];
  int mSerialBandwidthMbps;  

public:
  InnerLayerModule();

};


#endif
