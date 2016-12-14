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
#include <list>


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

void print_trig_freq(const std::list<int>& t_delta_queue)
{
  long t_delta_sum = 0;
  double t_delta_avg;
  long trig_freq;

  if(t_delta_queue.size() == 0) {
    trig_freq = 0;
  } else {
    for(auto it = t_delta_queue.begin(); it != t_delta_queue.end(); it++) {
      t_delta_sum += *it;
    }

    std::cout << "t_delta_sum: " << t_delta_sum << " ns" << std::endl;
    t_delta_avg = t_delta_sum/t_delta_queue.size();
    std::cout << "t_delta_avg: " << t_delta_avg << " ns" << std::endl;

    t_delta_avg /= 1.0E9;

    std::cout << "t_delta_avg: " << t_delta_avg << " s" << std::endl;

    trig_freq = 1/t_delta_avg;
    
    
    //trig_freq = (long)(1.0/(((double)t_delta_sum/t_delta_queue.size())/1.0e9));
  }

  std::cout << "Average trigger frequency: " << trig_freq << "Hz" << std::endl;;
}


void Stimuli::stimuliProcess(void)
{
  std::list<int> t_delta_history;
  const int t_delta_averaging_num = 50;
  
  int num_events = 1000;
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


      t_delta_history.push_back(t_delta);
      if(t_delta_history.size() > t_delta_averaging_num)
        t_delta_history.pop_front();
      
      print_trig_freq(t_delta_history);
    
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


void Stimuli::addTraces(sc_trace_file *wf)
{
  sc_trace(wf, chip_event_buffers_used, "chip_event_buffers_used");
  sc_trace(wf, chip_total_number_of_hits, "chip_total_number_of_hits");
}
