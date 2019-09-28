/**
 * @file   StimuliPCT.hpp
 * @author Simon Voigt Nesbo
 * @date   January 9, 2019
 * @brief  Header file for stimuli class for PCT
 */


///@defgroup testbench Main Alpide Simulation Testbench
///@{
#ifndef STIMULI_PCT_HPP
#define STIMULI_PCT_HPP

#include "StimuliBase.hpp"
#include "Alpide/Alpide.hpp"
#include "Event/EventGenPCT.hpp"
#include "Detector/PCT/PCTDetector.hpp"
#include "Detector/Common/ITSModulesStaves.hpp"
#include <QSettings>
#include <memory>
#include <string>


class StimuliPCT : public StimuliBase {
public:
  sc_signal<bool> s_pct_busy;

  // Only used in single chip simulation
  sc_signal<bool> s_alpide_data_line;

private:
  std::unique_ptr<EventGenPCT> mEventGen;

  // mPCT is only used for detector simulation
  std::unique_ptr<PCT::PCTDetector> mPCT;

  // mReadoutUnit and mAlpide is only used for
  // single chip simulations
  std::unique_ptr<ReadoutUnit> mReadoutUnit;
  std::unique_ptr<ITS::SingleChip> mAlpide;

  bool mRandomHitGen;

  void stimuliMethod(void);
  void triggerMethod(void);
  void writeStimuliInfo(void) const;

public:
  StimuliPCT(sc_core::sc_module_name name, QSettings* settings, std::string output_path);
  void addTraces(sc_trace_file *wf) const;
};


#endif
///@}
