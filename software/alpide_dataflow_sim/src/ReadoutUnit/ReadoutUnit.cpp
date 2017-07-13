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
///@param id ID of ReadoutUnit (same as stave ID, numbered continuously from
///       inner to outer barrels)
///@param links_in_use Number of Alpide data links in connected for this readout unit
///@param inner_barrel Set to true if RU is connected to inner barrel stave
ReadoutUnit::ReadoutUnit(sc_core::sc_module_name name, unsigned int id,
                         unsigned int links_in_use, bool inner_barrel)
  : sc_core::sc_module(name)
  , mID(id)
  , mActiveLinks(links_in_use)
  , mInnerBarrelMode(inner_barrel)
{
  mDataLinkParsers.resize(links_in_use);

  for(int i = 0; i < links_in_use; i++) {
    mDataLinkParsers[i] = std::make_shared<AlpideDataParser>("", false);
    mDataLinkParsers[i]->s_clk_in(s_system_clk_in);
    mDataLinkParsers[i]->s_serial_data_in(s_serial_data_input[i]);
    mDataLinkParsers[i]->s_link_busy_out(
      ///@todo Assign to a signal i ReadoutUnit here...
      );
  }


  SC_METHOD(triggerInputMethod);
  sensitive << E_trigger_in;

  SC_METHOD(mainMethod);
  sensitive_pos << s_system_clk_in;

  //SC_METHOD(dataInputMethod);
  //sensitive_pos << s_system_clk_in;
}


///@brief This guarantees that the fifo is created before it is used
///       (since it is created elsewhere, not in the ReadoutUnit class)
void ReadoutUnit::end_of_elaboration(void)
{
  SC_METHOD(busyChainMethod);
  sensitive_pos << s_busy_fifo_in.data_written_event();
}


///@brief Process trigger input events.
void ReadoutUnit::triggerInputMethod(void)
{
  sc_time time_now = int64_t time_now = sc_time_stamp().value();

  // Filter triggers that come too close in time
  if((time_now - mLastTriggerTime) >= mTriggerFilterTime) {

    ///@todo If detector is busy.. hold back on triggers here...

    mLastTriggerTime = time_now;
    E_trigger_filtered_out.notify(SC_ZERO_TIME);

    ///@todo Add a delay here corresponding to delay for sending/decoding triggers
    ///      on Alpide slow control link?
    //E_trigger_filtered_out.notify(mAlpideTriggerDelay, SC_NS);

    ///@todo Add some logic here (and delay?) so that we only request the event frames
    ///      once the Alpide chips are ready to accept a new event frame, and only
    ///      for the chips that are actually ready.
    E_request_event_frame.notify()
  }
}


///@brief SystemC method, sensitive to changes on any of the busy signals
///       from the AlpideDataParsers. Counts number of busy links to evaluate
///       local busy status.
void ReadoutUnit::evaluateBusyStatus(void)
{
  unsigned int busy_link_count = 0;

  for(int i = 0; i < mDataLinkParsers.size(); i++) {
    if(mDataLinkParsers[i]->s_link_busy_out.read())
      busy_link_count++;
  }

  if(busy_link_count != mBusyLinkCount) {
    mBusyLinkCount = busy_link_count;

    if(mBusyLinkCount > mBusyLinkThreshold)
      mLocalBusyStatus = true;
    else
      mLocalBusyStatus = false;

    ///@todo Do something here to send out a new package on the busy daisy chain
  }
}


///@brief SystemC method, sensitive to busy chain input event. Sends busy updates further down
///       the chain, unless they originated from this readout unit.
void ReadoutUnit::busyChainMethod(void)
{
  ///@todo Pass on busy event here, unless the busy event originated from this readout unit.
  ///@todo How can I implement a payload with these events? Maybe Matthias' structure is suitable for this?
  E_busy_chain_queue_out->notify(SC_ZERO_TIME);
}











/*





///@brief Main SystemC method for Readout Unit, sensitive to system clock (40MHz).
void ReadoutUnit::mainMethod(void)
{
  processInputData();
  evaluateBusyStatus();

  ///@todo Implement data parser here..

  // 1. Parse data
  // 2. Check for busy Alpide chips
  // 3. Update local busy map of this RU's Alpide chips
  // 4. Send out updated busy status/map if it changed
}


///@brief SystemC method, sensitive to system clock (40MHz).
///       Continuously parses data from all Alpide chips, looks for changes in busy
///       status, updates local busy map, and notifies other readout units of changes
///       to the busy status.
void ReadoutUnit::dataInputMethod(void)
{
  ///@todo Implement data parser here..

  // 1. Parse data
  // 2. Check for busy Alpide chips
  // 3. Update local busy map of this RU's Alpide chips
  // 4. Send out updated busy status/map if it changed
}


*/
