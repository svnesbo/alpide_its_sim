/**
 * @file   stimuli.cpp
 * @Author Simon Voigt Nesbo
 * @date   December 12, 2016
 * @brief  Source file for stimuli function for Alpide SystemC model
 *
 * Detailed description of file.
 */

#include "stimuli.h"
#include <systemc.h>


//@todo RENAME TESTBENCH, AND INSTANTIATE MODULES, CONNECT SIGNALS AND EVERYTHING IN HERE!
/*
SC_CTOR(Stimuli)
{
  // 25ns period, 0.5 duty cycle, first edge at 2 time units, first value is true
  //sc_clock clock_40MHz("clock_40MHz", 25, 0.5, 2, true);
  clock_40MHz = new sc_clock("clock_40MHz", 25, 0.5, 2, true);



  // Connect signals to Alpide
  mAlpide.s_clk_in(clock_40MHz);
  mAlpide.s_event_buffers_used(chip_event_buffers_used);
  mAlpide.s_total_number_of_hits(chip_total_number_of_hits);

  
  // Open VCD file
  mWaveformFile = sc_create_vcd_trace_file("counter");

  // Initialize variables, event generator and detector based on configuration file
  mEvents = new EventGenerator();
  mAlpideChip = new AlpideToyModel("alpide", 0);  
  
  SC_CTHREAD(stimuliProcess, clock_40MHz.pos());

  
  //@todo Logging process?
  //SC_CTHREAD(logging_process, clock_40MHz.pos());  
}
*/


void Stimuli::stimuliProcess(void)
{
  int num_events = 100;
  int time_ns = 0;

  int i = 0;
  
  while(simulation_done == false) {
    if(mEvents.getEventCount() < num_events) {
      std::cout << "Simulating event number " << i << std::endl;      
      mEvents.removeOldestEvent();
      mEvents.generateNextEvent();

      const Event& e = mEvents.getNextEvent();


      //@todo Convert everything to picoseconds!!
      int t_delta = e.getEventDeltaTime();
      if(t_delta == 0)
        t_delta = 1;
    
      // Wait for next event to occur
      std::cout << "Waiting for " << t_delta << " ns." << std::endl;
      wait(t_delta);

      mAlpide.newEvent();
      e.feedHitsToChip(mAlpide, mAlpide.getChipId());

      std::cout << "Number of events in chip: " << mAlpide.getNumEvents() << std::endl;
      std::cout << "Hits remaining in oldest event in chip: " << mAlpide.getHitsRemainingInOldestEvent();
      std::cout << "  Hits in total (all events): " << mAlpide.getHitTotalAllEvents() << std::endl;
    }

    // Have all mEvents generated and fed to chip, and all events read out from chip?
    else if(mAlpide.getNumEvents() == 0) {
      std::cout << "Finished generating all events, and Alpide chip is done emptying MEBs.\n";
      
      simulation_done = true;
      sc_core::sc_stop();
    }
    else {
      wait();
    }

    i++;
  }
}
