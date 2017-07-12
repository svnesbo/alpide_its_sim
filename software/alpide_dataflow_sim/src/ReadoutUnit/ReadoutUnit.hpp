/**
 * @file   ReadoutUnit.hpp
 * @author Simon Voigt Nesbo
 * @date   June 13, 2017
 * @brief  Mockup version of readout unit.
 *         Accepts trigger input from the dummy CTP module, and communicates the trigger
 *         to the Alpide objects. It also accepts data from the alpides, and decodes the
 *         data stream to detect busy situations in the alpides.
 *
 */

#ifndef READOUT_UNIT_HPP
#define READOUT_UNIT_HPP


#include "BusyLinkWord.hpp"
#include <Alpide/AlpideInterface.hpp>


#define NUM_ALPIDE_DATA_LINKS 28


ReadoutUnit : public sc_core::sc_module {
public:
  sc_in_clk s_system_clk_in;

  ControlTargetSocket s_alpide_control_output;
  std::array<DataInitiatorSocket, NUM_ALPIDE_DATA_LINKS> s_alpide_data_input;

  /*
    The stimuli (or stave?) class will have to instantiate the FIFOs between the RUs:

    sc_fifo<BusyLinkWord> s_busy_fifos[N];

    And connect the RUs together:

    RU[0]->s_busy_fifo_out(s_busy_fifos[0]);
    RU[1]->s_busy_fifo_in(s_busy_fifos[0]);

    And so on..
  */

  sc_port<sc_fifo_out_if<BusyLinkWord>> s_busy_fifo_in;
  sc_port<sc_fifo_in_if<BusyLinkWord>> s_busy_fifo_out;

  sc_event E_trigger_in;
  sc_event E_trigger_filtered_out;

  ///@todo Make this a vector/array somehow, to cater for many chips..
  sc_in<sc_uint<24>> s_serial_data_input[NUM_ALPIDE_DATA_LINKS];

private:
  unsigned int mID;
  sc_time mLastTriggerTime;

  void triggerInputMethod(void);
  void mainMethod(void);
  //  void dataInputMethod(void);
  //  void busyChainMethod(void);

  void processInputData();
  void evaluateBusyStatus();

public:
  ReadoutUnit(sc_core::sc_module_name name, unsigned int id);
  ~ReadoutUnit();
  void end_of_elaboration();
};


#endif
