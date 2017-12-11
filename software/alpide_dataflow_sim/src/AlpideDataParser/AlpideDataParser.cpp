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
///@param[in] pixel Reference to PixelData object
///@return True if pixel is in event frame, false if not.
bool AlpideEventFrame::pixelHitInEvent(PixelData& pixel) const
{
  if(mPixelDataSet.find(pixel) != mPixelDataSet.end())
    return true;
  else
    return false;
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
///@param use_fast_parser Don't analyze data at the binary level to find
///                       out what data words they are, use the data_type fields
///                       in AlpideDataWord instead.
AlpideEventBuilder::AlpideEventBuilder(bool save_events,
                                       bool include_hit_data,
                                       bool use_fast_parser)
  : mSaveEvents(save_events)
  , mIncludeHitData(include_hit_data)
  , mFastParserEnable(use_fast_parser)
{
  mProtocolStats[ALPIDE_IDLE] = 0;
  mProtocolStats[ALPIDE_CHIP_HEADER1] = 0;
  mProtocolStats[ALPIDE_CHIP_HEADER2] = 0;
  mProtocolStats[ALPIDE_CHIP_TRAILER] = 0;
  mProtocolStats[ALPIDE_CHIP_EMPTY_FRAME1] = 0;
  mProtocolStats[ALPIDE_CHIP_EMPTY_FRAME2] = 0;
  mProtocolStats[ALPIDE_REGION_HEADER] = 0;
  mProtocolStats[ALPIDE_REGION_TRAILER] = 0;
  mProtocolStats[ALPIDE_DATA_SHORT1] = 0;
  mProtocolStats[ALPIDE_DATA_SHORT2] = 0;
  mProtocolStats[ALPIDE_DATA_LONG1] = 0;
  mProtocolStats[ALPIDE_DATA_LONG2] = 0;
  mProtocolStats[ALPIDE_DATA_LONG3] = 0;
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


///@brief Takes a 3 byte Alpide data word as input, parses it, and depending on the data:
///       1) If this is a new Alpide data frame, a new AlpideEventFrame is created in mEvents
///       2) If this is data that belongs to the existing and most recent frame, hit data is added
///          to that frame.
///       3) If these are just idle words etc., nothing is done with them.
///@param[in] dw AlpideDataWord input to parse.
void AlpideEventBuilder::inputDataWord(AlpideDataWord dw)
{
  AlpideDataParsed data_parsed;

  if(mFastParserEnable == false)
    data_parsed = parseDataWord(dw);
  else {
    data_parsed.data[0] = dw.data_type[0];
    data_parsed.data[1] = dw.data_type[1];
    data_parsed.data[2] = dw.data_type[2];
  }

  //unsigned long data = (dw.data[2] << 16) | (dw.data[1] << 8) | dw.data[0];

  // This was used only for debug output..
  //std::bitset<24> data_bits(data);

  mBusyStatusChanged = false;

  // Create new frame/event?
  switch(data_parsed.data[2]) {
  case ALPIDE_CHIP_HEADER1:
  {
    //std::cout << "Got ALPIDE_CHIP_HEADER1: " << data_bits << std::endl;
    if(mSaveEvents == false && mEvents.empty() != true) {
      mEvents.clear();
    }
    mEvents.push_back(AlpideEventFrame());
    mEvents.back().setChipId(dw.data[2] & 0x0F);
    mEvents.back().setBunchCounterValue((uint16_t)dw.data[1] << 3);

    // A little dirty, but we should always have an AlpideChipHeader here
    AlpideChipHeader* dw_header = static_cast<AlpideChipHeader*>(&dw);
    mEvents.back().setTriggerId(dw_header->trigger_id);
    break;
  }

  case ALPIDE_CHIP_TRAILER:
    //std::cout << "Got ALPIDE_CHIP_TRAILER: " << data_bits << std::endl;
    if(!mEvents.empty()) {
      mEvents.back().setReadoutFlags(dw.data[2] & 0x0F);
      mEvents.back().setFrameCompleted(true);

      // Maintain a vector of trigger IDs for triggers
      // that resulted in a busy violation
      if(mEvents.back().getBusyViolation() == true)
        mBusyViolationTriggers.push_back(mEvents.back().getTriggerId());
    }
    break;

  case ALPIDE_CHIP_EMPTY_FRAME1:
  {
    //std::cout << "Got ALPIDE_CHIP_EMPTY_FRAME1: " << data_bits << std::endl;
    // Create an empty event frame
    if(mSaveEvents == false && mEvents.empty() != true) {
      mEvents.clear();
    }
    mEvents.push_back(AlpideEventFrame());
    mEvents.back().setChipId(dw.data[2] & 0x0F);
    mEvents.back().setBunchCounterValue((uint16_t)dw.data[1] << 3);
    mEvents.back().setFrameCompleted(true);

    // A little dirty, but we should always have an AlpideChipEmptryFrame here
    AlpideChipEmptyFrame* dw_empty_frame = static_cast<AlpideChipEmptyFrame*>(&dw);
    mEvents.back().setTriggerId(dw_empty_frame->trigger_id);
    break;
  }

  case ALPIDE_REGION_HEADER:
    //std::cout << "Got ALPIDE_REGION_HEADER: " << data_bits << std::endl;
    mCurrentRegion = dw.data[2] & 0b00011111;
    //std::cout << "\tCurrent region: " << mCurrentRegion << std::endl;
    break;

  case ALPIDE_REGION_TRAILER:
    // Do nothing. We should never see a region trailer word here
    break;

  case ALPIDE_DATA_SHORT1:
    //std::cout << "Got ALPIDE_DATA_SHORT1: " << data_bits << std::endl;
    if(!mEvents.empty() && mIncludeHitData) {
      uint8_t pri_enc_id = (dw.data[2] >> 2) & 0x0F;
      uint16_t addr = ((dw.data[2] & 0x03) << 8) | dw.data[1];
      mEvents.back().addPixelHit(PixelData(mCurrentRegion, pri_enc_id, addr));
      //std::cout << "\t" << "pri_enc: " << static_cast<unsigned int>(pri_enc_id);
      //std::cout << "\t" << "addr: " << addr << std::endl;
    }
    break;

  case ALPIDE_DATA_LONG1:
    //std::cout << "Got ALPIDE_DATA_LONG1: " << data_bits << std::endl;
    if(!mEvents.empty() && mIncludeHitData) {
      uint8_t pri_enc_id = (dw.data[2] >> 2) & 0x0F;
      uint16_t addr = ((dw.data[2] & 0x03) << 8) | dw.data[1];
      uint8_t hitmap = dw.data[0] & 0x7F;
      std::bitset<7> hitmap_bits(hitmap);

      //std::cout << "\t" << "pri_enc: " << static_cast<unsigned int>(pri_enc_id);
      //std::cout << "\t" << "addr: " << addr << std::endl;
      //std::cout << "\t" << "hitmap: " << hitmap_bits << std::endl;

      // Add hit for base address of cluster
      mEvents.back().addPixelHit(PixelData(mCurrentRegion, pri_enc_id, addr));

      // There's 7 hits in a hitmap
      for(int i = 0; i < 8; i++) {
        // Add a hit for each bit that is set in the hitmap
        if((hitmap >> i) & 0x01)
          mEvents.back().addPixelHit(PixelData(mCurrentRegion, pri_enc_id, addr+i+1));
      }
    }
    break;

  case ALPIDE_IDLE:
    //std::cout << "Got ALPIDE_IDLE: " << data_bits << std::endl;
    break;

    // Not used.. checking all 3 bytes at the bottom of this function
  case ALPIDE_BUSY_ON:
    // std::cout << "Got ALPIDE_BUSY_ON: " << std::endl;
    ///@todo Busy on here
    break;

    // Not used.. checking all 3 bytes at the bottom of this function
  case ALPIDE_BUSY_OFF:
    // std::cout << "Got ALPIDE_BUSY_OFF: " << std::endl;
    ///@todo Busy off here
    break;

  case ALPIDE_UNKNOWN:
    // std::cout << "Got ALPIDE_UNKNOWN: " << data_bits << std::endl;
    // std::cout << "Byte 2: " << std::hex << static_cast<unsigned>(dw.data[2]) << std::endl;
    // std::cout << "Byte 1: " << std::hex << static_cast<unsigned>(dw.data[1]) << std::endl;
    // std::cout << "Byte 0: " << std::hex << static_cast<unsigned>(dw.data[0]) << std::endl;
    // std::cout << std::dec;
    ///@todo Unknown Alpide data word received. Do something smart here?
    break;

  case ALPIDE_COMMA:
    //std::cout << "Got ALPIDE_COMMA: " << data_bits << std::endl;
    break;

  case ALPIDE_DATA_SHORT2:
  case ALPIDE_DATA_LONG2:
  case ALPIDE_DATA_LONG3:
  case ALPIDE_CHIP_HEADER2:
  case ALPIDE_CHIP_EMPTY_FRAME2:
    //std::cout << "Got ALPIDE_SOMETHING.., which I shouldn't be receiving here..: " << data_bits << std::endl;
    // These should never occur in data_parsed.byte[2].
    // They are here to get rid off compiler warnings :)
    break;
  }

  // Check/update link busy status
  if(data_parsed.data[2] == ALPIDE_BUSY_ON ||
     data_parsed.data[1] == ALPIDE_BUSY_ON ||
     data_parsed.data[0] == ALPIDE_BUSY_ON)
  {
    // Just put off time/trigger = on time/trigger for now,
    // they will get the right value when we get BUSY_OFF
    mBusyEvents.emplace_back(sc_time_stamp().value(),
                             sc_time_stamp().value(),
                             mCurrentTriggerId,
                             mCurrentTriggerId);
    mBusyStatus = true;
    mBusyStatusChanged = true;
  } else if(data_parsed.data[2] == ALPIDE_BUSY_OFF ||
            data_parsed.data[1] == ALPIDE_BUSY_OFF ||
            data_parsed.data[0] == ALPIDE_BUSY_OFF)
  {
    if(mBusyEvents.empty() == false) {
      mBusyEvents.back().mBusyOffTime = sc_time_stamp().value();
      mBusyEvents.back().mBusyOffTriggerId = mCurrentTriggerId;
    }

    mBusyStatus = false;
    mBusyStatusChanged = true;
  }

  // Increase statistics counters for protocol utilization
  mProtocolStats[data_parsed.data[2]]++;
  mProtocolStats[data_parsed.data[1]]++;
  mProtocolStats[data_parsed.data[0]]++;
}


///@brief Parse 3-byte Alpide data words the, and increase counters for
///       the different types of data words.
///       Note: the function only discovers what type of data word it is,
///       it does nothing with the data word's parameters
///@param[in] dw AlpideDataWord input to parse.
///@return AlpideDataParsed object with parsed data word type filled in for each byte
AlpideDataParsed AlpideEventBuilder::parseDataWord(AlpideDataWord dw)
{
  AlpideDataParsed data_parsed;

  // Parse most significant byte (data is sent MSB first)
  uint8_t data_word_check = dw.data[2] & MASK_DATA;
  uint8_t chip_word_check = dw.data[2] & MASK_CHIP;
  uint8_t region_header_word_check = dw.data[2] & MASK_REGION_HEADER;
  uint8_t idle_busy_comma_word_check = dw.data[2] & MASK_IDLE_BUSY_COMMA;


  if(data_word_check == DW_DATA_LONG) {
    data_parsed.data[2] = ALPIDE_DATA_LONG1;
    data_parsed.data[1] = ALPIDE_DATA_LONG2;
    data_parsed.data[0] = ALPIDE_DATA_LONG3;
  } else if(data_word_check == DW_DATA_SHORT) {
    data_parsed.data[2] = ALPIDE_DATA_SHORT1;
    data_parsed.data[1] = ALPIDE_DATA_SHORT2;
    data_parsed.data[0] = parseNonHeaderBytes(dw.data[0]);
  } else if(chip_word_check == DW_CHIP_HEADER) {
    data_parsed.data[2] = ALPIDE_CHIP_HEADER1;
    data_parsed.data[1] = ALPIDE_CHIP_HEADER2;
    data_parsed.data[0] = parseNonHeaderBytes(dw.data[0]);
  } else if(chip_word_check == DW_CHIP_TRAILER) {
    data_parsed.data[2] = ALPIDE_CHIP_TRAILER;
    data_parsed.data[1] = parseNonHeaderBytes(dw.data[1]);
    data_parsed.data[0] = parseNonHeaderBytes(dw.data[0]);
  } else if(chip_word_check == DW_CHIP_EMPTY_FRAME) {
    data_parsed.data[2] = ALPIDE_CHIP_EMPTY_FRAME1;
    data_parsed.data[1] = ALPIDE_CHIP_EMPTY_FRAME2;
    data_parsed.data[0] = parseNonHeaderBytes(dw.data[0]);
  } else if(region_header_word_check == DW_REGION_HEADER) {
    data_parsed.data[2] = ALPIDE_REGION_HEADER;
    data_parsed.data[1] = parseNonHeaderBytes(dw.data[1]);
    data_parsed.data[0] = parseNonHeaderBytes(dw.data[0]);
  } else if(dw.data[2] == DW_REGION_TRAILER) {
    // We should never see a region trailer here
    // It is included for debugging purposes only..
    data_parsed.data[2] = ALPIDE_REGION_TRAILER;
    data_parsed.data[1] = ALPIDE_REGION_TRAILER;
    data_parsed.data[0] = ALPIDE_REGION_TRAILER;
  } else if(idle_busy_comma_word_check == DW_IDLE) {
    data_parsed.data[2] = ALPIDE_IDLE;
    data_parsed.data[1] = parseNonHeaderBytes(dw.data[1]);
    data_parsed.data[0] = parseNonHeaderBytes(dw.data[0]);
  } else if(idle_busy_comma_word_check == DW_BUSY_ON) {
    data_parsed.data[2] = ALPIDE_BUSY_ON;
    data_parsed.data[1] = parseNonHeaderBytes(dw.data[1]);
    data_parsed.data[0] = parseNonHeaderBytes(dw.data[0]);
  } else if(idle_busy_comma_word_check == DW_BUSY_OFF) {
    data_parsed.data[2] = ALPIDE_BUSY_OFF;
    data_parsed.data[1] = parseNonHeaderBytes(dw.data[1]);
    data_parsed.data[0] = parseNonHeaderBytes(dw.data[0]);
  } else if(idle_busy_comma_word_check == DW_COMMA) {
    data_parsed.data[2] = ALPIDE_COMMA;
    data_parsed.data[1] = parseNonHeaderBytes(dw.data[1]);
    data_parsed.data[0] = parseNonHeaderBytes(dw.data[0]);
  } else {
    data_parsed.data[2] = ALPIDE_UNKNOWN;
    data_parsed.data[1] = parseNonHeaderBytes(dw.data[1]);
    data_parsed.data[0] = parseNonHeaderBytes(dw.data[0]);
  }

  return data_parsed;
}


///@brief Use this to parse the last 1-2 (least significant) bytes of a 24-bit Alpide data word,
///       for words which are known to not utilize these bytes, e.g.:
///       Data long uses all 3 bytes - don't use this function
///       Data short uses first 2 bytes - use this function for the last byte
///       Region header uses the first byte - use this function for the last two bytes
///       The function will return either IDLE, BUSY_ON, BUSY_OFF, or UNKNOWN for these bytes.
///@param[in] data One of the "additional" bytes in a data word to parse
///@return Data word type for "additional" byte provided in data argument
AlpideDataType AlpideEventBuilder::parseNonHeaderBytes(uint8_t data)
{
  if(data == DW_IDLE) {
    return ALPIDE_IDLE;
  } else if(data == DW_BUSY_ON) {
    //std::cout << "Got BUSY_ON" << std::endl;
    return ALPIDE_BUSY_ON;
  } else if(data == DW_BUSY_OFF) {
    //std::cout << "Got BUSY_OFF" << std::endl;
    return ALPIDE_BUSY_OFF;
  } else if(data == DW_COMMA) {
    //std::cout << "Got COMMA" << std::endl;
    return ALPIDE_COMMA;
  } else {
    return ALPIDE_UNKNOWN;
  }
}


SC_HAS_PROCESS(AlpideDataParser);
///@brief Data parser constructor
///@param name SystemC module name
///@param save_events Specify if the parser should store all events in memory,
///            or discard old events and only keep the latest one.
AlpideDataParser::AlpideDataParser(sc_core::sc_module_name name, bool save_events)
  : sc_core::sc_module(name)
  , AlpideEventBuilder(save_events)
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
  AlpideDataWord dw = s_serial_data_in.read();

  inputDataWord(dw);

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
