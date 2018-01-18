/**
 * @file   Stimuli.hpp
 * @author Simon Voigt Nesbo
 * @date   December 12, 2016
 * @brief  Header file for stimuli function for Alpide Dataflow SystemC model
 */


///@defgroup testbench Main Alpide Simulation Testbench
///@{
#ifndef STIMULI_HPP
#define STIMULI_HPP

#include "Alpide/Alpide.hpp"
#include "../event/EventGenerator.hpp"
#include "../ITS/ITSDetector.hpp"
#include "../ITS/ITSModulesStaves.hpp"
#include <QSettings>
#include <string>


class Stimuli : public sc_core::sc_module {
public:
  sc_in_clk clock;
  sc_signal<bool> s_physics_event;
  sc_signal<bool> s_its_busy;

  // Only used in single chip simulation
  sc_signal<bool> s_alpide_data_line;

private:
  EventGenerator *mEventGen;

  // mITS is only used for detector simulation
  ITS::ITSDetector *mITS;

  // mReadoutUnit and mAlpide is only used for
  // single chip simulations
  ReadoutUnit *mReadoutUnit;
  ITS::SingleChip *mAlpide;

  const QSettings* mSettings;
  std::string mOutputPath;
  bool simulation_done = false;
  bool mContinuousMode;
  bool mSingleChipSimulation;

  ///@todo Make it a 64-bit int?
  uint64_t mNumEvents;
  unsigned int mNumChips;
  unsigned int mStrobeActiveNs;
  unsigned int mStrobeInactiveNs;
  unsigned int mTriggerDelayNs;

public:
  Stimuli(sc_core::sc_module_name name, QSettings* settings, std::string output_path);
  void stimuliMainMethod(void);
  void stimuliQedNoiseEventMethod(void);
  void continuousTriggerMethod(void);
  void physicsEventSignalMethod(void);
  void stimuliEventProcess(void);
  void addTraces(sc_trace_file *wf) const;
  void writeStimuliInfo(void) const;
};


#endif
///@}
