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


SC_MODULE(Stimuli) {
  sc_in_clk clock;
  sc_signal<sc_uint<8> > chip_event_buffers_used;
  sc_signal<sc_uint<32> > chip_total_number_of_hits;

private:
  EventGenerator mEvents;
  AlpideToyModel mAlpide;
  bool simulation_done = false;  

public:
  SC_CTOR(Stimuli)
    : mEvents()
    , mAlpide("alpide", 0)
  {
    // Connect signals to Alpide
    mAlpide.s_clk_in(clock);
    mAlpide.s_event_buffers_used(chip_event_buffers_used);
    mAlpide.s_total_number_of_hits(chip_total_number_of_hits);
  
    SC_CTHREAD(stimuliProcess, clock.pos());

  
    //@todo Logging process?
    //SC_CTHREAD(logging_process, clock_40MHz.pos());  
  }

  void stimuliProcess(void);

};





#endif
