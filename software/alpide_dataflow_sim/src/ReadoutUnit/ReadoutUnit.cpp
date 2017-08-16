/**
 * @file   ReadoutUnit.cpp
 * @author Simon Voigt Nesbo
 * @date   June 13, 2017
 * @brief  Mockup version of readout unit.
 *         Accepts trigger input from the dummy CTP module, and communicates the trigger
 *         to the Alpide objects. It also accepts data from the alpides, and decodes the
 *         data stream to detect busy situations in the alpides.
 *
 */


#include "ReadoutUnit.hpp"


///@todo Implementation
///      Use AlpideDataParser? Implement quick parsing of busy words, and generation of busy map.

//Do I need SC_HAS_PROCESS if I only use SC_METHOD??
SC_HAS_PROCESS(ReadoutUnit);
///@brief Constructor for ReadoutUnit
///@param name SystemC module name
///@param layer_id ID of layer that this RU belongs to
///@param stave_id ID of the stave in the layer that this RU is connected to
///@param n_ctrl_links Number of Alpide control links connected to this readout unit
///@param n_data_links Number of Alpide data links connected to this readout unit
///@param trigger_filter_time The Readout Unit will filter out triggers more closely
///                           spaced than this time (specified in nano seconds)
///@param inner_barrel Set to true if RU is connected to inner barrel stave
ReadoutUnit::ReadoutUnit(sc_core::sc_module_name name,
                         unsigned int layer_id,
                         unsigned int stave_id,
                         unsigned int n_ctrl_links,
                         unsigned int n_data_links,
                         unsigned int trigger_filter_time,
                         bool inner_barrel)
  : sc_core::sc_module(name)
  , mLayerId(layer_id)
  , mStaveId(stave_id)
  , s_alpide_control_output(n_ctrl_links)
  , s_alpide_data_input(n_data_links)
  , s_serial_data_input(n_data_links)
  , mInnerBarrelMode(inner_barrel)
  , mReadoutUnitTriggerDelay(0)
  , mTriggerFilterTimeNs(trigger_filter_time)
{
  mDataLinkParsers.resize(links_in_use);
  mAlpideLinkBusySignals.resize(links_in_use);

  for(int i = 0; i < links_in_use; i++) {
    mDataLinkParsers[i] = std::make_shared<AlpideDataParser>("", false);
    mDataLinkParsers[i]->s_clk_in(s_system_clk_in);
    mDataLinkParsers[i]->s_serial_data_in(s_serial_data_input[i]);
    mDataLinkParsers[i]->s_link_busy_out(mAlpideLinkBusySignals[i]);
  }

  SC_METHOD(triggerInputMethod);
  sensitive << E_trigger_in;

  SC_METHOD(busyChainMethod);
  sensitive << s_busy_fifo_in.data_written_event();

  SC_METHOD(evaluateBusyStatusMethod);
  for(int i = 0; i < mDataLinkBusySignals.size(); i++) {
    sensitive << mAlpideLinkBusySignals[i].
  }
}


///@brief This guarantees that the fifo is created before it is used
///       (since it is created elsewhere, not in the ReadoutUnit class)
void ReadoutUnit::end_of_elaboration(void)
{
  SC_METHOD(busyChainMethod);
  sensitive_pos << s_busy_fifo_in.data_written_event();
}


///@brief Send triggers to the Alpide using the control socket interface
///       Shamelessly stolen from alpideControl.cpp in Matthias Bonora's
///       SystemC simulations for the Readout Unit.
void ReadoutUnit::sendTrigger(void)
{
  uint8_t opcode = 0x55; // Trigger

  std::string msg = "Send Trigger at: " + sc_core::sc_time_stamp().to_string();
  SC_REPORT_INFO_VERB(name(),msg.c_str(),sc_core::SC_DEBUG);
  s_alpide_control_output->transport({ opcode });
}


///@brief Process trigger input events.
void ReadoutUnit::triggerInputMethod(void)
{
  sc_time time_now = int64_t time_now = sc_time_stamp().value();

  // Filter triggers that come too close in time
  if((time_now - mLastTriggerTime) >= mTriggerFilterTime) {

    ///@todo If detector is busy.. hold back on triggers here...

    mLastTriggerTime = time_now;
    sendTrigger();
  }
}


///@brief SystemC method, sensitive to changes on any of the busy signals
///       from the AlpideDataParsers. Counts number of busy links to evaluate
///       local busy status.
void ReadoutUnit::evaluateBusyStatusMethod(void)
{
  unsigned int busy_link_count = 0;

  for(int i = 0; i < mAlpideLinkBusySignals.size(); i++) {
    if(mAlpideLinkBusySignals[i].read())
      busy_link_count++;
  }

  if(busy_link_count != mBusyLinkCount) {
    mBusyLinkCount = busy_link_count;

    if(mBusyLinkCount > mBusyLinkThreshold)
      mLocalBusyStatus = true;
    else
      mLocalBusyStatus = false;

    sc_time time_now = sc_time_stamp().value();
    std::shared_ptr<BusyLinkWord> busy_word = new(BusyCountUpdate(mID,
                                                                  time_now,
                                                                  mBusyLinkCount,
                                                                  mLocalBusyStatus));

    s_busy_fifo_out->nb_write(busy_word);
  }
}


///@brief SystemC method, sensitive to busy chain input event. Sends busy updates further down
///       the chain, unless they originated from this readout unit.
void ReadoutUnit::busyChainMethod(void)
{
  ///@todo Pass on busy event here, unless the busy event originated from this readout unit.
  ///@todo How can I implement a payload with these events? Maybe Matthias' structure is suitable for this?

  std::shared_ptr<BusyLinkWord> busy_word;
  s_busy_fifo_in->nb_read(busy_word);

  if(busy_word) {

    // Ignore (and discard) busy words that originated from this readout unit
    // (ie. it has made the roundtrip through the busy chain)
    if(busy_word->mOriginAddress != mID) {
      std::shared_ptr<BusyCountUpdate> busy_count_word_ptr;
      std::shared_ptr<BusyGlobalStatusUpdate> busy_global_word_ptr;

      busy_count_word = std::dynamic_pointer_cast<BusyCountUpdate>(busy_word);
      busy_global_word = std::dynamic_pointer_cast<BusyGlobalStatusUpdate>(busy_word);

      // Find out what kind of busy word we're dealing with, and process it
      if(busy_count_word) {
        ///@todo What should we do with busy count updates from other RU's?
        /// Do something smart here..
      } else if (busy_global_word) {
        mGlobalBusyStatus = busy_global_word->mGlobalBusyStatus;
      }
    }

    // Pass the busy word down the daisy chain link
    s_busy_fifo_out->nb_write(busy_word);
  }
}
