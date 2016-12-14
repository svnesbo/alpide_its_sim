/**
 * @file   main.cpp
 * @Author Simon Voigt Nesbo
 * @date   December 11, 2016
 * @brief  Main source file for Alpide "toy model" simulation testbench
 *
 * Detailed description of file.
 */

#include "../settings/settings.h"
#include "../event/event_generator.h"
#include "../alpide/alpide_toy_model.h"
#include "stimuli.h"
#include <systemc.h>
#include <set>
#include <iostream>

enum SimulationMode {ONE_CHIP, FULL_DETECTOR, OTHER_MODES};


int sc_main(int argc, char** argv)
{
  sc_core::sc_set_time_resolution(1, sc_core::SC_NS);  
  // Parse configuration file here
  //QSettings* readoutSimSettings = getSimSettings();

  Stimuli stimuli("stimuli");

  // 25ns period, 0.5 duty cycle, first edge at 2 time units, first value is true
  sc_clock clock_40MHz("clock_40MHz", 25, 0.5, 2, true);

  stimuli.clock(clock_40MHz);

  // Open VCD file
  sc_trace_file *wf = sc_create_vcd_trace_file("alpide_toy-model_results");

  stimuli.addTraces(wf);

  // Dump the desired signals..
  sc_trace(wf, clock_40MHz, "clock");

  std::cout << "Starting simulation.." << std::endl;
  
  sc_core::sc_start();

  std::cout << "Started simulation.." << std::endl;

  sc_close_vcd_trace_file(wf);  
}


//@todo STIMULI FUNCTION!
