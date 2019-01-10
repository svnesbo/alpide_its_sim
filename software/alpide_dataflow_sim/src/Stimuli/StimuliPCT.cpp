/**
 * @file   Stimuli.cpp
 * @author Simon Voigt Nesbo
 * @date   December 12, 2016
 * @brief  Source file for stimuli function for Alpide Dataflow SystemC model
 */

#include "StimuliPCT.hpp"
#include "../ITS/ITSSimulationStats.hpp"

// Ignore warnings about use of auto_ptr in SystemC library
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#include <systemc.h>
#pragma GCC diagnostic pop

#include <list>
#include <sstream>
#include <fstream>

extern volatile bool g_terminate_program;


SC_HAS_PROCESS(StimuliPCT);
///@brief Constructor for stimuli class.
///       Instantiates and initializes the EventGenerator and Alpide objects,
///       connects the SystemC ports
///@param[in] name SystemC module name
///@param[in] settings QSettings object with simulation settings.
///@param[in] output_path Path to store output files generated by the Stimuli class
StimuliPCT::StimuliPCT(sc_core::sc_module_name name, QSettings* settings, std::string output_path)
  : StimuliBase(name, settings, output_path)
{
  mOutputPath = output_path;

  // Initialize variables for Stimuli object
  mNumEvents = settings->value("simulation/n_events").toULongLong();
  mSingleChipSimulation = settings->value("simulation/single_chip").toBool();
  mContinuousMode = settings->value("simulation/continuous_mode").toBool();
  mStrobeActiveNs = settings->value("event/strobe_active_length_ns").toUInt();
  mStrobeInactiveNs = settings->value("event/strobe_inactive_length_ns").toUInt();
  mTriggerDelayNs = settings->value("event/trigger_delay_ns").toUInt();

  unsigned int trigger_filter_time = settings->value("event/trigger_filter_time_ns").toUInt();
  bool trigger_filter_enable = settings->value("event/trigger_filter_enable").toBool();
  int dtu_delay = settings->value("alpide/dtu_delay").toUInt();
  bool enable_data_long = settings->value("alpide/data_long_enable").toBool();
  bool matrix_readout_speed = settings->value("alpide/matrix_readout_speed_fast").toBool();
  bool strobe_extension = settings->value("alpide/strobe_extension_enable").toBool();
  unsigned int min_busy_cycles = settings->value("alpide/minimum_busy_cycles").toUInt();

  std::cout << std::endl;
  std::cout << "-------------------------------------------------" << std::endl;
  std::cout << "Simulation settings:" << std::endl;
  std::cout << "-------------------------------------------------" << std::endl;
  std::cout << "Number of events: " << mNumEvents << std::endl;
  std::cout << "Single chip simulation: " << (mSingleChipSimulation ? "true" : "false") << std::endl;
  std::cout << "Trigger mode: " << (mContinuousMode ? "continuous" : "triggered") << std::endl;
  std::cout << "Strobe active time (ns): " << mStrobeActiveNs << std::endl;
  std::cout << "Strobe inactive time (ns): " << mStrobeInactiveNs << std::endl;
  std::cout << "Trigger delay (ns): " << mTriggerDelayNs << std::endl;
  std::cout << "Trigger filter time (ns): " << trigger_filter_time << std::endl;
  std::cout << "Trigger filter enabled: " << (trigger_filter_enable ? "true" : "false") << std::endl;
  std::cout << "DTU delay (clock cycles): " << dtu_delay << std::endl;
  std::cout << "Data long enabled: " << (enable_data_long ? "true" : "false") << std::endl;
  std::cout << "Matrix readout speed fast: " << (matrix_readout_speed ? "true" : "false") << std::endl;
  std::cout << "Strobe extension enabled: " << (strobe_extension ? "true" : "false") << std::endl;

  std::cout << "Layer 0 hit density: ";
  std::cout << settings->value("event/hit_density_layer0").toDouble() << std::endl;

  std::cout << "Layer 1 hit density: ";
  std::cout << settings->value("event/hit_density_layer1").toDouble() << std::endl;

  std::cout << "Layer 2 hit density: ";
  std::cout << settings->value("event/hit_density_layer2").toDouble() << std::endl;

  std::cout << "Layer 3 hit density: ";
  std::cout << settings->value("event/hit_density_layer3").toDouble() << std::endl;

  std::cout << "Layer 4 hit density: ";
  std::cout << settings->value("event/hit_density_layer4").toDouble() << std::endl;

  std::cout << "Layer 5 hit density: ";
  std::cout << settings->value("event/hit_density_layer5").toDouble() << std::endl;

  std::cout << "Layer 6 hit density: ";
  std::cout << settings->value("event/hit_density_layer6").toDouble() << std::endl;

  std::cout << "Layer 0 number of staves: ";
  std::cout << settings->value("its/layer0_num_staves").toUInt() << std::endl;

  std::cout << "Layer 1 number of staves: ";
  std::cout << settings->value("its/layer1_num_staves").toUInt() << std::endl;

  std::cout << "Layer 2 number of staves: ";
  std::cout << settings->value("its/layer2_num_staves").toUInt() << std::endl;

  std::cout << "Layer 3 number of staves: ";
  std::cout << settings->value("its/layer3_num_staves").toUInt() << std::endl;

  std::cout << "Layer 4 number of staves: ";
  std::cout << settings->value("its/layer4_num_staves").toUInt() << std::endl;

  std::cout << "Layer 5 number of staves: ";
  std::cout << settings->value("its/layer5_num_staves").toUInt() << std::endl;

  std::cout << "Layer 6 number of staves: ";
  std::cout << settings->value("its/layer6_num_staves").toUInt() << std::endl;

  mEventGen = std::move(std::unique_ptr<EventGenerator>(new EventGenerator("event_gen",
                                                                           settings,
                                                                           mOutputPath)));

  if(mSingleChipSimulation) {
    mAlpide = std::move(std::unique_ptr<ITS::SingleChip>(new ITS::SingleChip("SingleChip",
                                                                             0,
                                                                             dtu_delay,
                                                                             mStrobeActiveNs,
                                                                             strobe_extension,
                                                                             enable_data_long,
                                                                             mContinuousMode,
                                                                             matrix_readout_speed,
                                                                             min_busy_cycles)));

    mAlpide->s_system_clk_in(clock);

    mReadoutUnit = std::move(std::unique_ptr<ReadoutUnit>(new ReadoutUnit("RU",
                                                                          0,
                                                                          0,
                                                                          1,
                                                                          1,
                                                                          trigger_filter_time,
                                                                          trigger_filter_enable,
                                                                          true)));

    mReadoutUnit->s_busy_in(mReadoutUnit->s_busy_out);
    mReadoutUnit->s_system_clk_in(clock);
    mReadoutUnit->s_serial_data_input[0](mAlpide->s_alpide_data_out_exp);
    mReadoutUnit->s_alpide_control_output[0].bind(mAlpide->socket_control_in[0]);
    mAlpide->socket_data_out[0].bind(mReadoutUnit->s_alpide_data_input[0]);
  }
  else { // ITS Detector Simulation
    ITS::detectorConfig config;
    config.layer[0].num_staves = settings->value("its/layer0_num_staves").toInt();
    config.layer[1].num_staves = settings->value("its/layer1_num_staves").toInt();
    config.layer[2].num_staves = settings->value("its/layer2_num_staves").toInt();
    config.layer[3].num_staves = settings->value("its/layer3_num_staves").toInt();
    config.layer[4].num_staves = settings->value("its/layer4_num_staves").toInt();
    config.layer[5].num_staves = settings->value("its/layer5_num_staves").toInt();
    config.layer[6].num_staves = settings->value("its/layer6_num_staves").toInt();

    config.alpide_dtu_delay_cycles = dtu_delay;
    config.alpide_strobe_length_ns = mStrobeActiveNs;
    config.alpide_min_busy_cycles = min_busy_cycles;
    config.alpide_strobe_ext = strobe_extension;
    config.alpide_data_long_en = enable_data_long;
    config.alpide_matrix_speed = matrix_readout_speed;
    config.alpide_continuous_mode = mContinuousMode;

    mITS = std::move(std::unique_ptr<ITS::ITSDetector>(new ITS::ITSDetector("ITS", config,
                                                                            trigger_filter_time,
                                                                            trigger_filter_enable)));
    mITS->s_system_clk_in(clock);
    mITS->s_detector_busy_out(s_its_busy);
  }

  s_physics_event = false;

  if(mContinuousMode == true) {
    SC_METHOD(continuousTriggerMethod);
  }

  SC_METHOD(stimuliMainMethod);
  sensitive << mEventGen->E_physics_event;
  dont_initialize();

  SC_METHOD(stimuliQedNoiseEventMethod);
  sensitive << mEventGen->E_qed_noise_event;
  dont_initialize();

  // This method just generates a (VCD traceable) SystemC signal
  // that coincides with the physics event from the event generator
  SC_METHOD(physicsEventSignalMethod);
  sensitive << mEventGen->E_physics_event;
  dont_initialize();
}


///@brief Main control of simulation stimuli
void StimuliPCT::stimuliMainMethod(void)
{
  if(simulation_done == true || g_terminate_program == true) {
    int64_t time_now = sc_time_stamp().value();
    std::cout << "@ " << time_now << " ns: \tSimulation done" << std::endl;

    sc_core::sc_stop();

    writeStimuliInfo();

    if(mSingleChipSimulation)
      writeAlpideStatsToFile(mOutputPath, mAlpide->getChips());
    else
      mITS->writeSimulationStats(mOutputPath);

    mEventGen->writeSimulationStats(mOutputPath);
  }
  // We want to stop at n_events, not n_events-1.
  else if(mEventGen->getPhysicsEventCount() <= mNumEvents) {
    //if((mEventGen->getPhysicsEventCount() % 100) == 0) {
    int64_t time_now = sc_time_stamp().value();
    std::cout << "@ " << time_now << " ns: \tPhysics event number ";
    std::cout << mEventGen->getPhysicsEventCount() << std::endl;
    //}

    std::cout << "Feeding " << mEventGen->getLatestPhysicsEvent().size() << " pixels to ITS detector." << std::endl;
    // Get hits for this event, and "feed" them to the ITS detector
    auto event_hits = mEventGen->getLatestPhysicsEvent();

    if(mSingleChipSimulation) {
      for(auto it = event_hits.begin(); it != event_hits.end(); it++)
        mAlpide->pixelInput(*it);

      std::cout << "Creating event for next trigger.." << std::endl;

      if(mContinuousMode == false) {
        // Create an event for the next trigger, delayed by the
        // total/specified trigger delay (to account for cable/CTP delays etc.)
        mReadoutUnit->E_trigger_in.notify(mTriggerDelayNs, SC_NS);
      }
    }
    else {
      for(auto it = event_hits.begin(); it != event_hits.end(); it++)
        mITS->pixelInput(*it);

      std::cout << "Creating event for next trigger.." << std::endl;

      if(mContinuousMode == false) {
      // Create an event for the next trigger, delayed by the
      // total/specified trigger delay (to account for cable/CTP delays etc.)
        mITS->E_trigger_in.notify(mTriggerDelayNs, SC_NS);
      }
    }

    if(mEventGen->getPhysicsEventCount() == mNumEvents) {
      // When we have reached the desired number of events, or upon CTRL+C, allow simulation
      // to run for another X us to allow readout of data remaining in MEBs, FIFOs etc.
      next_trigger(100, SC_US);
      simulation_done = true;
      mEventGen->stopEventGeneration();
    } else {
      next_trigger(mEventGen->E_physics_event);
    }
  }
}


///@brief SystemC method for feeding QED and noise events that are
///       not associated with a trigger to the ALPIDE chips.
void Stimuli::stimuliQedNoiseEventMethod(void)
{
    // Get hits for this event, and "feed" them to the ITS detector
    auto event_hits = mEventGen->getLatestQedNoiseEvent();

    if(mSingleChipSimulation) {
      for(auto it = event_hits.begin(); it != event_hits.end(); it++)
        mAlpide->pixelInput(*it);
    }
    else {
      for(auto it = event_hits.begin(); it != event_hits.end(); it++)
        mITS->pixelInput(*it);
    }
}


///@brief SystemC method for generating triggers in continuous mode
void Stimuli::continuousTriggerMethod(void)
{
  if(mSingleChipSimulation)
    mReadoutUnit->E_trigger_in.notify(mTriggerDelayNs, SC_NS);
  else
    mITS->E_trigger_in.notify(mTriggerDelayNs, SC_NS);

  next_trigger(mStrobeActiveNs+mStrobeInactiveNs, SC_NS);
}


///@brief This SystemC method just toggles the s_physics_event for a clock cycle
///       signal every time we get an E_physics_event from the event generator,
///       so that we can have a signal for this that we can add to the trace file.
void Stimuli::physicsEventSignalMethod(void)
{
  if(s_physics_event.read() == true) {
    s_physics_event.write(false);
    next_trigger(mEventGen->E_physics_event);
  } else {
    s_physics_event.write(true);
    next_trigger(25,SC_NS);
  }
}


///@brief Add SystemC signals to log in VCD trace file.
///@param[in,out] wf VCD waveform file pointer
void Stimuli::addTraces(sc_trace_file *wf) const
{
  sc_trace(wf, s_physics_event, "PHYSICS_EVENT");

  sc_trace(wf, s_its_busy, "its_busy");
  sc_trace(wf, s_alpide_data_line, "alpide_data_line");

  if(mSingleChipSimulation) {
    //mReadoutUnit->addTraces(wf, "");
    mAlpide->addTraces(wf, "");
  } else {
    mITS->addTraces(wf, "");
  }
}


void Stimuli::writeStimuliInfo(void) const
{
  std::string info_filename = mOutputPath + std::string("/simulation_info.txt");
  ofstream info_file(info_filename);

  if(!info_file.is_open()) {
    std::cerr << "Error opening simulation info file: " << info_filename << std::endl;
    return;
  }

  info_file << "Number of physics events requested: " << mNumEvents << std::endl;

  info_file << "Number of physics events simulated: ";
  info_file << mEventGen->getPhysicsEventCount() << std::endl;
}
