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
#include <QSettings>
#include <string>


class Stimuli : public sc_core::sc_module {
public:
  sc_in_clk clock;
  sc_signal<bool> s_physics_event;
  sc_signal<bool > s_chip_ready[100]; // TODO: Replace with vector or something to support more chips
  sc_signal<sc_uint<24> > s_alpide_serial_data[100]; // TODO: Replace with vector or something to support more chips

private:
  sc_event E_physics_event;
  sc_event E_CTP_trigger;

  EventGenerator *mEvents;
  std::vector<Alpide*> mAlpideChips;
  const QSettings* mSettings;
  std::string mOutputPath;
  bool simulation_done = false;
  bool mContinuousMode;

  ///@todo Make it a 64-bit int?
  int mNumEvents;
  int mNumChips;
  int mStrobeActiveNs;
  int mStrobeInactiveNs;
  int mTriggerDelayNs;

public:
  Stimuli(sc_core::sc_module_name name, QSettings* settings, std::string output_path);
  void stimuliMainMethod(void);
  void stimuliEventProcess(void);
  void addTraces(sc_trace_file *wf) const;
  void writeDataToFile(void) const;
};


#endif
///@}
