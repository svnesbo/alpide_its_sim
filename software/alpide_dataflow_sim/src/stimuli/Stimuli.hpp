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
#include "../CTP/CTP.hpp"
#include <QSettings>
#include <string>


class Stimuli : public sc_core::sc_module {
public:
  sc_in_clk clock;
  sc_signal<bool> s_physics_event;
  sc_signal<bool> s_its_busy;

private:
  ///@todo Remove?
  //  sc_event E_physics_event;   // Event Gen to Stim class

  sc_event E_physics_trigger; // EventGenerator physics event to CTP
  sc_event E_CTP_trigger;     // CTP to detector/chip

  EventGenerator *mEventGen;
  ITS::ITSDetector *mITS;
  //CTP *mCTP;

  const QSettings* mSettings;
  std::string mOutputPath;
  bool simulation_done = false;
  bool mContinuousMode;
  bool mSingleChipSimulation;

  ///@todo Make it a 64-bit int?
  int mNumEvents;
  int mNumChips;
  int mStrobeActiveNs;
  int mStrobeInactiveNs;
  int mTriggerDelayNs;

public:
  Stimuli(sc_core::sc_module_name name, QSettings* settings, std::string output_path);
  void stimuliMainMethod(void);
  void physicsEventSignalMethod(void);
  void stimuliEventProcess(void);
  void addTraces(sc_trace_file *wf) const;
  void writeDataToFile(void) const;
};


#endif
///@}
