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

const uint8_t TRIGGER_SENT = 0;
const uint8_t TRIGGER_NOT_SENT_BUSY = 1;
const uint8_t TRIGGER_FILTERED = 2;


class ReadoutUnit : public sc_core::sc_module {
public:
  sc_in_clk s_system_clk_in;

  std::vector<ControlInitiatorSocket> s_alpide_control_output;
  std::vector<DataTargetSocket> s_alpide_data_input;

  sc_event_queue E_trigger_in;

  ///@todo Make this a vector/array somehow, to cater for many chips..
  std::vector<sc_in<AlpideDataWord>> s_serial_data_input;

  // Busy in and out signals for busy daisy chain
  sc_port<sc_fifo_in_if<BusyLinkWord>> s_busy_in;
  sc_export<sc_fifo<BusyLinkWord>> s_busy_out;

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
  uint64_t mTriggerIdCount = 0;
  uint64_t mPreviousTriggerId = 0;
  uint64_t mTriggersFilteredCount = 0;

  // One entry per control link.
  // Should be same size as s_alpide_control_output.
  std::vector<uint64_t> mTriggersSentCount;

  // Map holds the trigger action taken per event ID per control link
  // One map per control link in the vector
  // Valid values for the uint8_t:
  // TRIGGER_SENT, TRIGGER_NOT_SENT_BUSY, TRIGGER_FILTERED.
  std::vector<std::map<uint64_t, uint8_t>> mTriggerActionMaps;


  std::vector<std::shared_ptr<AlpideDataParser>> mDataLinkParsers;
  std::vector<sc_export<sc_signal<bool>>> mAlpideLinkBusySignals;

  void sendTrigger(void);
  void alpideDataSocketInput(const DataPayload &pl);

  void evaluateBusyStatusMethod(void);
  void triggerInputMethod(void);
  void busyChainMethod(void);
//  void processInputData(void);

public:
  ReadoutUnit(sc_core::sc_module_name name,
              unsigned int layer_id,
              unsigned int stave_id,
              unsigned int n_ctrl_links,
              unsigned int n_data_links,
              unsigned int trigger_filter_time,
              bool inner_barrel);
  void end_of_elaboration();
  void addTraces(sc_trace_file *wf, std::string name_prefix) const;
  void writeSimulationStats(const std::string output_path) const;
};



#endif
