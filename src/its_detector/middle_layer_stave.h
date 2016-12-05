/**
 * @file   middle_layer_stave.h
 * @Author Simon Voigt Nesbo
 * @date   December 5, 2016
 * @brief  Header file for middle layer stave class
 *
 * Detailed description of file.
 */
#ifndef MIDDLE_LAYER_STAVE_H
#define MIDDLE_LAYER_STAVE_H

#include "outer_layer_module.h"


class MiddleLayerStave {
public: // SystemC signals
  sc_fifo <DataByte> s_serial_lines[n];
  sc_in_clk s_clk_in;
  sc_in<bool> s_trigger_in;

private:
  OuterLayerModule *mModules;
  int mNumberOfModules;

public:
  MiddleLayerStave();

};


#endif
