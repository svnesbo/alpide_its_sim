/**
 * @file   StimuliPCT.cpp
 * @author Simon Voigt Nesbo
 * @date   January 16, 2019
 * @brief  Source file for Stimuli class for PCT
 */

#include "StimuliPCT.hpp"
#include "Detector/Common/DetectorSimulationStats.hpp"

// Ignore warnings about use of auto_ptr and unused parameters in SystemC library
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#include <systemc.h>
#pragma GCC diagnostic pop

#include <list>
#include <sstream>
#include <fstream>

extern volatile bool g_terminate_program;


SC_HAS_PROCESS(StimuliPCT);
///@brief Constructor for stimuli class.
///       Instantiates and initializes the event generator and Alpide objects,
///       connects the SystemC ports
///@param[in] name SystemC module name
///@param[in] settings QSettings object with simulation settings.
///@param[in] output_path Path to store output files generated by the Stimuli class
StimuliPCT::StimuliPCT(sc_core::sc_module_name name, QSettings* settings, std::string output_path)
  : StimuliBase(name, settings, output_path)
{
  std::cout << "Number of layers: ";
  std::cout << settings->value("pct/num_layers").toUInt() << std::endl;

  std::cout << "Number of staves per layer: ";
  std::cout << settings->value("pct/num_staves_per_layer").toUInt() << std::endl;

  std::cout << "Length of event time frame (ns): ";
  std::cout << settings->value("pct/time_frame_length_ns").toUInt() << std::endl;

  std::cout << "Number of particles generated with random generator per second (mean): ";
  std::cout << settings->value("pct/random_particles_per_s_mean").toDouble() << std::endl;

  std::cout << "Number of particles generated with random generator per second (stddev): ";
  std::cout << settings->value("pct/random_particles_per_s_stddev").toDouble() << std::endl;

  std::cout << "Standard deviation for beam coords with random generator (mm): ";
  std::cout << settings->value("pct/random_beam_stddev_mm").toDouble() << std::endl;

  std::cout << "Beam start coord (mm): (";
  std::cout << settings->value("pct/beam_start_coord_x_mm").toDouble();
  std::cout << ",";
  std::cout << settings->value("pct/beam_start_coord_y_mm").toDouble();
  std::cout << ")"  << std::endl;

  std::cout << "Beam end coord (mm): (";
  std::cout << settings->value("pct/beam_end_coord_x_mm").toDouble();
  std::cout << ",";
  std::cout << settings->value("pct/beam_end_coord_y_mm").toDouble();
  std::cout << ")"  << std::endl;

  std::cout << "Beam speed along x-axis (mm per us): ";
  std::cout << settings->value("pct/beam_speed_x_mm_per_us").toDouble() << std::endl;

  std::cout << "Beam step along y-axis (mm): ";
  std::cout << settings->value("pct/beam_step_y_mm").toDouble() << std::endl;

  std::cout << std::endl << std::endl;

  if(mSystemContinuousMode == false) {
    std::string error_msg = "System continuous mode must be true for pCT.";
    throw std::runtime_error(error_msg);
  }

  // Initialize detector configuration for PCT.
  // Doing it here because event generator expects this parameter,
  // though it is not used for single chip simulation
  PCT::PCTDetectorConfig config;
  std::string layer_config_str = settings->value("pct/layers").toString().toStdString();

  // Deactive all layers..
  for(unsigned int i = 0; i < PCT::N_LAYERS; i++) {
      config.layer[i].num_staves = 0;
  }


  // ..and then active the layers that are included in the configuration
  while(layer_config_str.length() > 0) {
    // Expect a semicolon delimited string of layers, eg. "0;5;10"
    std::string::size_type delim_pos = layer_config_str.find(";");
    std::string layer_str = layer_config_str.substr(0, delim_pos);
    unsigned int layer = std::stoi(layer_str);

    if(delim_pos == std::string::npos)
      layer_config_str.erase(0);
    else
      layer_config_str.erase(0, delim_pos+1);

    std::cout << "Layer: " << layer << std::endl;

    // Add layer to detector configuration
    config.layer[layer].num_staves = settings->value("pct/num_staves_per_layer").toUInt();
  }

  config.chip_cfg = mChipCfg;

  mEventGen = std::move(std::unique_ptr<EventGenPCT>(new EventGenPCT("event_gen",
                                                                     settings,
                                                                     config,
                                                                     mOutputPath)));

  if(mSingleChipSimulation) {
    mAlpide = std::move(std::unique_ptr<ITS::SingleChip>(new ITS::SingleChip("SingleChip",
                                                                             0,
                                                                             mChipCfg)));

    mAlpide->s_system_clk_in(clock);

    mReadoutUnit = std::move(std::unique_ptr<ReadoutUnit>(new ReadoutUnit("RU",
                                                                          0,
                                                                          0,
                                                                          1,
                                                                          1,
                                                                          mTriggerFilterTimeNs,
                                                                          mTriggerFilterEnabled,
                                                                          true,
                                                                          mDataRateIntervalNs)));

    mReadoutUnit->s_busy_in(mReadoutUnit->s_busy_out);
    mReadoutUnit->s_system_clk_in(clock);
    mReadoutUnit->s_serial_data_input[0](mAlpide->s_alpide_data_out_exp);
    mReadoutUnit->s_alpide_control_output[0].bind(mAlpide->socket_control_in[0]);
    mAlpide->socket_data_out[0].bind(mReadoutUnit->s_alpide_data_input[0]);
  }
  else { // ITS Detector Simulation
    mPCT = std::move(std::unique_ptr<PCT::PCTDetector>(new PCT::PCTDetector("PCT", config,
                                                                            mTriggerFilterTimeNs,
                                                                            mTriggerFilterEnabled,
                                                                            mDataRateIntervalNs)));
    mPCT->s_system_clk_in(clock);
    mPCT->s_detector_busy_out(s_pct_busy);
  }

  SC_METHOD(triggerMethod);

  SC_METHOD(stimuliMethod);
  sensitive << mEventGen->E_untriggered_event;
  dont_initialize();
}


///@brief Main control of simulation stimuli
void StimuliPCT::stimuliMethod(void)
{
  if(simulation_done == true) {
    uint64_t time_now = sc_time_stamp().value();
    std::cout << "@ " << time_now << " ns: \tSimulation done" << std::endl;

    sc_core::sc_stop();

    writeStimuliInfo();

    if(mSingleChipSimulation)
      Detector::writeAlpideStatsToFile(mOutputPath,
                                       mAlpide->getChips(),
                                       &PCT::PCT_global_chip_id_to_position);
    else
      mPCT->writeSimulationStats(mOutputPath);

    mEventGen->writeSimulationStats(mOutputPath);
  }
  else {
    uint64_t time_now = sc_time_stamp().value();
    std::cout << "@ " << time_now << " ns: \tEvent frame number ";
    std::cout << mEventGen->getUntriggeredEventCount() << std::endl;
    std::cout << "\tBeam coords (mm): (";
    std::cout << mEventGen->getBeamCenterCoordX() << ",";
    std::cout << mEventGen->getBeamCenterCoordY() << ")" << std::endl;

    // Get hits for this event, and "feed" them to the PCT detector
    auto event_hits = mEventGen->getUntriggeredEvent();

    if(mSingleChipSimulation) {
      std::cout << "Feeding " << event_hits.size() << " pixels to Alpide chip." << std::endl;

      for(auto it = event_hits.begin(); it != event_hits.end(); it++)
        mAlpide->pixelInput(*it);
    }
    else {
      std::cout << "Feeding " << event_hits.size() << " pixels to PCT detector." << std::endl;

      for(auto it = event_hits.begin(); it != event_hits.end(); it++)
        mPCT->pixelInput(*it);

      std::cout << "Creating event for next trigger.." << std::endl;
    }

    if(mEventGen->getBeamEndCoordsReached() == true || g_terminate_program == true) {
      // When the beam has reached the specified end position, the simulation should end.
      // But we allow the simulation to run for another X us to allow readout of data
      // remaining in MEBs, FIFOs etc.
      next_trigger(100, SC_US);
      simulation_done = true;
      mEventGen->stopEventGeneration();
    } else {
      next_trigger(mEventGen->E_untriggered_event);
    }
  }
}


///@brief SystemC method for generating triggers
void StimuliPCT::triggerMethod(void)
{
  if(mSingleChipSimulation)
    mReadoutUnit->E_trigger_in.notify(mTriggerDelayNs, SC_NS);
  else
    mPCT->E_trigger_in.notify(mTriggerDelayNs, SC_NS);

  next_trigger(mSystemContinuousPeriodNs, SC_NS);
}


///@brief Add SystemC signals to log in VCD trace file.
///@param[in,out] wf VCD waveform file pointer
void StimuliPCT::addTraces(sc_trace_file *wf) const
{
  sc_trace(wf, s_pct_busy, "pct_busy");


  if(mSingleChipSimulation) {
    sc_trace(wf, s_alpide_data_line, "alpide_data_line");
    mAlpide->addTraces(wf, "");
  } else {
    mPCT->addTraces(wf, "");
  }
}


void StimuliPCT::writeStimuliInfo(void) const
{
  std::string info_filename = mOutputPath + std::string("/simulation_info.txt");
  ofstream info_file(info_filename);

  if(!info_file.is_open()) {
    std::cerr << "Error opening simulation info file: " << info_filename << std::endl;
    return;
  }

  // Analysis scripts expects the file to have this format
  info_file << "Number of triggered events requested: " << 0 << std::endl;
  info_file << "Number of triggered events simulated: " << 0 << std::endl;
  info_file << "Number of untriggered events requested: " << 0 << std::endl;
  info_file << "Number of untriggered events simulated: ";
  info_file << mEventGen->getUntriggeredEventCount() << std::endl;
}
