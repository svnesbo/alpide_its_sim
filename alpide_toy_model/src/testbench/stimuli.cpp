/**
 * @file   stimuli.cpp
 * @author Simon Voigt Nesbo
 * @date   December 12, 2016
 * @brief  Source file for stimuli function for Alpide SystemC model
 */

#include "stimuli.h"
#include <systemc.h>
#include <list>
#include <QString>


///@brief Takes a list of t_delta values (time between events) for the last events,
///       calculates the average event rate over those events, and prints it to std::cout.
///       The list must be maintained by the caller.
///@todo  Update/fix/remove this function.. currently not used..
void print_event_rate(const std::list<int>& t_delta_queue)
{
  long t_delta_sum = 0;
  double t_delta_avg;
  long event_rate;

  if(t_delta_queue.size() == 0) {
    event_rate = 0;
  } else {
    for(auto it = t_delta_queue.begin(); it != t_delta_queue.end(); it++) {
      t_delta_sum += *it;
    }

    std::cout << "t_delta_sum: " << t_delta_sum << " ns" << std::endl;
    t_delta_avg = t_delta_sum/t_delta_queue.size();
    std::cout << "t_delta_avg: " << t_delta_avg << " ns" << std::endl;

    t_delta_avg /= 1.0E9;

    std::cout << "t_delta_avg: " << t_delta_avg << " s" << std::endl;

    event_rate = 1/t_delta_avg;
  }

  std::cout << "Average event rate: " << event_rate << "Hz" << std::endl;;
}


SC_HAS_PROCESS(Stimuli);
///@brief Constructor for stimuli class.
///       Instantiates and initializes the EventGenerator and AlpideToyModel objects,
///       connects the SystemC ports
///@param name SystemC module name
///@param settings Settings object with simulation settings.
Stimuli::Stimuli(sc_core::sc_module_name name, QSettings* settings)
  : sc_core::sc_module(name)
{
  // Initialize variables for Stimuli object
  mNumEvents = settings->value("simulation/n_events").toInt();
  mStrobeActiveNs = settings->value("event/strobe_active_length_ns").toInt();
  mStrobeInactiveNs = settings->value("event/strobe_inactive_length_ns").toInt();  

    // Get some values from settings object used to initialize EventGenerator class
  QString multipl_dist_type = settings->value("event/hit_multiplicity_distribution_type").toString();
  int bunch_crossing_rate_ns = settings->value("event/bunch_crossing_rate_ns").toInt();
  int average_event_rate_ns = settings->value("event/average_event_rate_ns").toInt();
  int trigger_filter_time_ns = settings->value("event/trigger_filter_time_ns").toInt();
  bool trigger_filter_enable = settings->value("event/trigger_filter_enable").toBool();
  int strobe_length_ns = settings->value("event/strobe_length_ns").toInt();
  int random_seed = settings->value("simulation/random_seed").toInt();
  bool create_csv_file = settings->value("data_output/write_event_csv").toBool();
  int pixel_dead_time_ns = settings->value("alpide/pixel_shaping_dead_time_ns").toInt();
  int pixel_active_time_ns = settings->value("alpide/pixel_shaping_active_time_ns").toInt();

  // Instantiate event generator object with the desired hit multiplicity distribution
  if(multipl_dist_type == "gauss") {
    int multipl_gauss_stddev = settings->value("event/hit_multiplicity_gauss_stddev").toInt();  
    int multipl_gauss_avg = settings->value("event/hit_multiplicity_gauss_avg").toInt();
    
    mEvents = new EventGenerator("event_gen",
                                 bunch_crossing_rate_ns,
                                 average_event_rate_ns,
                                 strobe_length_ns,
                                 multipl_gauss_avg,
                                 multipl_gauss_stddev,
                                 pixel_dead_time_ns,
                                 pixel_active_time_ns,
                                 random_seed,
                                 create_csv_file);
  } else if(multipl_dist_type == "discrete") {
    QString multipl_dist_file = settings->value("event/hit_multiplicity_distribution_file").toString();

    mEvents = new EventGenerator("event_gen",
                                 bunch_crossing_rate_ns,
                                 average_event_rate_ns,
                                 strobe_length_ns,
                                 multipl_dist_file.toStdString().c_str(),
                                 pixel_dead_time_ns,
                                 pixel_active_time_ns,                                 
                                 random_seed,
                                 create_csv_file);
  }

  if(trigger_filter_enable == true) {
    mEvents->enableTriggerFiltering();      
    mEvents->setTriggerFilterTime(trigger_filter_time_ns);
  }        

  // Connect SystemC signals to EventGenerator
  mEvents->s_clk_in(clock);  
  mEvents->E_trigger_event_available(E_trigger_event_available);
  mEvents->s_strobe_in(s_strobe);

  // Instantiate and connect signals to Alpide
  mAlpide = new AlpideToyModel("alpide", 0);
  mAlpide->s_clk_in(clock);
  mAlpide->s_event_buffers_used(chip_event_buffers_used);
  mAlpide->s_total_number_of_hits(chip_total_number_of_hits);
  
  SC_CTHREAD(stimuliMainProcess, clock.pos());
  
  SC_METHOD(stimuliEventProcess);
  sensitive << E_trigger_event_available;
}


///@brief Main control of simulation stimuli, which mainly involves controlling the
///       strobe signal and stop the simulation after the desired number of events.
void Stimuli::stimuliMainProcess(void)
{
  std::list<int> t_delta_history;
  const int t_delta_averaging_num = 50;
  
  int time_ns = 0;

  int i = 0;

  std::cout << "Staring simulation of " << mNumEvents << " events." << std::endl;
  
  while(simulation_done == false) {
    // Generate strobe pulses for as long as we have more events to simulate
    if(mEvents->getTriggerEventCount() < mNumEvents) {
      std::cout << "Generating strobe/event number " << i << std::endl;

      s_strobe.write(true);
      wait(mStrobeActiveNs, SC_NS);

      s_strobe.write(false);
      wait(mStrobeInactiveNs, SC_NS);

      i++;
    }

    // After all strobes have been generated, allow simulation to run until all events
    // have been read out from the Alpide MEBs.
    else if(mAlpide->getNumEvents() == 0) {
      std::cout << "Finished generating all events, and Alpide chip is done emptying MEBs.\n";
      
      simulation_done = true;
      sc_core::sc_stop();
    }
    else {
      wait();
    }
  }
}


///@brief SystemC controlled method. Waits for EventGenerator to notify the E_trigger_event_available
///       notification queue that a new trigger event is available.
///       When a trigger event is available it is fed to the Alpide chip(s).
void Stimuli::stimuliEventProcess(void)
{
  ///@todo Check if there are actually events?
  ///Throw an error if we get notification but there are not events?
  
  // In a separate block because reference e is invalidated by popNextEvent().
  {
    const TriggerEvent& e = mEvents->getNextTriggerEvent();

    //@todo Implement the whole ITS here, feed events to all relevant chips..
    e.feedHitsToChip(*mAlpide, mAlpide->getChipId());

    std::cout << "Number of events in chip: " << mAlpide->getNumEvents() << std::endl;
    std::cout << "Hits remaining in oldest event in chip: " << mAlpide->getHitsRemainingInOldestEvent();
    std::cout << "  Hits in total (all events): " << mAlpide->getHitTotalAllEvents() << std::endl;
  }

  // Remove the oldest event once we are done processing it..
  mEvents->removeOldestEvent();
}


///@brief Add SystemC signals to log in VCD trace file.
///@todo Make it configurable which traces we want in the file?
void Stimuli::addTraces(sc_trace_file *wf)
{
  sc_trace(wf, chip_event_buffers_used, "chip_event_buffers_used");
  sc_trace(wf, chip_total_number_of_hits, "chip_total_number_of_hits");
}
