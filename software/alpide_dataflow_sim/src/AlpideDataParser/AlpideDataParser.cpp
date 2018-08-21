/**
 * @file   alpide_data_parser.h
 * @author Simon Voigt Nesbo
 * @date   March 6, 2017
 * @brief  Classes for parsing serial data from Alpide chip,
 *         and building/reconstructing events/frames from the data.
 *         A busy signal indicates if the parser detected BUSY ON/OFF words,
 *         which makes the parser useful for readout unit simulations.
 */

#include "misc/vcd_trace.hpp"
#include "AlpideDataParser.hpp"
#include <cstddef>
#include <iostream>
#include <bitset>


///@brief Look for a pixel hit in this event frame
///@param[in] pixel Reference to PixelHit object
///@return True if pixel is in event frame, false if not.
bool AlpideEventFrame::pixelHitInEvent(PixelHit& pixel) const
{
  if(mPixelHitSet.find(pixel) != mPixelHitSet.end())
    return true;
  else
    return false;
}


bool AlpideEventFrame::getFatal(void) const
{
  return ((mReadoutFlags & READOUT_FLAGS_FATAL) == READOUT_FLAGS_FATAL);
}

bool AlpideEventFrame::getReadoutAbort(void) const
{
  // Readout abort aka. data overrun mode
  return ((mReadoutFlags & READOUT_FLAGS_ABORT) == READOUT_FLAGS_ABORT);
}

bool AlpideEventFrame::getBusyViolation(void) const
{
  return (mReadoutFlags & READOUT_FLAGS_BUSY_VIOLATION) != 0;
}


bool AlpideEventFrame::getFlushedIncomplete(void) const
{
  return (mReadoutFlags & READOUT_FLAGS_FLUSHED_INCOMPLETE) != 0;
}


bool AlpideEventFrame::getStrobeExtended(void) const
{
  return (mReadoutFlags & READOUT_FLAGS_STROBE_EXTENDED) != 0;
}


bool AlpideEventFrame::getBusyTransition(void) const
{
  return (mReadoutFlags & READOUT_FLAGS_BUSY_TRANSITION) != 0;
}


///@brief Constructor for AlpideEventBuilder
///@param save_events Specify if the parser should store all events in memory,
///            or discard old events and only keep the latest one.
///@param include_hit_data Include pixel hits in the events that are built
AlpideEventBuilder::AlpideEventBuilder(bool save_events,
                                       bool include_hit_data)
  : mSaveEvents(save_events)
  , mIncludeHitData(include_hit_data)
{
  mProtocolStats[ALPIDE_IDLE] = 0;
  mProtocolStats[ALPIDE_CHIP_HEADER] = 0;
  mProtocolStats[ALPIDE_CHIP_TRAILER] = 0;
  mProtocolStats[ALPIDE_CHIP_EMPTY_FRAME] = 0;
  mProtocolStats[ALPIDE_REGION_HEADER] = 0;
  mProtocolStats[ALPIDE_REGION_TRAILER] = 0;
  mProtocolStats[ALPIDE_DATA_SHORT] = 0;
  mProtocolStats[ALPIDE_DATA_LONG] = 0;
  mProtocolStats[ALPIDE_BUSY_ON] = 0;
  mProtocolStats[ALPIDE_BUSY_OFF] = 0;
  mProtocolStats[ALPIDE_COMMA] = 0;
  mProtocolStats[ALPIDE_UNKNOWN] = 0;
}


///@brief Get number of events stored in event builder.
///       Only events that have been completed (fully received)
///       are taken into consideration.
///@return Number of (completed) events
unsigned int AlpideEventBuilder::getNumEvents(void) const
{
  unsigned int num_events = mEvents.size();

  if(num_events > 0) {
    if(mEvents.back().getFrameCompleted() == false)
      num_events--;
  }

  return num_events;
}


///@brief Get a reference to the next event. This does not delete the event, and successive calls
///       will return the same event unless popEvent() has been called.
///@return Pointer to the next event if there are more events, nullptr if there are no events.
const AlpideEventFrame* AlpideEventBuilder::getNextEvent(void) const
{
  if(mEvents.empty())
    return nullptr;
  else
    return &mEvents.front();
}


///@brief Pop/remove the oldest event (if there are any events, otherwise do nothing).
void AlpideEventBuilder::popEvent(void)
{
  if(!mEvents.empty())
    mEvents.erase(mEvents.begin());
}


///@brief Takes a 1 byte of Alpide data at a time as input, parses the stream
///       of data, and depending on the data:
///       1) If this is a new Alpide data frame, a new AlpideEventFrame is created in mEvents
///       2) If this is data that belongs to the existing and most recent frame, hit data is added
///          to that frame.
///       3) If these are just idle words etc., nothing is done with them.
///@param[in] data Byte of Alpide data to parse.
///@param[in] trig_id Trigger ID for the currently incoming data
void AlpideEventBuilder::inputDataByte(std::uint8_t data, uint64_t trig_id)
{
  if(mDataWordStarted == false) {
    mCurrentDwType = parseDataByte(data);
    mDataWordStarted = true;
    mByteCounterCurrentWord = 0;
    mByteIndexCurrentWord = 2;
  }

  mCurrentDataWord[mByteIndexCurrentWord] = data;
  mByteCounterCurrentWord++;
  mByteIndexCurrentWord--;

  // Increase statistics counters for protocol utilization
  mProtocolStats[mCurrentDwType]++;

  mBusyStatusChanged = false;

  // Create new frame/event?
  switch(mCurrentDwType) {
  case ALPIDE_CHIP_HEADER:
    if(mByteCounterCurrentWord == DW_CHIP_HEADER_SIZE) {
      //std::cout << "Got ALPIDE_CHIP_HEADER1: " << data_bits << std::endl;
      if(mSaveEvents == false && mEvents.empty() != true) {
        mEvents.clear();
      }
      mEvents.push_back(AlpideEventFrame());
      mEvents.back().setChipId(mCurrentDataWord[2] & 0x0F);
      mEvents.back().setBunchCounterValue((uint16_t)mCurrentDataWord[1] << 3);
      mEvents.back().setTriggerId(trig_id);

      mDataWordStarted = false;
    }
    break;

  case ALPIDE_CHIP_TRAILER:
    if(mByteCounterCurrentWord == DW_CHIP_TRAILER_SIZE) {
      //std::cout << "Got ALPIDE_CHIP_TRAILER: " << data_bits << std::endl;
      if(!mEvents.empty()) {
        mEvents.back().setReadoutFlags(mCurrentDataWord[2] & 0x0F);
        mEvents.back().setFrameCompleted(true);

        // Maintain vectors of trigger IDs for triggers
        // that resulted in FATAL condition, READOUT ABORT,
        // BUSY VIOLATION, or FLUSHED INCOMPLETE
        if(mEvents.back().getFatal() == true)
          mFatalTriggers.push_back(mEvents.back().getTriggerId());
        else if(mEvents.back().getReadoutAbort() == true)
          mReadoutAbortTriggers.push_back(mEvents.back().getTriggerId());
        else if(mEvents.back().getBusyViolation() == true)
          mBusyViolationTriggers.push_back(mEvents.back().getTriggerId());
        else if(mEvents.back().getFlushedIncomplete() == true)
          mFlushedIncomplTriggers.push_back(mEvents.back().getTriggerId());
      }

      mDataWordStarted = false;
    }
    break;

  case ALPIDE_CHIP_EMPTY_FRAME:
    if(mByteCounterCurrentWord == DW_CHIP_EMPTY_FRAME_SIZE) {
      //std::cout << "Got ALPIDE_CHIP_EMPTY_FRAME1: " << data_bits << std::endl;
      // Create an empty event frame
      if(mSaveEvents == false && mEvents.empty() != true) {
        mEvents.clear();
      }
      mEvents.push_back(AlpideEventFrame());
      mEvents.back().setChipId(mCurrentDataWord[2] & 0x0F);
      mEvents.back().setBunchCounterValue((uint16_t)mCurrentDataWord[1] << 3);
      mEvents.back().setFrameCompleted(true);
      mEvents.back().setTriggerId(trig_id);

      mDataWordStarted = false;
    }
    break;

  case ALPIDE_REGION_HEADER:
    if(mByteCounterCurrentWord == DW_REGION_HEADER_SIZE) {
      //std::cout << "Got ALPIDE_REGION_HEADER: " << data_bits << std::endl;
      mCurrentRegion = mCurrentDataWord[2] & 0b00011111;
      //std::cout << "\tCurrent region: " << mCurrentRegion << std::endl;

      mDataWordStarted = false;
    }
    break;

  case ALPIDE_REGION_TRAILER:
    // Do nothing. We should never see a region trailer word here
    break;

  case ALPIDE_DATA_SHORT:
    if(mByteCounterCurrentWord == DW_DATA_SHORT_SIZE) {
      //std::cout << "Got ALPIDE_DATA_SHORT1: " << data_bits << std::endl;
      if(!mEvents.empty() && mIncludeHitData) {
        uint8_t pri_enc_id = (mCurrentDataWord[2] >> 2) & 0x0F;
        uint16_t addr = ((mCurrentDataWord[2] & 0x03) << 8) | mCurrentDataWord[1];
        mEvents.back().addPixelHit(PixelHit(mCurrentRegion, pri_enc_id, addr));
        //std::cout << "\t" << "pri_enc: " << static_cast<unsigned int>(pri_enc_id);
        //std::cout << "\t" << "addr: " << addr << std::endl;
      }
      mDataWordStarted = false;
    }
    break;

  case ALPIDE_DATA_LONG:
    if(mByteCounterCurrentWord == DW_DATA_LONG_SIZE) {
      //std::cout << "Got ALPIDE_DATA_LONG1: " << data_bits << std::endl;
      if(!mEvents.empty() && mIncludeHitData) {
        uint8_t pri_enc_id = (mCurrentDataWord[2] >> 2) & 0x0F;
        uint16_t addr = ((mCurrentDataWord[2] & 0x03) << 8) | mCurrentDataWord[1];
        uint8_t hitmap = mCurrentDataWord[0] & 0x7F;
        std::bitset<7> hitmap_bits(hitmap);

        //std::cout << "\t" << "pri_enc: " << static_cast<unsigned int>(pri_enc_id);
        //std::cout << "\t" << "addr: " << addr << std::endl;
        //std::cout << "\t" << "hitmap: " << hitmap_bits << std::endl;

        // Add hit for base address of cluster
        mEvents.back().addPixelHit(PixelHit(mCurrentRegion, pri_enc_id, addr));

        // There's 7 hits in a hitmap
        for(int i = 0; i < 8; i++) {
          // Add a hit for each bit that is set in the hitmap
          if((hitmap >> i) & 0x01)
            mEvents.back().addPixelHit(PixelHit(mCurrentRegion, pri_enc_id, addr+i+1));
        }
      }
      mDataWordStarted = false;
    }
    break;

    // Not used.. checking all 3 bytes at the bottom of this function
  case ALPIDE_BUSY_ON:
    // std::cout << "Got ALPIDE_BUSY_ON: " << std::endl;
    //
    // Just put off time/trigger = on time/trigger for now,
    // they will get the right value when we get BUSY_OFF
    mBusyEvents.emplace_back(sc_time_stamp().value(),
                             sc_time_stamp().value(),
                             mCurrentTriggerId,
                             mCurrentTriggerId);
    mBusyStatus = true;
    mBusyStatusChanged = true;
    mDataWordStarted = false;
    break;

    // Not used.. checking all 3 bytes at the bottom of this function
  case ALPIDE_BUSY_OFF:
    // std::cout << "Got ALPIDE_BUSY_OFF: " << std::endl;
    if(mBusyEvents.empty() == false) {
      mBusyEvents.back().mBusyOffTime = sc_time_stamp().value();
      mBusyEvents.back().mBusyOffTriggerId = mCurrentTriggerId;
    }

    mBusyStatus = false;
    mBusyStatusChanged = true;
    mDataWordStarted = false;
    break;

  case ALPIDE_IDLE:
  case ALPIDE_COMMA:
  case ALPIDE_UNKNOWN:
  default:
    mDataWordStarted = false;
    break;
  }
}


///@brief A byte from Alpide data stream
///@param[in] data Alpide data byte
///@return AlpideDataParsed object with parsed data word type filled in for each byte
AlpideDataType AlpideEventBuilder::parseDataByte(std::uint8_t data)
{
  // Parse most significant byte (data is sent MSB first)
  uint8_t data_word_check = data & MASK_DATA;

  if(data_word_check == DW_DATA_LONG)
    return ALPIDE_DATA_LONG;
  else if(data_word_check == DW_DATA_SHORT)
    return ALPIDE_DATA_SHORT;

  uint8_t chip_word_check = data & MASK_CHIP;

  if(chip_word_check == DW_CHIP_HEADER)
    return ALPIDE_CHIP_HEADER;
  else if(chip_word_check == DW_CHIP_TRAILER)
    return ALPIDE_CHIP_TRAILER;
  else if(chip_word_check == DW_CHIP_EMPTY_FRAME)
    return ALPIDE_CHIP_EMPTY_FRAME;

  uint8_t region_header_word_check = data & MASK_REGION_HEADER;

  if(region_header_word_check == DW_REGION_HEADER)
    return ALPIDE_REGION_HEADER;

  if(data == DW_REGION_TRAILER)
    // We should never see a region trailer here
    // It is included for debugging purposes only..
    return ALPIDE_REGION_TRAILER;

  uint8_t idle_busy_comma_word_check = data & MASK_IDLE_BUSY_COMMA;

  if(idle_busy_comma_word_check == DW_IDLE)
    return ALPIDE_IDLE;
  if(idle_busy_comma_word_check == DW_BUSY_ON)
    return ALPIDE_BUSY_ON;
  if(idle_busy_comma_word_check == DW_BUSY_OFF)
    return ALPIDE_BUSY_OFF;
  if(idle_busy_comma_word_check == DW_COMMA)
    return ALPIDE_COMMA;
  else
    return ALPIDE_UNKNOWN;
}


SC_HAS_PROCESS(AlpideDataParser);
///@brief Data parser constructor
///@param name SystemC module name
///@param save_events Specify if the parser should store all events in memory,
///            or discard old events and only keep the latest one.
AlpideDataParser::AlpideDataParser(sc_core::sc_module_name name, bool word_mode, bool save_events)
  : sc_core::sc_module(name)
  , AlpideEventBuilder(save_events)
  , mWordMode(word_mode)
{
  s_link_busy_out(s_link_busy);

  SC_METHOD(parserInputProcess);
  sensitive_pos << s_clk_in;
}


///@brief Matrix readout SystemC method. Expects a 3-byte word input on each clock edge.
///       The 3-byte data word is passed to the underlying base class for processing and
///       event frame generation.
///       A busy signal indicates if the parser has detected BUSY ON/OFF words.
void AlpideDataParser::parserInputProcess(void)
{
  sc_uint<24> dw = s_serial_data_in.read();
  uint64_t trig_id = s_serial_data_trig_id.read();

  inputDataByte((uint8_t)dw.range(23,16), trig_id);

  // Word mode is used for inner barrel chips
  // Outer barrel chips only output 1 byte per 40MHz clock cycle
  if(mWordMode) {
    inputDataByte((uint8_t)dw.range(15,8), trig_id);
    inputDataByte((uint8_t)dw.range(7,0), trig_id);
  }

  if(mBusyStatusChanged) {
    ///@todo Do something smart here? Implement a notification/event maybe?
    s_link_busy.write(mBusyStatus);
  }
}


///@brief      Add SystemC signals to log in VCD trace file.
///@param[in,out] wf Pointer to VCD trace file object
///@param[in]  name_prefix Name prefix to be added to all the trace names
void AlpideDataParser::addTraces(sc_trace_file *wf, std::string name_prefix) const
{
  std::stringstream ss;
  ss << name_prefix << "alpide_data_parser" << ".";
  std::string parser_name_prefix = ss.str();

  addTrace(wf, parser_name_prefix, "serial_data_in", s_serial_data_in);
  addTrace(wf, parser_name_prefix, "clk_in", s_clk_in);
}
