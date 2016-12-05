/**
 * @file   outer_layer_module.h
 * @Author Simon Voigt Nesbo
 * @date   December 5, 2016
 * @brief  Header file for outer layer module class
 *
 * Detailed description of file.
 */
#ifndef OUTER_LAYER_MODULE_H
#define OUTER_LAYER_MODULE_H

#include "../alpide/alpide.h"

const int n_chips_in_outer_module = 8;

class OuterLayerModule {
public: // SystemC signals
  sc_fifo <DataByte> s_serial_lines[n_chips_in_outer_module]
  sc_in_clk s_clk_in;
  sc_in<bool> s_trigger_in;
  
private:
  // Maybe this should be a pointer, and I should create the chips
  // with new and call the right constructor etc.?
  Alpide mAlpideChips[n_chips_in_outer_module];
  int mSerialBandwidthMbps;

public:
  OuterLayerModule();

};


#endif
