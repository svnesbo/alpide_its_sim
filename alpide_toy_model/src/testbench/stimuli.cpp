/**
 * @file   stimuli.cpp
 * @author Simon Voigt Nesbo
 * @date   December 12, 2016
 * @brief  Source file for stimuli function for Alpide SystemC model
 */

#include "stimuli.h"
#include <systemc.h>
#include <list>
#include <sstream>


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
///@param settings QSettings object with simulation settings.
Stimuli::Stimuli(sc_core::sc_module_name name, QSettings* settings)
  : sc_core::sc_module(name)
{
  // Initialize variables for Stimuli object
  mNumEvents = settings->value("simulation/n_events").toInt();
  mNumChips = settings->value("simulation/n_chips").toInt();
  mContinuousMode = settings->value("simulation/continuous_mode").toBool();
  mStrobeActiveNs = settings->value("event/strobe_active_length_ns").toInt();
  mStrobeInactiveNs = settings->value("event/strobe_inactive_length_ns").toInt();
  mTriggerDelayNs = settings->value("event/trigger_delay_ns").toInt();  

  bool write_vcd = settings->value("data_output/write_vcd").toBool();


  // Instantiate event generator object
  mEvents = new EventGenerator("event_gen", settings);

  // Connect SystemC signals to EventGenerator
  mEvents->s_clk_in(clock);  
  mEvents->E_trigger_event_available(E_trigger_event_available);
  mEvents->s_strobe_in(s_strobe);
  mEvents->s_physics_event_out(s_physics_event);

  // Instantiate and connect signals to Alpide
  mAlpideChips.resize(mNumChips);
  for(int i = 0; i < mNumChips; i++) {
    std::stringstream chip_name;
    chip_name << "alpide_" << i;
    mAlpideChips[i] = new AlpideToyModel(chip_name.str().c_str(), i, write_vcd);
    mAlpideChips[i]->s_clk_in(clock);
//    mAlpide->s_event_buffers_used(chip_event_buffers_used);
//    mAlpide->s_total_number_of_hits(chip_total_number_of_hits);
  }
  
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
      if((mEvents->getTriggerEventCount() % 100) == 0) {
        int64_t time_now = sc_time_stamp().value();
        std::cout << "@ " << time_now << " ns: \tGenerating strobe/event number " << i << std::endl;
      }

      if(mContinuousMode == true) {
        s_strobe.write(true);
        wait(mStrobeActiveNs, SC_NS);

        s_strobe.write(false);
        wait(mStrobeInactiveNs, SC_NS);
      } else {
        wait(s_physics_event.value_changed_event());
        if(s_physics_event.read() == true) {
          wait(mTriggerDelayNs, SC_NS);
          s_strobe.write(true);
        
          wait(mStrobeActiveNs, SC_NS);
          s_strobe.write(false);
        }
      }

      i++;
    }

    // After all strobes have been generated, allow simulation to run until all events
    // have been read out from the Alpide MEBs.
    else {
      int events_left = 0;
      
      // Check if the Alpide chips still have events to read out
      for(int i = 0; i < mNumChips; i++)
        events_left += mAlpideChips[i]->getNumEvents();
          
      if(events_left == 0) {
        std::cout << "Finished generating all events, and Alpide chip is done emptying MEBs.\n";
      
        simulation_done = true;
        sc_core::sc_stop();
      } else {
        wait();
      }
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

    // Don't process if we received NoTriggerEvent
    if(e.getEventId() != -1) {
      int chip_id = e.getChipId();
      e.feedHitsToChip(*mAlpideChips[chip_id]);

      #ifdef DEBUG_OUTPUT
      std::cout << "Number of events in chip: " << mAlpideChips[chip_id]->getNumEvents() << std::endl;
      std::cout << "Hits remaining in oldest event in chip: " << mAlpideChips[chip_id]->getHitsRemainingInOldestEvent();
      std::cout << "  Hits in total (all events): " << mAlpideChips[chip_id]->getHitTotalAllEvents() << std::endl;
      #endif
    
      // Remove the oldest event once we are done processing it..
      mEvents->removeOldestEvent();
    }
  }
}


///@brief Add SystemC signals to log in VCD trace file.
///@todo Make it configurable which traces we want in the file?
void Stimuli::addTraces(sc_trace_file *wf) const
{
  sc_trace(wf, s_strobe, "STROBE");
  sc_trace(wf, s_physics_event, "PHYSICS_EVENT");
  
  for(auto it = mAlpideChips.begin(); it != mAlpideChips.end(); it++) {
    (*it)->addTraces(wf);
  }
}
