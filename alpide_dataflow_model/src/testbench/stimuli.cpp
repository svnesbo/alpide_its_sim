/**
 * @file   stimuli.cpp
 * @author Simon Voigt Nesbo
 * @date   December 12, 2016
 * @brief  Source file for stimuli function for Alpide Dataflow SystemC model
 */

#include "stimuli.h"
#include <systemc.h>
#include <list>
#include <sstream>
#include <fstream>


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
///       Instantiates and initializes the EventGenerator and Alpide objects,
///       connects the SystemC ports
///@param name SystemC module name
///@param settings QSettings object with simulation settings.
///@param output_path Path to store output files generated by the Stimuli class
Stimuli::Stimuli(sc_core::sc_module_name name, QSettings* settings, std::string output_path)
  : sc_core::sc_module(name)
{
  mOutputPath = output_path;
  
  // Initialize variables for Stimuli object
  mNumEvents = settings->value("simulation/n_events").toInt();
  mNumChips = settings->value("simulation/n_chips").toInt();
  mContinuousMode = settings->value("simulation/continuous_mode").toBool();
  mStrobeActiveNs = settings->value("event/strobe_active_length_ns").toInt();
  mStrobeInactiveNs = settings->value("event/strobe_inactive_length_ns").toInt();
  mTriggerDelayNs = settings->value("event/trigger_delay_ns").toInt();  

  bool write_vcd = settings->value("data_output/write_vcd").toBool();


  // Instantiate event generator object
  mEvents = new EventGenerator("event_gen", settings, mOutputPath);

  // Connect SystemC signals to EventGenerator
  mEvents->s_clk_in(clock);  
  mEvents->E_trigger_event_available(E_trigger_event_available);
  mEvents->s_strobe_in(s_strobe);
  mEvents->s_physics_event_out(s_physics_event);

  int region_fifo_size = settings->value("alpide/region_fifo_size").toInt();
  int tru_fifo_size = settings->value("alpide/tru_fifo_size").toInt();
  bool enable_clustering = settings->value("alpide/clustering_enable").toBool();
  
  // Instantiate and connect signals to Alpide
  mAlpideChips.resize(mNumChips);
  for(int i = 0; i < mNumChips; i++) {
    std::stringstream chip_name;
    chip_name << "alpide_" << i;
    mAlpideChips[i] = new Alpide(chip_name.str().c_str(),
                                 i,
                                 region_fifo_size,
                                 tru_fifo_size,
                                 enable_clustering,
                                 mContinuousMode);
    
    mAlpideChips[i]->s_matrix_readout_clk_in(matrix_readout_clock);
    mAlpideChips[i]->s_system_clk_in(clock);
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

  int event_num = 0;

  std::cout << "Staring simulation of " << mNumEvents << " events." << std::endl;
  
  while(simulation_done == false) {
    // Generate strobe pulses for as long as we have more events to simulate
    if(mEvents->getTriggerEventCount() < mNumEvents) {
      if((mEvents->getTriggerEventCount() % 100) == 0) {
        int64_t time_now = sc_time_stamp().value();
        std::cout << "@ " << time_now << " ns: \tGenerating strobe/event number ";
        std::cout << event_num << std::endl;
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

      event_num++;
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

  writeDataToFile();
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
///@param wf VCD waveform file pointer
void Stimuli::addTraces(sc_trace_file *wf) const
{
  sc_trace(wf, s_strobe, "STROBE");
  sc_trace(wf, s_physics_event, "PHYSICS_EVENT");

  // Add traces for all Alpide chips
  for(auto it = mAlpideChips.begin(); it != mAlpideChips.end(); it++) {
    (*it)->addTraces(wf, "");
  }
}


///@brief Write simulation data to file. Histograms for MEB usage from the Alpide chips,
///       and trigger event statistics (number of accepted/rejected) in the chips are recorded here
void Stimuli::writeDataToFile(void) const
{
  std::vector<std::map<unsigned int, std::uint64_t> > alpide_histos;
  unsigned int all_histos_biggest_key = 0;

  std::string csv_filename = mOutputPath + std::string("/MEB_size_histograms.csv");
  ofstream csv_file(csv_filename);

  if(!csv_file.is_open()) {
    std::cerr << "Error opening CSV file for histograms: " << csv_filename << std::endl;
    return;
  }
  
  csv_file << "Multi Event Buffers in use";
  
  // Get histograms from chip objects, and finish writing CSV header 
  for(auto it = mAlpideChips.begin(); it != mAlpideChips.end(); it++) {
    int chip_id = (*it)->getChipId();
    csv_file << ";Chip ID " << chip_id;
    
    alpide_histos.push_back((*it)->getMEBHisto());

    // Check and possibly update the biggest MEB size (key) found in the histograms
    auto current_histo = alpide_histos.back();
    if(current_histo.rbegin() != current_histo.rend()) {
      unsigned int current_histo_biggest_key = current_histo.rbegin()->first;
      if(all_histos_biggest_key < current_histo_biggest_key)
        all_histos_biggest_key = current_histo_biggest_key;
    }
  }

  // Write values to CSV file
  for(unsigned int MEB_size = 0; MEB_size <= all_histos_biggest_key; MEB_size++) {
    csv_file << std::endl;
    csv_file << MEB_size;
    
    for(unsigned int i = 0; i < alpide_histos.size(); i++) {
      csv_file << ";";
      
      auto histo_it = alpide_histos[i].find(MEB_size);

      // Write value if it was found in histogram
      if(histo_it != alpide_histos[i].end())
        csv_file << histo_it->second;
      else
        csv_file << 0;
    }
  }

  
  std::string trigger_stats_filename = mOutputPath + std::string("/trigger_events_stats.csv");
  ofstream trigger_stats_file(trigger_stats_filename);

  trigger_stats_file << "Chip ID; Accepted trigger events; Rejected trigger events" << std::endl;
  for(auto it = mAlpideChips.begin(); it != mAlpideChips.end(); it++) {
    trigger_stats_file << (*it)->getChipId() << ";";
    trigger_stats_file << (*it)->getTriggerEventsAcceptedCount() << ";";
    trigger_stats_file << (*it)->getTriggerEventsRejectedCount() << std::endl;
  }
}
