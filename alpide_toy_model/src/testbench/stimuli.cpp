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
  }

  std::cout << "Average trigger frequency: " << trig_freq << "Hz" << std::endl;;
}


SC_HAS_PROCESS(Stimuli);
Stimuli::Stimuli(sc_core::sc_module_name name, QSettings* settings)
  : sc_core::sc_module(name)
{
  int hit_multiplicity_avg = settings->value("event/hit_multiplicity_avg").toInt();
  int hit_multiplicity_stddev = settings->value("event/hit_multiplicity_stddev").toInt();
  int bunch_crossing_rate_ns = settings->value("event/bunch_crossing_rate_ns").toInt();

  //@todo Rename to average_trigger_rate_ns?
  int average_crossing_rate_ns = settings->value("event/average_crossing_rate_ns").toInt();
  int trigger_filter_time_ns = settings->value("event/trigger_filter_time_ns").toInt();
  bool trigger_filter_enable = settings->value("event/trigger_filter_enable").toBool();
  int random_seed = settings->value("simulation/random_seed").toInt();
  bool create_csv_file = settings->value("data_output/write_event_csv").toBool();

  mNumEvents = settings->value("simulation/n_events").toInt();

  // Instantiate and connect signals to Alpide
  mAlpide = new AlpideToyModel("alpide", 0);
  mAlpide->s_clk_in(clock);
  mAlpide->s_event_buffers_used(chip_event_buffers_used);
  mAlpide->s_total_number_of_hits(chip_total_number_of_hits);


  // Instantiate event generator object
  mEvents = new EventGenerator(bunch_crossing_rate_ns,
                               average_crossing_rate_ns,
                               hit_multiplicity_avg,
                               hit_multiplicity_stddev,
                               random_seed,
                               create_csv_file);

  if(trigger_filter_enable == true) {
    mEvents->enableTriggerFiltering();      
    mEvents->setTriggerFilterTime(trigger_filter_time_ns);
  }      
  
  SC_CTHREAD(stimuliProcess, clock.pos());
}


void Stimuli::stimuliProcess(void)
{
  std::list<int> t_delta_history;
  const int t_delta_averaging_num = 50;
  
  int time_ns = 0;

  int i = 0;

  std::cout << "Staring simulation of " << mNumEvents << " events." << std::endl;
  
  while(simulation_done == false) {
    if(mEvents->getEventCount() < mNumEvents) {
      std::cout << "Simulating event number " << i << std::endl;      
      mEvents->removeOldestEvent();
      mEvents->generateNextEvent();

      const Event& e = mEvents->getNextEvent();

      

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

      e.feedHitsToChip(*mAlpide, mAlpide->getChipId());

      std::cout << "Number of events in chip: " << mAlpide->getNumEvents() << std::endl;
      std::cout << "Hits remaining in oldest event in chip: " << mAlpide->getHitsRemainingInOldestEvent();
      std::cout << "  Hits in total (all events): " << mAlpide->getHitTotalAllEvents() << std::endl;
    }

    // Have all mEvents generated and fed to chip, and all events read out from chip?
    else if(mAlpide->getNumEvents() == 0) {
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
