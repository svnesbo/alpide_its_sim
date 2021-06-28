/**
 * @file   StimuliBase.hpp
 * @author Simon Voigt Nesbo
 * @date   November 19, 2018
 * @brief  Header file for stimuli base class for Alpide Dataflow SystemC model
 */


///@defgroup testbench Main Alpide Simulation Testbench
///@{
#ifndef STIMULI_BASE_HPP
#define STIMULI_BASE_HPP

// Ignore warnings about use of auto_ptr and unused parameters in SystemC library
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#include <systemc.h>
#pragma GCC diagnostic pop

#include <QSettings>
#include "Alpide/AlpideConfig.hpp"

class StimuliBase : public sc_core::sc_module
{
public:
  sc_in_clk clock;

protected:
  const QSettings* mSettings;
  std::string mOutputPath;
  bool simulation_done = false;
  bool mSystemContinuousMode;
  bool mSingleChipSimulation;

  uint64_t mNumEvents;
  unsigned int mNumChips;
  unsigned int mSystemContinuousPeriodNs;
  unsigned int mStrobeActiveNs;
  unsigned int mStrobeInactiveNs;
  unsigned int mTriggerDelayNs;
  unsigned int mTriggerFilterTimeNs;
  bool mTriggerFilterEnabled;
  unsigned int mDataRateIntervalNs;

  AlpideConfig mChipCfg;

public:
  StimuliBase(sc_core::sc_module_name name, QSettings* settings, std::string output_path);
  virtual void addTraces(sc_trace_file *wf) const = 0;
};


#endif
///@}
