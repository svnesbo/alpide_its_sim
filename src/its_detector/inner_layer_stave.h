/**
 * @file   inner_layer_stave.h
 * @Author Simon Voigt Nesbo
 * @date   December 5, 2016
 * @brief  Header file for inner layer stave class
 *
 * Detailed description of file.
 */
#ifndef INNER_LAYER_STAVE_H
#define INNER_LAYER_STAVE_H

#include "inner_layer_module.h"


class InnerLayerStave {
public: // SystemC signals
  sc_fifo <DataByte> s_serial_lines[n_chips_in_inner_module]
  sc_in_clk s_clk_in;
  sc_in<bool> s_trigger_in;

private:
  InnerLayerModule *mModules;
  int mNumberOfModules;

public:
  InnerLayerStave();

};


#endif
