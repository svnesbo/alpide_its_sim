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
  sc_trace_file *wf = NULL;
  
  sc_core::sc_set_time_resolution(1, sc_core::SC_NS);  
  // Parse configuration file here
  QSettings* simulation_settings = getSimSettings();

  Stimuli stimuli("stimuli", simulation_settings);

  // 25ns period, 0.5 duty cycle, first edge at 2 time units, first value is true
  sc_clock clock_40MHz("clock_40MHz", 25, 0.5, 2, true);

  stimuli.clock(clock_40MHz);

  // Open VCD file
  if(simulation_settings->value("data_output/write_vcd").toBool() == true) {
    wf = sc_create_vcd_trace_file("alpide_toy-model_results2");
    stimuli.addTraces(wf);

    if(simulation_settings->value("data_output/write_vcd_clock").toBool() == true) {
      //@todo Add a warning here if user tries to simulate over 1000 events with this option enabled,
      //      because it will consume 100s of megabytes
      sc_trace(wf, clock_40MHz, "clock");
    }
  }


  std::cout << "Starting simulation.." << std::endl;
  
  sc_core::sc_start();

  std::cout << "Started simulation.." << std::endl;

  if(wf != NULL) {
    sc_close_vcd_trace_file(wf);
  }

  return 0;
}


//@todo STIMULI FUNCTION!
