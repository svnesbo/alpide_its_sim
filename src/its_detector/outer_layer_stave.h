/**
 * @file   outer_layer_stave.h
 * @Author Simon Voigt Nesbo
 * @date   December 5, 2016
 * @brief  Header file for outer layer stave class
 *
 * Detailed description of file.
 */
#ifndef OUTER_LAYER_STAVE_H
#define OUTER_LAYER_STAVE_H

#include "outer_layer_module.h"


class OuterLayerStave {
public: // SystemC signals
  sc_fifo <DataByte> s_serial_lines[n];
  sc_in_clk s_clk_in;
  sc_in<bool> s_trigger_in;

private:
  OuterLayerModule *mModules;
  int mNumberOfModules;

public:
  OuterLayerStave();

};


#endif
