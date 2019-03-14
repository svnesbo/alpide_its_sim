/**
 * @file   StimuliFocal.hpp
 * @author Simon Voigt Nesbo
 * @date   March 8, 2019
 * @brief  Header file for stimuli class for Focal
 */


///@defgroup testbench Main Alpide Simulation Testbench
///@{
#ifndef STIMULI_FOCAL_HPP
#define STIMULI_FOCAL_HPP

#include "StimuliBase.hpp"
#include "Alpide/Alpide.hpp"
#include "Event/EventGenITS.hpp"
#include "Detector/PCT/PCTDetector.hpp"
#include "Detector/Common/ITSModulesStaves.hpp"
#include <QSettings>
#include <memory>
#include <string>


class StimuliFocal : public StimuliBase {
private:
  sc_signal<bool> s_physics_event;
  sc_signal<bool> s_focal_busy;

  // Only used in single chip simulation
  sc_signal<bool> s_alpide_data_line;

private:
  std::unique_ptr<EventGenITS> mEventGen;

  // mFocal is only used for detector simulation
  std::unique_ptr<PCT::PCTDetector> mFocal;

  // mReadoutUnit and mAlpide is only used for
  // single chip simulations
  std::unique_ptr<ReadoutUnit> mReadoutUnit;
  std::unique_ptr<ITS::SingleChip> mAlpide;

  void stimuliMainMethod(void);
  void stimuliQedNoiseEventMethod(void);
  void continuousTriggerMethod(void);
  void physicsEventSignalMethod(void);
  void writeStimuliInfo(void) const;
public:
  StimuliFocal(sc_core::sc_module_name name, QSettings* settings, std::string output_path);
  void addTraces(sc_trace_file *wf) const;
};


#endif
///@}
