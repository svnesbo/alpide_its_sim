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

// Ignore warnings about use of auto_ptr in SystemC library
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#include <systemc.h>
#pragma GCC diagnostic pop

#include <vector>
#include <memory>

#include "BusyLinkWord.hpp"
#include <Alpide/AlpideInterface.hpp>
#include "../AlpideDataParser/AlpideDataParser.hpp"


#define NUM_ALPIDE_DATA_LINKS 28


class ReadoutUnit : public sc_core::sc_module {
public:
  sc_in_clk s_system_clk_in;

  std::vector<ControlInitiatorSocket> s_alpide_control_output;
  std::vector<DataTargetSocket> s_alpide_data_input;

  /*
    The stimuli (or stave?) class will have to instantiate the FIFOs between the RUs:

    sc_fifo<BusyLinkWord> s_busy_fifos[N];

    And connect the RUs together:

    RU[0]->s_busy_fifo_out(s_busy_fifos[0]);
    RU[1]->s_busy_fifo_in(s_busy_fifos[0]);

    And so on..
  */


  sc_port<sc_fifo_in_if<BusyLinkWord>> s_busy_in;
  sc_export<sc_fifo<BusyLinkWord>> s_busy_out;

  sc_event E_trigger_in;

  ///@todo Make this a vector/array somehow, to cater for many chips..
  std::vector<sc_in<sc_uint<24>>> s_serial_data_input;

private:
  sc_fifo<BusyLinkWord> s_busy_fifo_out;

  unsigned int mId = 0;
  unsigned int mLayerId;
  unsigned int mStaveId;
  unsigned int mActiveLinks;
  unsigned int mBusyLinkCount;
  unsigned int mBusyLinkThreshold;
  unsigned int mReadoutUnitTriggerDelay;
  unsigned int mTriggerFilterTimeNs;
  bool mLocalBusyStatus;
  bool mGlobalBusyStatus;
  bool mInnerBarrelMode;
  bool mBusyDaisyChainMaster;
  uint64_t mLastTriggerTime;

  std::vector<std::shared_ptr<AlpideDataParser>> mDataLinkParsers;
  std::vector<sc_export<sc_signal<bool>>> mAlpideLinkBusySignals;

  void sendTrigger(void);

  void evaluateBusyStatusMethod(void);
  void triggerInputMethod(void);
  void busyChainMethod(void);
  void processInputData(void);

public:
  ReadoutUnit(sc_core::sc_module_name name,
              unsigned int layer_id,
              unsigned int stave_id,
              unsigned int n_ctrl_links,
              unsigned int n_data_links,
              unsigned int trigger_filter_time,
              bool inner_barrel);
  void end_of_elaboration();
};


#endif
