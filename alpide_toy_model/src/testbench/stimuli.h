/**
 * @file   stimuli.h
 * @Author Simon Voigt Nesbo
 * @date   December 12, 2016
 * @brief  Header file for stimuli function for Alpide SystemC model
 *
 * Detailed description of file.
 */

#ifndef STIMULI_H
#define STIMULI_H

#include "../alpide/alpide_toy_model.h"
#include "../event/event_generator.h"
#include <QSettings>


class Stimuli : public sc_core::sc_module {
public:
  sc_in_clk clock;
  sc_signal<bool> s_strobe;
  sc_event_queue_port E_trigger_event_available;
  sc_signal<sc_uint<8> > chip_event_buffers_used;
  sc_signal<sc_uint<32> > chip_total_number_of_hits;

private:
  EventGenerator *mEvents;
  AlpideToyModel *mAlpide;
  const QSettings* mSettings;
  bool simulation_done = false;
  int mNumEvents;

  int mStrobeActiveClockCycles;
  int mStrobeInactiveClockCycles;
  
public:
  Stimuli(sc_core::sc_module_name name, QSettings* settings);
  void stimuliMainProcess(void);
  void stimuliEventProcess(void);  
  void addTraces(sc_trace_file *wf);

};





#endif
