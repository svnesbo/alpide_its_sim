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
#include <misc/vcd_trace.hpp>


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
  , s_alpide_control_output(n_ctrl_links)
  , s_alpide_data_input(n_data_links)
  , s_serial_data_input(n_data_links)
  , s_busy_in("busy_in")
  , s_busy_out("busy_out")
  , mLayerId(layer_id)
  , mStaveId(stave_id)
  , mReadoutUnitTriggerDelay(0)
  , mTriggerFilterTimeNs(trigger_filter_time)
  , mInnerBarrelMode(inner_barrel)
  , mTriggersSentCount(n_ctrl_links)
  , mTriggerActionMaps(n_ctrl_links)
  , mAlpideLinkBusySignals(n_data_links)
{
  // This prevents the first trigger from being filtered
  mLastTriggerTime = -mTriggerFilterTimeNs;

  mDataLinkParsers.resize(n_data_links);

  for(unsigned int i = 0; i < n_data_links; i++) {
    // Data parsers should not save events, that just eats memory.. :(
    mDataLinkParsers[i] = std::make_shared<AlpideDataParser>("", false);

    mDataLinkParsers[i]->s_clk_in(s_system_clk_in);
    mDataLinkParsers[i]->s_serial_data_in(s_serial_data_input[i]);
    mAlpideLinkBusySignals[i](mDataLinkParsers[i]->s_link_busy_out);

    s_alpide_data_input[i].register_put(
      std::bind(&ReadoutUnit::alpideDataSocketInput, this, std::placeholders::_1));
  }

  s_busy_out(s_busy_fifo_out);

  SC_METHOD(triggerInputMethod);
  sensitive << E_trigger_in;
  dont_initialize();

  SC_METHOD(evaluateBusyStatusMethod);
  for(unsigned int i = 0; i < mAlpideLinkBusySignals.size(); i++) {
    sensitive << mAlpideLinkBusySignals[i];
  }
  dont_initialize();
}


///@brief This guarantees that the fifo is created before it is used
///       (since it is created elsewhere, not in the ReadoutUnit class)
void ReadoutUnit::end_of_elaboration(void)
{
  SC_METHOD(busyChainMethod);
  sensitive << s_busy_in->data_written_event();
  dont_initialize();
}


///@brief Dummy callback function for the Alpide data socket. Not used here.
void ReadoutUnit::alpideDataSocketInput(const DataPayload &pl)
{
  // Do nothing
  //std::cout << "Received some Alpide data" << std::endl;
}


///@brief Send triggers to the Alpide using the control socket interface
///       Shamelessly stolen from alpideControl.cpp in Matthias Bonora's
///       SystemC simulations for the Readout Unit.
void ReadoutUnit::sendTrigger(void)
{
  ControlRequestPayload trigger_word;

  trigger_word.opcode = 0x55; // Trigger
  trigger_word.chipId = 0x00;
  trigger_word.address = 0x0000;

  // Tell the Alpide how much it should increase its trigger ID count with.
  // The trigger ID counts up without skipping any values in the RU,
  // but since not all triggers are distributed to the Alpide, to have a
  // synchronized trigger ID across we need to tell it how much to increase
  // the trigger ID with.
  // Since the data in this socket is only 16 bits, we can not send the
  // full trigger ID.
  trigger_word.data = mTriggerIdCount-mPreviousTriggerId;

  std::string msg = "Send Trigger at: " + sc_core::sc_time_stamp().to_string();
  SC_REPORT_INFO_VERB(name(),msg.c_str(),sc_core::SC_DEBUG);

  uint64_t time_now = sc_time_stamp().value();
  bool filter_trigger = (time_now - mLastTriggerTime) < mTriggerFilterTimeNs;

  for(unsigned int i = 0; i < s_alpide_control_output.size(); i++) {
    if(filter_trigger) {
      // Filter triggers that come too close in time
      mTriggerActionMaps[i][mTriggerIdCount] = TRIGGER_FILTERED;
      mTriggersFilteredCount++;
    } else if(true) { ///@todo else if(link_busy[i] == false) {
      // If we are not busy, send trigger
      s_alpide_control_output[i]->transport(trigger_word);
      mTriggersSentCount[i]++;
      mTriggerActionMaps[i][mTriggerIdCount] = TRIGGER_SENT;
      mPreviousTriggerId = mTriggerIdCount;
      mLastTriggerTime = time_now;
    } else {
      mTriggerActionMaps[i][mTriggerIdCount] = TRIGGER_NOT_SENT_BUSY;
      mLastTriggerTime = time_now;
    }
  }

  mTriggerIdCount++;
}


///@brief Process trigger input events.
void ReadoutUnit::triggerInputMethod(void)
{
  uint64_t time_now = sc_time_stamp().value();

  std::cout << "@" << time_now;
  std::cout << ": RU " << mLayerId << ":" << mStaveId << " triggered.";
  std::cout << std::endl;

  sendTrigger();
}


///@brief SystemC method, sensitive to changes on any of the busy signals
///       from the AlpideDataParsers. Counts number of busy links to evaluate
///       local busy status.
void ReadoutUnit::evaluateBusyStatusMethod(void)
{
  unsigned int busy_link_count = 0;

  for(unsigned int i = 0; i < mAlpideLinkBusySignals.size(); i++) {
    if(mAlpideLinkBusySignals[i]->read())
      busy_link_count++;
  }

  if(busy_link_count != mBusyLinkCount) {
    mBusyLinkCount = busy_link_count;

    if(mBusyLinkCount > mBusyLinkThreshold)
      mLocalBusyStatus = true;
    else
      mLocalBusyStatus = false;

    uint64_t time_now = sc_time_stamp().value();

    // std::shared_ptr<BusyLinkWord> busy_word
    //   = std::make_shared<BusyCountUpdate>(mId,
    //                                       time_now,
    //                                       mBusyLinkCount,
    //                                       mLocalBusyStatus);

    BusyLinkWord busy_word = BusyCountUpdate(mId,
                                             time_now,
                                             mBusyLinkCount,
                                             mLocalBusyStatus);

    s_busy_fifo_out.nb_write(busy_word);
  }
}


///@brief SystemC method, sensitive to busy chain input event. Sends busy updates further down
///       the chain, unless they originated from this readout unit.
void ReadoutUnit::busyChainMethod(void)
{
  ///@todo Pass on busy event here, unless the busy event originated from this readout unit.
  ///@todo How can I implement a payload with these events? Maybe Matthias' structure is suitable for this?

  BusyLinkWord busy_word;

  if(s_busy_in->nb_read(busy_word)) {

    uint64_t time_now = sc_time_stamp().value();
    std::cout << "@" << time_now << ": RU " << mStaveId << ":" << mLayerId << " Got busy word." << std::endl;
    std::cout << "Origin ID: " << busy_word.mOriginAddress << std::endl;
    std::cout << "Timestamp : " << busy_word.mTimeStamp << std::endl;
    std::cout << "Busy word type: " << busy_word.getString();

    // Ignore (and discard) busy words that originated from this readout unit
    // (ie. it has made the roundtrip through the busy chain)
    if(busy_word.mOriginAddress != mId) {
      ///@todo Finish this..

      // std::shared_ptr<BusyCountUpdate> busy_count_word_ptr;
      // std::shared_ptr<BusyGlobalStatusUpdate> busy_global_word_ptr;

      // busy_count_word_ptr = std::dynamic_pointer_cast<BusyCountUpdate>(busy_word);
      // busy_global_word_ptr = std::dynamic_pointer_cast<BusyGlobalStatusUpdate>(busy_word);

      // // Find out what kind of busy word we're dealing with, and process it
      // if(busy_count_word_ptr) {
      //   ///@todo What should we do with busy count updates from other RU's?
      //   /// Do something smart here..
      // } else if (busy_global_word_ptr) {
      //   mGlobalBusyStatus = busy_global_word_ptr->mGlobalBusyStatus;
      // }

      // Pass the busy word down the daisy chain link
      std::cout << "Passing on busy word down the chain.." << std::endl;
      s_busy_fifo_out.nb_write(busy_word);
    }
  }
}


///@brief Add SystemC signals to log in VCD trace file.
///@param[in,out] wf Pointer to VCD trace file object
///@param[in] name_prefix Name prefix to be added to all the trace names
void ReadoutUnit::addTraces(sc_trace_file *wf, std::string name_prefix) const
{
  std::stringstream ss;
  ss << name_prefix << "RU_" << mLayerId << "_" << mStaveId << ".";
  std::string RU_name_prefix = ss.str();

  ///@todo Add a trigger out to RU that we can trace here..
  //addTrace(wf, RU_name_prefix, "trigger_out", s_trigger_out);
}


///@brief Write simulation stats/data to file
///@param[in] output_path Path to simulation output directory
void ReadoutUnit::writeSimulationStats(const std::string output_path) const
{
  std::string csv_filename = output_path + std::string("_Link_utilization.csv");
  ofstream prot_stats_csv_file(csv_filename);

  // Write file with Alpide data protocol word utilization
  // -----------------------------------------------------
  if(!prot_stats_csv_file.is_open()) {
    std::cerr << "Error opening link utilization stats file: " << csv_filename << std::endl;
    return;
  } else {
    std::cout << "Writing link utilization stats to file:\n\"";
    std::cout << csv_filename << "\"" << std::endl;
  }

  prot_stats_csv_file << "Link ID;";
  prot_stats_csv_file << "COMMA (bytes);";
  prot_stats_csv_file << "IDLE_TOTAL (bytes);";
  prot_stats_csv_file << "IDLE_PURE (bytes);";
  prot_stats_csv_file << "IDLE_FILLER (bytes);";
  prot_stats_csv_file << "BUSY_ON (bytes);";
  prot_stats_csv_file << "BUSY_OFF (bytes);";
  prot_stats_csv_file << "DATA_SHORT (bytes);";
  prot_stats_csv_file << "DATA_LONG (bytes);";
  prot_stats_csv_file << "REGION_HEADER (bytes);";
  prot_stats_csv_file << "REGION_TRAILER (bytes);";
  prot_stats_csv_file << "CHIP_HEADER (bytes);";
  prot_stats_csv_file << "CHIP_TRAILER (bytes);";
  prot_stats_csv_file << "CHIP_EMPTY_FRAME (bytes);";
  prot_stats_csv_file << "UNKNOWN (bytes);";
  prot_stats_csv_file << "IDLE_TOTAL (count);";
  prot_stats_csv_file << "IDLE_PURE (count);";
  prot_stats_csv_file << "IDLE_FILLER (count);";
  prot_stats_csv_file << "BUSY_ON (count);";
  prot_stats_csv_file << "BUSY_OFF (count);";
  prot_stats_csv_file << "DATA_SHORT (count);";
  prot_stats_csv_file << "DATA_LONG (count);";
  prot_stats_csv_file << "REGION_HEADER (count);";
  prot_stats_csv_file << "REGION_TRAILER (count);";
  prot_stats_csv_file << "CHIP_HEADER (count);";
  prot_stats_csv_file << "CHIP_TRAILER (count);";
  prot_stats_csv_file << "CHIP_EMPTY_FRAME (count);";
  prot_stats_csv_file << std::endl;

  for(unsigned int i = 0; i < mDataLinkParsers.size(); i++) {
    auto stats = mDataLinkParsers[i]->getProtocolStats();

    // Calculate total number of bytes used by data words,
    // and counts of each type of data word
    uint64_t comma_bytes = stats[ALPIDE_COMMA];

    uint64_t idle_total_bytes = stats[ALPIDE_IDLE];
    uint64_t idle_total_count = idle_total_bytes;

    uint64_t chip_header_bytes = stats[ALPIDE_CHIP_HEADER1] +
                                 stats[ALPIDE_CHIP_HEADER2];
    uint64_t chip_header_count = chip_header_bytes/2;

    uint64_t chip_empty_frame_bytes = stats[ALPIDE_CHIP_EMPTY_FRAME1] +
                                      stats[ALPIDE_CHIP_EMPTY_FRAME2];
    uint64_t chip_empty_frame_count = chip_empty_frame_bytes/2;

    uint64_t data_short_bytes = stats[ALPIDE_DATA_SHORT1] +
                                stats[ALPIDE_DATA_SHORT2];
    uint64_t data_short_count = data_short_bytes/2;

    uint64_t data_long_bytes = stats[ALPIDE_DATA_LONG1] +
                               stats[ALPIDE_DATA_LONG2] +
                               stats[ALPIDE_DATA_LONG3];
    uint64_t data_long_count = data_long_bytes/3;

    uint64_t chip_trailer_bytes = stats[ALPIDE_CHIP_TRAILER];
    uint64_t chip_trailer_count = chip_trailer_bytes;

    uint64_t region_header_bytes = stats[ALPIDE_REGION_HEADER];
    uint64_t region_header_count = region_header_bytes;

    uint64_t region_trailer_bytes = stats[ALPIDE_REGION_TRAILER];
    uint64_t region_trailer_count = region_trailer_bytes;

    uint64_t busy_on_bytes = stats[ALPIDE_BUSY_ON];
    uint64_t busy_on_count = busy_on_bytes;

    uint64_t busy_off_bytes = stats[ALPIDE_BUSY_OFF];
    uint64_t busy_off_count = busy_off_bytes;

    uint64_t unknown_bytes = stats[ALPIDE_UNKNOWN];


    // Calculate number of IDLE "filler" bytes, ie. the IDLE words
    // that fill empty gaps in other data words
    uint64_t idle_filler_bytes = chip_header_bytes;
    idle_filler_bytes += 2*chip_trailer_bytes;
    idle_filler_bytes += chip_empty_frame_bytes;
    idle_filler_bytes += 2*region_header_bytes;

    // Region trailer is triplicated
    idle_filler_bytes += 0*region_trailer_bytes;

    idle_filler_bytes += data_short_bytes;
    idle_filler_bytes += 2*busy_on_bytes;
    idle_filler_bytes += 2*busy_off_bytes;

    uint64_t idle_filler_count = idle_filler_bytes;

    uint64_t idle_pure_bytes = idle_total_bytes-idle_filler_bytes;
    uint64_t idle_pure_count = idle_pure_bytes;

    prot_stats_csv_file << i << ";";

    prot_stats_csv_file << comma_bytes << ";";
    prot_stats_csv_file << idle_total_bytes << ";";
    prot_stats_csv_file << idle_pure_bytes << ";";
    prot_stats_csv_file << idle_filler_bytes << ";";
    prot_stats_csv_file << busy_on_bytes << ";";
    prot_stats_csv_file << busy_off_bytes << ";";
    prot_stats_csv_file << data_short_bytes << ";";
    prot_stats_csv_file << data_long_bytes << ";";
    prot_stats_csv_file << region_header_bytes << ";";
    prot_stats_csv_file << region_trailer_bytes << ";";
    prot_stats_csv_file << chip_header_bytes << ";";
    prot_stats_csv_file << chip_trailer_bytes << ";";
    prot_stats_csv_file << chip_empty_frame_bytes << ";";
    prot_stats_csv_file << unknown_bytes << ";";

    prot_stats_csv_file << idle_total_count << ";";
    prot_stats_csv_file << idle_pure_count << ";";
    prot_stats_csv_file << idle_filler_count << ";";
    prot_stats_csv_file << busy_on_count << ";";
    prot_stats_csv_file << busy_off_count << ";";
    prot_stats_csv_file << data_short_count << ";";
    prot_stats_csv_file << data_long_count << ";";
    prot_stats_csv_file << region_header_count << ";";
    prot_stats_csv_file << region_trailer_count << ";";
    prot_stats_csv_file << chip_header_count << ";";
    prot_stats_csv_file << chip_trailer_count << ";";
    prot_stats_csv_file << chip_empty_frame_count << std::endl;
  }
  prot_stats_csv_file.close();


  // Write binary data file with trigger stats
  // -----------------------------------------------------
  std::string trig_stats_filename = output_path + std::string("_Trigger_stats.dat");
  ofstream trig_stats_file(trig_stats_filename, std::ios_base::out | std::ios_base::binary);

  if(!trig_stats_file.is_open()) {
    std::cerr << "Error opening trigger stats file: " << trig_stats_filename << std::endl;
    return;
  } else {
    std::cout << "Writing trigger stats to file:\n\"";
    std::cout << trig_stats_filename << "\"" << std::endl;
  }


  // Write number of triggers and number of links to file header
  uint64_t num_triggers = mTriggerIdCount;
  uint8_t num_links = s_alpide_control_output.size();
  trig_stats_file.write((char*)&num_triggers, sizeof(uint64_t));
  trig_stats_file.write((char*)&num_links, sizeof(uint8_t));

  // Write action for each link, for each trigger
  for(uint64_t trigger_id = 0; trigger_id < mTriggerIdCount; trigger_id++) {
    for(uint64_t link_id = 0; link_id < mTriggerActionMaps.size(); link_id++) {
      uint8_t link_action = mTriggerActionMaps[link_id].at(trigger_id);
      trig_stats_file.write((char*)&link_action, sizeof(uint8_t));
    }
  }
  trig_stats_file.close();


  // Write file with trigger summary
  // -----------------------------------------------------
  csv_filename = output_path + std::string("_Trigger_summary.csv");
  ofstream trigger_summary_csv_file(csv_filename);

  if(!trigger_summary_csv_file.is_open()) {
    std::cerr << "Error opening trigger summary file: " << csv_filename << std::endl;
    return;
  } else {
    std::cout << "Writing trigger summary to file:\n\"";
    std::cout << csv_filename << "\"" << std::endl;
  }

  trigger_summary_csv_file << "Triggers received; Triggers filtered";
  for(unsigned int i = 0; i < mTriggersSentCount.size(); i++) {
    trigger_summary_csv_file << "; Link " << i << " triggers sent";
  }
  trigger_summary_csv_file << std::endl;

  // Trigger id count is equivalent to number of triggers received
  trigger_summary_csv_file << mTriggerIdCount << "; ";
  trigger_summary_csv_file << mTriggersFilteredCount << "; ";

  for(unsigned int i = 0; i < mTriggersSentCount.size(); i++) {
    trigger_summary_csv_file << mTriggersSentCount[i] << "; ";
  }
  trigger_summary_csv_file << std::endl;
  trigger_summary_csv_file.close();

  ///@todo More ITS/RU stats here..
}
