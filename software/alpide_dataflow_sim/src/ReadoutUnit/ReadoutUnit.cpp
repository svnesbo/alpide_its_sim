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
  , mReadoutUnitTriggerDelay(0)
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


///@brief Process trigger input events.
void ReadoutUnit::triggerInputMethod(void)
{
  sc_time time_now = int64_t time_now = sc_time_stamp().value();

  // Filter triggers that come too close in time
  if((time_now - mLastTriggerTime) >= mTriggerFilterTime) {

    ///@todo If detector is busy.. hold back on triggers here...

    mLastTriggerTime = time_now;
    E_trigger_filtered_out.notify(mReadoutUnitTriggerDelay, SC_NS);
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
