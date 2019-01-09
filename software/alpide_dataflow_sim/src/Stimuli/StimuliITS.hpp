/**
 * @file   StimuliITS.hpp
 * @author Simon Voigt Nesbo
 * @date   November 19, 2018
 * @brief  Header file for stimuli class for ITS
 */


///@defgroup testbench Main Alpide Simulation Testbench
///@{
#ifndef STIMULI_ITS_HPP
#define STIMULI_ITS_HPP

#include "StimuliBase.hpp"
#include "Alpide/Alpide.hpp"
#include "../Event/EventGenITS.hpp"
#include "../ITS/ITSDetector.hpp"
#include "../ITS/ITSModulesStaves.hpp"
#include <QSettings>
#include <memory>
#include <string>


class StimuliITS : public StimuliBase, public sc_core::sc_module {
private:
  sc_signal<bool> s_physics_event;
  sc_signal<bool> s_its_busy;

  // Only used in single chip simulation
  sc_signal<bool> s_alpide_data_line;


  std::unique_ptr<EventGenITS> mEventGen;

  // mITS is only used for detector simulation
  std::unique_ptr<ITS::ITSDetector> mITS;

  // mReadoutUnit and mAlpide is only used for
  // single chip simulations
  std::unique_ptr<ReadoutUnit> mReadoutUnit;
  std::unique_ptr<ITS::SingleChip> mAlpide;

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
  StimuliITS(sc_core::sc_module_name name, QSettings* settings, std::string output_path);
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
