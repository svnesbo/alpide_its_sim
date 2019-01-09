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

// Ignore warnings about use of auto_ptr in SystemC library
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#include <systemc.h>
#pragma GCC diagnostic pop

#include <QSettings>


class StimuliBase {
public:
  sc_in_clk clock;

private:
  const QSettings* mSettings;
  std::string mOutputPath;
  bool simulation_done = false;
  bool mContinuousMode;
  bool mSingleChipSimulation;

  uint64_t mNumEvents;
  unsigned int mNumChips;
  unsigned int mStrobeActiveNs;
  unsigned int mStrobeInactiveNs;
  unsigned int mTriggerDelayNs;

public:
  StimuliBase(QSettings* settings, std::string output_path);
  virtual void addTraces(sc_trace_file *wf) const = 0;
};


#endif
///@}