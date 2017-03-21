/**
 * @file   alpide_data_parser.h
 * @author Simon Voigt Nesbo
 * @date   March 6, 2017
 * @brief  Classes for parsing serial data from Alpide chip, 
 *         and building/reconstructing events/frames from the data
 */

#include "../misc/vcd_trace.h"
#include "alpide_data_parser.h"
#include <cstddef>
#include <iostream>
#include <bitset>


///@brief Look for a pixel hit in this event frame
///@param pixel Reference to PixelData object
///@return True if pixel is in event frame, false if not.
bool AlpideEventFrame::pixelHitInEvent(PixelData& pixel) const
{
  if(mPixelDataSet.find(pixel) != mPixelDataSet.end())
    return true;
  else
    return false;
}


unsigned int AlpideEventBuilder::getNumEvents(void) const
{
  return mEvents.size();
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
///@param dw AlpideDataWord input to parse.
void AlpideEventBuilder::inputDataWord(AlpideDataWord dw)
{
  AlpideDataParsed data_parsed = parseDataWord(dw);
  unsigned long data = (dw.data[2] << 16) | (dw.data[1] << 8) | dw.data[0];
  std::bitset<24> data_bits(data);

  // Create new frame/event?
  switch(data_parsed.data[2]) {
  case ALPIDE_CHIP_HEADER1:
    std::cout << "Got ALPIDE_CHIP_HEADER1: " << data_bits << std::endl;
    mEvents.push_back(AlpideEventFrame());
    break;
    
  case ALPIDE_CHIP_TRAILER:
    std::cout << "Got ALPIDE_CHIP_TRAILER: " << data_bits << std::endl;    
    if(!mEvents.empty())
      mEvents.back().setFrameCompleted(true);
    break;
    
  case ALPIDE_CHIP_EMPTY_FRAME1:
    std::cout << "Got ALPIDE_CHIP_EMPTY_FRAME1: " << data_bits << std::endl;    
    // Create an empty event frame
    mEvents.push_back(AlpideEventFrame());
    mEvents.back().setFrameCompleted(true);
    break;
    
  case ALPIDE_REGION_HEADER:
    std::cout << "Got ALPIDE_REGION_HEADER: " << data_bits << std::endl;    
    mCurrentRegion = dw.data[2] & 0b00011111;
    std::cout << "\tCurrent region: " << mCurrentRegion << std::endl;
    break;
    
  case ALPIDE_DATA_SHORT1:
    std::cout << "Got ALPIDE_DATA_SHORT1: " << data_bits << std::endl;
    if(!mEvents.empty()) {
      uint8_t pri_enc_id = (dw.data[2] >> 2) & 0x0F;
      uint16_t addr = ((dw.data[2] & 0x03) << 8) | dw.data[1];
      mEvents.back().addPixelHit(PixelData(mCurrentRegion, pri_enc_id, addr));
      std::cout << "\t" << "pri_enc: " << static_cast<unsigned int>(pri_enc_id);
      std::cout << "\t" << "addr: " << addr << std::endl;
    }
    break;
    
  case ALPIDE_DATA_LONG1:
    std::cout << "Got ALPIDE_DATA_LONG1: " << data_bits << std::endl;
    if(!mEvents.empty()) {
      uint8_t pri_enc_id = (dw.data[2] >> 2) & 0x0F;
      uint16_t addr = ((dw.data[2] & 0x03) << 8) | dw.data[1];      
      uint8_t hitmap = dw.data[0] & 0x7F;
      std::bitset<7> hitmap_bits(hitmap);

      std::cout << "\t" << "pri_enc: " << static_cast<unsigned int>(pri_enc_id);
      std::cout << "\t" << "addr: " << addr << std::endl;
      std::cout << "\t" << "hitmap: " << hitmap_bits << std::endl;

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
    std::cout << "Got ALPIDE_IDLE: " << data_bits << std::endl;    
    break;
    
  case ALPIDE_BUSY_ON:
    std::cout << "Got ALPIDE_BUSY_ON: " << data_bits << std::endl;    
    ///@todo Busy on here
    break;
    
  case ALPIDE_BUSY_OFF:
    std::cout << "Got ALPIDE_BUSY_OFF: " << data_bits << std::endl;    
    ///@todo Busy off here
    break;
    
  case ALPIDE_UNKNOWN:
    std::cout << "Got ALPIDE_UNKNOWN: " << data_bits << std::endl;
    std::cout << "Byte 2: " << std::hex << static_cast<unsigned>(dw.data[2]) << std::endl;
    std::cout << "Byte 1: " << std::hex << static_cast<unsigned>(dw.data[1]) << std::endl;
    std::cout << "Byte 0: " << std::hex << static_cast<unsigned>(dw.data[0]) << std::endl;
    ///@todo Unknown Alpide data word received. Do something smart here?
    break;

  case ALPIDE_DATA_SHORT2:
  case ALPIDE_DATA_LONG2:
  case ALPIDE_DATA_LONG3:
  case ALPIDE_CHIP_HEADER2:
  case ALPIDE_CHIP_EMPTY_FRAME2:
    std::cout << "Got ALPIDE_SOMETHING.., which I shouldn't be receiving here..: " << data_bits << std::endl;    
    // These should never occur in data_parsed.byte[2].
    // They are here to get rid off compiler warnings :)
    break;
  }
}


///@brief Parse 3-byte Alpide data words the, and increase counters for the different types of
///       data words. Note: the function only discovers what type of data word it is, it does
///       nothing with the data word's parameters
///@param dw AlpideDataWord input to parse.
///@return AlpideDataParsed object with parsed data word type filled in for each byte
AlpideDataParsed AlpideEventBuilder::parseDataWord(AlpideDataWord dw)
{
  AlpideDataParsed data_parsed;
  
  // Parse most significant byte - Check all options...
  uint8_t data_word_check = dw.data[2] & MASK_DATA;
  uint8_t chip_word_check = dw.data[2] & MASK_CHIP;
  uint8_t region_word_check = dw.data[2] & MASK_REGION_HEADER;
  uint8_t idle_busy_word_check = dw.data[2] & MASK_IDLE_BUSY;    


  if(data_word_check == DW_DATA_LONG) {
    mDataLongCount++;
    data_parsed.data[2] = ALPIDE_DATA_LONG1;
    data_parsed.data[1] = ALPIDE_DATA_LONG2;
    data_parsed.data[0] = ALPIDE_DATA_LONG3;    
  } else if(data_word_check == DW_DATA_SHORT) {
    mDataShortCount++;
    data_parsed.data[2] = ALPIDE_DATA_SHORT1;
    data_parsed.data[1] = ALPIDE_DATA_SHORT2;
    data_parsed.data[0] = parseNonHeaderBytes(dw.data[0]);
  } else if(chip_word_check == DW_CHIP_HEADER) {
    mChipHeaderCount++;
    data_parsed.data[2] = ALPIDE_CHIP_HEADER1;
    data_parsed.data[1] = ALPIDE_CHIP_HEADER2;
    data_parsed.data[0] = parseNonHeaderBytes(dw.data[0]);
  } else if(chip_word_check == DW_CHIP_TRAILER) {
    mChipTrailerCount++;
    data_parsed.data[2] = ALPIDE_CHIP_TRAILER;
    data_parsed.data[1] = parseNonHeaderBytes(dw.data[1]);            
    data_parsed.data[0] = parseNonHeaderBytes(dw.data[0]);        
  } else if(chip_word_check == DW_CHIP_EMPTY_FRAME) {
    mChipEmptyFrameCount++;    
    data_parsed.data[2] = ALPIDE_CHIP_EMPTY_FRAME1;
    data_parsed.data[1] = ALPIDE_CHIP_EMPTY_FRAME2;
    data_parsed.data[0] = parseNonHeaderBytes(dw.data[0]);    
  } else if(region_word_check == DW_REGION_HEADER) {
    mRegionHeaderCount++;    
    data_parsed.data[2] = ALPIDE_REGION_HEADER;
    data_parsed.data[1] = parseNonHeaderBytes(dw.data[1]);            
    data_parsed.data[0] = parseNonHeaderBytes(dw.data[0]);            
  } else if(idle_busy_word_check == DW_IDLE) {
    mIdleCount++;
    mIdleByteCount++;    
    data_parsed.data[2] = ALPIDE_IDLE;
    data_parsed.data[1] = parseNonHeaderBytes(dw.data[1]);            
    data_parsed.data[0] = parseNonHeaderBytes(dw.data[0]);                
  } else if(idle_busy_word_check == DW_BUSY_ON) {
    mBusyOnCount++;    
    data_parsed.data[2] = ALPIDE_BUSY_ON;
    data_parsed.data[1] = parseNonHeaderBytes(dw.data[1]);            
    data_parsed.data[0] = parseNonHeaderBytes(dw.data[0]);                    
  } else if(idle_busy_word_check == DW_BUSY_OFF) {
    mBusyOffCount++;    
    data_parsed.data[2] = ALPIDE_BUSY_OFF;
    data_parsed.data[1] = parseNonHeaderBytes(dw.data[1]);            
    data_parsed.data[0] = parseNonHeaderBytes(dw.data[0]);                        
  } else {
    mUnknownDataWordCount++;    
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
///       It will also increase counters for the corresponding words.
///@param data One of the "additional" bytes in a data word to parse
///@return Data word type for "additional" byte provided in data argument
AlpideDataTypes AlpideEventBuilder::parseNonHeaderBytes(uint8_t data)
{
  if(data == DW_IDLE) {
    mIdleByteCount++;
    return ALPIDE_IDLE;
  } else if(data == DW_BUSY_ON) {
    std::cout << "Got BUSY_ON" << std::endl;
    mBusyOnCount++;
    return ALPIDE_BUSY_ON;
  } else if(data == DW_BUSY_OFF) {
    std::cout << "Got BUSY_OFF" << std::endl;    
    mBusyOffCount++;
    return ALPIDE_BUSY_OFF;
  } else {
    mUnknownDataWordCount++;
    return ALPIDE_UNKNOWN;
  }
}


SC_HAS_PROCESS(AlpideDataParser);
AlpideDataParser::AlpideDataParser(sc_core::sc_module_name name)
  : sc_core::sc_module(name)
{
  SC_METHOD(parserInputProcess);
  sensitive_pos << s_clk_in;  
}


///@brief Matrix readout SystemC method. Expects a 3-byte word input on each clock edge.
///       The 3-byte data word is passed to the underlying base class for processing and
///       event frame generation.
void AlpideDataParser::parserInputProcess(void)
{
  AlpideDataWord dw;

  // dw.data[0] = s_serial_data_in.read() & 0xFF;
  // dw.data[1] = (s_serial_data_in.read() >> 8) & 0xFF;
  // dw.data[2] = (s_serial_data_in.read() >> 16) & 0xFF;

  dw.data[0] = s_serial_data_in.read().range(7, 0);
  dw.data[1] = s_serial_data_in.read().range(15,8);
  dw.data[2] = s_serial_data_in.read().range(23,16);    
    
  inputDataWord(dw);
}


///@brief Add SystemC signals to log in VCD trace file.
///@param wf Pointer to VCD trace file object
///@param name_prefix Name prefix to be added to all the trace names
void AlpideDataParser::addTraces(sc_trace_file *wf, std::string name_prefix) const
{
  std::stringstream ss;
  ss << name_prefix << "alpide_data_parser" << ".";
  std::string parser_name_prefix = ss.str();

  addTrace(wf, parser_name_prefix, "serial_data_in", s_serial_data_in);
  addTrace(wf, parser_name_prefix, "clk_in", s_clk_in);
}
