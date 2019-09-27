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
///@param trigger_filter_enable Enable trigger filtering
///@param inner_barrel Set to true if RU is connected to inner barrel stave
///@param data_rate_interval_ns Interval in nanoseconds over which number of data bytes should
///                             be counted, to be used for data rate calculations
ReadoutUnit::ReadoutUnit(sc_core::sc_module_name name,
                         unsigned int layer_id,
                         unsigned int stave_id,
                         unsigned int n_ctrl_links,
                         unsigned int n_data_links,
                         unsigned int trigger_filter_time,
                         bool trigger_filter_enable,
                         bool inner_barrel,
                         unsigned int data_rate_interval_ns)
  : sc_core::sc_module(name)
  , s_system_clk_in("system_clk_in")
  , s_alpide_control_output(n_ctrl_links)
  , s_alpide_data_input(n_data_links)
  , s_serial_data_input(n_data_links)
  , s_serial_data_trig_id(n_data_links)
  , s_busy_in("busy_in")
  , s_busy_out("busy_out")
  , mLayerId(layer_id)
  , mStaveId(stave_id)
  , mReadoutUnitTriggerDelay(0)
  , mTriggerFilterTimeNs(trigger_filter_time)
  , mTriggerFilterEnabled(trigger_filter_enable)
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
    mDataLinkParsers[i] = std::make_shared<AlpideDataParser>("",
                                                             inner_barrel,
                                                             data_rate_interval_ns,
                                                             false);

    mDataLinkParsers[i]->s_clk_in(s_system_clk_in);
    mDataLinkParsers[i]->s_serial_data_in(s_serial_data_input[i]);
    mDataLinkParsers[i]->s_serial_data_trig_id(s_serial_data_trig_id[i]);
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

  // Update current trigger ID in the data parsers
  for(unsigned int i = 0; i < mDataLinkParsers.size(); i++)
    mDataLinkParsers[i]->setCurrentTriggerId(mTriggerIdCount);

  // Issue triggers on ALPIDE control links (unless trigger is being filtered)
  for(unsigned int i = 0; i < s_alpide_control_output.size(); i++) {
    if(mTriggerFilterEnabled && filter_trigger) {
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
    std::cout << "@" << time_now << ": RU " << mLayerId << ":" << mStaveId << " Got busy word." << std::endl;
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
  // ------------------------------------------------------
  // Write data rate CSV file
  // ------------------------------------------------------

  std::string data_rate_csv_filename = output_path + std::string("_Data_rate.csv");
  ofstream data_rate_csv_file(data_rate_csv_filename);

  if(!data_rate_csv_file.is_open()) {
    std::cerr << "Error opening data rate stats file: " << data_rate_csv_filename << std::endl;
    return;
  } else {
    std::cout << "Writing data rate stats to file:\n\"";
    std::cout << data_rate_csv_filename << "\"" << std::endl;
  }

  data_rate_csv_file << "Time (ns); RU total (Mbps)";

  for(unsigned int i = 0; i < mDataLinkParsers.size(); i++) {
    data_rate_csv_file << ";Link " << i << " (Mbps)";
  }

  uint64_t data_rate_interval_ns = mDataLinkParsers[0]->getDataIntervalNs();

  // Assuming that each link parser has the recorded the same number of intervals, which should
  // hold true since they starts and stop at the same time, and use the same interval length
  for(auto interval_it = mDataLinkParsers[0]->getDataIntervalByteCounts().begin();
      interval_it != mDataLinkParsers[0]->getDataIntervalByteCounts().end();
      interval_it++)
  {
    data_rate_csv_file << std::endl;

    uint64_t interval_num = interval_it->first;
    uint64_t data_bytes_total = 0;

    data_rate_csv_file << interval_num*data_rate_interval_ns << ";";

    // Calculate total data rate (for readout unit)
    for(unsigned int i = 0; i < mDataLinkParsers.size(); i++) {
      data_bytes_total += mDataLinkParsers[i]->getDataIntervalByteCounts()[interval_num];
    }

    // Convert number of bytes in interval to Mbps
    double data_rate_total_mbps = 8*(data_bytes_total*(1E9/data_rate_interval_ns))/(1E6);
    data_rate_csv_file << data_rate_total_mbps;

    // Output data rate for each link
    for(unsigned int i = 0; i < mDataLinkParsers.size(); i++) {
      uint64_t data_bytes_link = mDataLinkParsers[i]->getDataIntervalByteCounts()[interval_num];

      // Convert number of bytes in interval to Mbps
      double data_rate_link_mbps = 8*(data_bytes_link*(1E9/data_rate_interval_ns))/(1E6);
      data_rate_csv_file << ";" << data_rate_link_mbps;
    }
  }
  data_rate_csv_file.close();


  // ------------------------------------------------------
  // Write file with Alpide data protocol word utilization
  // ------------------------------------------------------
  std::string csv_filename = output_path + std::string("_Link_utilization.csv");
  ofstream prot_stats_csv_file(csv_filename);

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

    uint64_t chip_header_bytes = stats[ALPIDE_CHIP_HEADER];
    uint64_t chip_header_count = chip_header_bytes/2;

    uint64_t chip_empty_frame_bytes = stats[ALPIDE_CHIP_EMPTY_FRAME];
    uint64_t chip_empty_frame_count = chip_empty_frame_bytes/2;

    uint64_t data_short_bytes = stats[ALPIDE_DATA_SHORT];
    uint64_t data_short_count = data_short_bytes/2;

    uint64_t data_long_bytes = stats[ALPIDE_DATA_LONG];
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
    uint64_t idle_filler_bytes = chip_header_count;
    idle_filler_bytes += 2*chip_trailer_count;
    idle_filler_bytes += chip_empty_frame_count;
    idle_filler_bytes += 2*region_header_count;

    // Region trailer is triplicated
    idle_filler_bytes += 0*region_trailer_count;

    idle_filler_bytes += data_short_count;
    idle_filler_bytes += 2*busy_on_count;
    idle_filler_bytes += 2*busy_off_count;

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


  // -----------------------------------------------------
  // Write binary data file with trigger actions
  // -----------------------------------------------------
  // File format:
  //
  //   Header:
  //   uint64_t: number of triggers
  //   uint8_t:  number of control links
  //
  //   For each trigger ID:
  //     Control link 0:
  //       uint8_t: link action
  //     Control link 1:
  //       uint8_t: link action
  //     ...
  //     Control link n-1:
  //       uint8_t: link action

  std::string trig_actions_filename = output_path + std::string("_trigger_actions.dat");
  ofstream trig_actions_file(trig_actions_filename, std::ios_base::out | std::ios_base::binary);

  if(!trig_actions_file.is_open()) {
    std::cerr << "Error opening trigger actions file: " << trig_actions_filename << std::endl;
    return;
  } else {
    std::cout << "Writing trigger actions to file:\n\"";
    std::cout << trig_actions_filename << "\"" << std::endl;
  }


  // Write number of triggers and number of control links to file header
  uint64_t num_triggers = mTriggerIdCount;
  uint8_t num_ctrl_links = s_alpide_control_output.size();
  trig_actions_file.write((char*)&num_triggers, sizeof(uint64_t));
  trig_actions_file.write((char*)&num_ctrl_links, sizeof(uint8_t));

  // Write action for each link, for each trigger
  for(uint64_t trigger_id = 0; trigger_id < mTriggerIdCount; trigger_id++) {
    for(uint64_t link_id = 0; link_id < mTriggerActionMaps.size(); link_id++) {
      uint8_t link_action = mTriggerActionMaps[link_id].at(trigger_id);
      trig_actions_file.write((char*)&link_action, sizeof(uint8_t));
    }
  }
  trig_actions_file.close();


  // -----------------------------------------------------
  // Write binary data file with busy events
  // -----------------------------------------------------
  // File format:
  //
  //   Header:
  //   uint8_t:  number of data links
  //
  //   For each data link:
  //       Header:
  //          uint64_t: Number of "busy events"
  //       Data (for each busy event)
  //          uint64_t: time of BUSY_ON
  //          uint64_t: time of BUSY_OFF
  //          uint64_t: trigger ID when BUSY_ON occured
  //          uint64_t: trigger ID when BUSY_OFF occured

  std::string busy_events_filename = output_path + std::string("_busy_events.dat");
  ofstream busy_events_file(busy_events_filename, std::ios_base::out | std::ios_base::binary);

  if(!busy_events_file.is_open()) {
    std::cerr << "Error opening busy events file: " << busy_events_filename << std::endl;
    return;
  } else {
    std::cout << "Writing busy events to file:\n\"";
    std::cout << busy_events_filename << "\"" << std::endl;
  }


  // Write number of data links to file header
  uint8_t num_data_links = s_alpide_data_input.size();
  busy_events_file.write((char*)&num_data_links, sizeof(uint8_t));

  // Write busy events for each link
  for(uint64_t link_id = 0; link_id < mDataLinkParsers.size(); link_id++) {
    std::vector<BusyEvent> busy_events = mDataLinkParsers[link_id]->getBusyEvents();
    uint64_t num_busy_events = busy_events.size();
    busy_events_file.write((char*)&num_busy_events, sizeof(uint64_t));

    for(auto busy_event_it = busy_events.begin();
        busy_event_it != busy_events.end();
        busy_event_it++)
    {
      busy_events_file.write((char*)&(busy_event_it->mBusyOnTime), sizeof(uint64_t));
      busy_events_file.write((char*)&(busy_event_it->mBusyOffTime), sizeof(uint64_t));
      busy_events_file.write((char*)&(busy_event_it->mBusyOnTriggerId), sizeof(uint64_t));
      busy_events_file.write((char*)&(busy_event_it->mBusyOffTriggerId), sizeof(uint64_t));
    }
  }
  busy_events_file.close();


  // -----------------------------------------------------
  // Write binary data file with busy violation events
  // -----------------------------------------------------
  // File format:
  //
  //   Header:
  //   uint8_t:  number of data links
  //
  //   For each data link:
  //       uint8_t: Number of chips with data for this link
  //
  //       For each chip in data link which has data:
  //          Header:
  //             uint8_t:  Chip ID
  //             uint64_t: Number of "busy violation events"
  //          Data (for each busy violation event)
  //             uint64_t: trigger ID for busy violation

  std::string busyv_events_filename = output_path + std::string("_busyv_events.dat");
  ofstream busyv_events_file(busyv_events_filename, std::ios_base::out | std::ios_base::binary);

  if(!busyv_events_file.is_open()) {
    std::cerr << "Error opening busy violation events file: " << busyv_events_filename << std::endl;
    return;
  } else {
    std::cout << "Writing busy violation events to file:\n\"";
    std::cout << busyv_events_filename << "\"" << std::endl;
  }


  // Write number of data links to file header
  busyv_events_file.write((char*)&num_data_links, sizeof(uint8_t));

  // Write busy events for each link
  for(uint64_t link_id = 0; link_id < mDataLinkParsers.size(); link_id++) {
    std::map<unsigned int, std::vector<uint64_t>> busyv_events =
      mDataLinkParsers[link_id]->getBusyViolationTriggers();

    // Write number of chips that had busyv events for this link
    uint8_t num_chips_with_data = busyv_events.size();
    busyv_events_file.write((char*)&num_chips_with_data, sizeof(uint8_t));

    // Write busyv events per chip in this link
    for(auto busyv_chip_it = busyv_events.begin();
        busyv_chip_it != busyv_events.end();
        busyv_chip_it++)
    {
      uint8_t chip_id = busyv_chip_it->first;
      busyv_events_file.write((char*)&chip_id, sizeof(uint8_t));

      uint64_t num_busyv_events = busyv_chip_it->second.size();
      busyv_events_file.write((char*)&num_busyv_events, sizeof(uint64_t));

      for(auto busyv_event_it = busyv_chip_it->second.begin();
          busyv_event_it != busyv_chip_it->second.end();
          busyv_event_it++)
      {
        uint64_t busyv_trigger_id = *busyv_event_it;
        busyv_events_file.write((char*)&busyv_trigger_id, sizeof(uint64_t));
      }
    }
  }
  busyv_events_file.close();


  // ------------------------------------------------------
  // Write binary data file with flushed incomplete events
  // ------------------------------------------------------
  // File format (same as for busyv basically):
  //
  //   Header:
  //   uint8_t:  number of data links
  //
  //   For each data link:
  //       uint8_t: Number of chips with data for this link
  //
  //       For each chip in data link which has data:
  //          Header:
  //             uint8_t:  Chip ID
  //             uint64_t: Number of "flushed incomplete events"
  //          Data (for each flushed incomplete event)
  //             uint64_t: trigger ID for flushed incomplete

  std::string flush_events_filename = output_path + std::string("_flush_events.dat");
  ofstream flush_events_file(flush_events_filename, std::ios_base::out | std::ios_base::binary);

  if(!flush_events_file.is_open()) {
    std::cerr << "Error opening flushed incomplete events file: ";
    std::cerr << flush_events_filename << std::endl;
    return;
  } else {
    std::cout << "Writing flushed incomplete events to file:\n\"";
    std::cout << flush_events_filename << "\"" << std::endl;
  }


  // Write number of data links to file header
  flush_events_file.write((char*)&num_data_links, sizeof(uint8_t));

  // Write flushed incomplete events for each link
  for(uint64_t link_id = 0; link_id < mDataLinkParsers.size(); link_id++) {
    std::map<unsigned int, std::vector<uint64_t>> flush_events
      = mDataLinkParsers[link_id]->getFlushedIncomplTriggers();

    // Write number of chips that had flush events for this link
    uint8_t num_chips_with_data = flush_events.size();
    flush_events_file.write((char*)&num_chips_with_data, sizeof(uint8_t));

    // Write flush events per chip in this link
    for(auto flush_chip_it = flush_events.begin();
        flush_chip_it != flush_events.end();
        flush_chip_it++)
    {
      uint8_t chip_id = flush_chip_it->first;
      flush_events_file.write((char*)&chip_id, sizeof(uint8_t));

      uint64_t num_flush_events = flush_chip_it->second.size();
      flush_events_file.write((char*)&num_flush_events, sizeof(uint64_t));

      for(auto flush_event_it = flush_chip_it->second.begin();
          flush_event_it != flush_chip_it->second.end();
          flush_event_it++)
      {
        uint64_t flush_trigger_id = *flush_event_it;
        flush_events_file.write((char*)&flush_trigger_id, sizeof(uint64_t));
      }
    }
  }
  flush_events_file.close();


  // ---------------------------------------------------------------
  // Write binary data file with readout abort (data overrun) events
  // ---------------------------------------------------------------
  // File format (same as for busyv basically):
  //
  //   Header:
  //   uint8_t:  number of data links
  //
  //   For each data link:
  //       uint8_t: Number of chips with data for this link
  //
  //       For each chip in data link which has data:
  //          Header:
  //             uint8_t:  Chip ID
  //             uint64_t: Number of "readout abort events"
  //          Data (for each trigger chip was in readout abort)
  //             uint64_t: trigger ID for readout abort event

  std::string ro_abort_event_filename = output_path + std::string("_ro_abort_events.dat");
  ofstream ro_abort_event_file(ro_abort_event_filename, std::ios_base::out | std::ios_base::binary);

  if(!ro_abort_event_file.is_open()) {
    std::cerr << "Error opening readout abort events file: ";
    std::cerr << ro_abort_event_filename << std::endl;
    return;
  } else {
    std::cout << "Writing readout abort events to file:\n\"";
    std::cout << ro_abort_event_filename << "\"" << std::endl;
  }


  // Write number of data links to file header
  ro_abort_event_file.write((char*)&num_data_links, sizeof(uint8_t));

  // Write readout abort events for each link
  for(uint64_t link_id = 0; link_id < mDataLinkParsers.size(); link_id++) {
    std::map<unsigned int, std::vector<uint64_t>> ro_abort_event
      = mDataLinkParsers[link_id]->getReadoutAbortTriggers();

    // Write number of chips that had readout abort events for this link
    uint8_t num_chips_with_data = ro_abort_event.size();
    ro_abort_event_file.write((char*)&num_chips_with_data, sizeof(uint8_t));

    // Write readout abort events per chip in this link
    for(auto readout_abort_chip_it = ro_abort_event.begin();
        readout_abort_chip_it != ro_abort_event.end();
        readout_abort_chip_it++)
    {
      uint8_t chip_id = readout_abort_chip_it->first;
      ro_abort_event_file.write((char*)&chip_id, sizeof(uint8_t));

      uint64_t num_ro_abort_event = readout_abort_chip_it->second.size();
      ro_abort_event_file.write((char*)&num_ro_abort_event, sizeof(uint64_t));

      for(auto readout_abort_it = readout_abort_chip_it->second.begin();
          readout_abort_it != readout_abort_chip_it->second.end();
          readout_abort_it++)
      {
        uint64_t ro_abort_trigger_id = *readout_abort_it;
        ro_abort_event_file.write((char*)&ro_abort_trigger_id, sizeof(uint64_t));
      }
    }
  }
  ro_abort_event_file.close();


  // ------------------------------------------------------
  // Write binary data file with fatal events
  // ------------------------------------------------------
  // File format (same as for busyv basically):
  //
  //   Header:
  //   uint8_t:  number of data links
  //
  //   For each data link:
  //       uint8_t: Number of chips with data for this link
  //
  //       For each chip in data link which has data:
  //          Header:
  //             uint8_t:  Chip ID
  //             uint64_t: Number of "fatal events"
  //          Data (for each trigger chip was in fatal mode)
  //             uint64_t: trigger ID for fatal event

  std::string fatal_event_filename = output_path + std::string("_fatal_events.dat");
  ofstream fatal_event_file(fatal_event_filename, std::ios_base::out | std::ios_base::binary);

  if(!fatal_event_file.is_open()) {
    std::cerr << "Error opening fatal events events file: ";
    std::cerr << fatal_event_filename << std::endl;
    return;
  } else {
    std::cout << "Writing readout fatal events to file:\n\"";
    std::cout << fatal_event_filename << "\"" << std::endl;
  }


  // Write number of data links to file header
  fatal_event_file.write((char*)&num_data_links, sizeof(uint8_t));

  // Write fatal events for each link
  for(uint64_t link_id = 0; link_id < mDataLinkParsers.size(); link_id++) {
    std::map<unsigned int, std::vector<uint64_t>> fatal_event
      = mDataLinkParsers[link_id]->getFatalTriggers();

    // Write number of chips that had fatal events for this link
    uint8_t num_chips_with_data = fatal_event.size();
    fatal_event_file.write((char*)&num_chips_with_data, sizeof(uint8_t));

    // Write fatal events per chip in this link
    for(auto fatal_chip_it = fatal_event.begin();
        fatal_chip_it != fatal_event.end();
        fatal_chip_it++)
    {
      uint8_t chip_id = fatal_chip_it->first;
      fatal_event_file.write((char*)&chip_id, sizeof(uint8_t));

      uint64_t num_fatal_event = fatal_chip_it->second.size();
      fatal_event_file.write((char*)&num_fatal_event, sizeof(uint64_t));

      for(auto fatal_it = fatal_chip_it->second.begin();
          fatal_it != fatal_chip_it->second.end();
          fatal_it++)
      {
        uint64_t fatal_trigger_id = *fatal_it;
        fatal_event_file.write((char*)&fatal_trigger_id, sizeof(uint64_t));
      }
    }
  }
  fatal_event_file.close();


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
