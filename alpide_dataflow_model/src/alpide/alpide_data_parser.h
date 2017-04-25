/**
 * @file   alpide_data_parser.h
 * @author Simon Voigt Nesbo
 * @date   March 3, 2017
 * @brief  Classes for parsing serial data from Alpide chip, 
 *         and building/reconstructing events/frames from the data
 */


#ifndef ALPIDE_DATA_PARSER_H
#define ALPIDE_DATA_PARSER_H

#include "alpide_data_format.h"
#include "../event/trigger_event.h"
#include <vector>

// Ignore warnings about use of auto_ptr in SystemC library
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#include <systemc.h>
#pragma GCC diagnostic pop


/// Enumerations used to identify the meaning of the different bytes in the data stream from
/// the Alpide chip. Not to be confused with the definitions in alpide_data_format.h which are used
/// to initialize the 24-bit FIFO words.
enum AlpideDataTypes {ALPIDE_IDLE,
                      ALPIDE_CHIP_HEADER1,
                      ALPIDE_CHIP_HEADER2,
                      ALPIDE_CHIP_TRAILER,
                      ALPIDE_CHIP_EMPTY_FRAME1,
                      ALPIDE_CHIP_EMPTY_FRAME2,
                      ALPIDE_REGION_HEADER,
                      ALPIDE_DATA_SHORT1,
                      ALPIDE_DATA_SHORT2,
                      ALPIDE_DATA_LONG1,
                      ALPIDE_DATA_LONG2,
                      ALPIDE_DATA_LONG3,
                      ALPIDE_BUSY_ON,
                      ALPIDE_BUSY_OFF,
                      ALPIDE_COMMA,
                      ALPIDE_UNKNOWN};


struct AlpideDataParsed {
  AlpideDataTypes data[3];
};


class AlpideEventFrame {
private:
  std::set<PixelData> mPixelDataSet;  
  bool mFrameCompleted;
  
public:
  AlpideEventFrame() : mFrameCompleted(false) {}
  bool pixelHitInEvent(PixelData& pixel) const;
  void setFrameCompleted(bool val) {mFrameCompleted = val;}
  bool getFrameCompleted(void) {return mFrameCompleted;}
  unsigned int getEventSize(void) const {return mPixelDataSet.size();}
  void addPixelHit(const PixelData& pixel) {
    mPixelDataSet.insert(pixel);    
  }
  std::set<PixelData>::const_iterator getPixelSetIterator(void) const {
    return mPixelDataSet.cbegin();
  }
  std::set<PixelData>::const_iterator getPixelSetEnd(void) const {
    return mPixelDataSet.cend();
  }
};


class AlpideEventBuilder {
private:
  std::vector<AlpideEventFrame> mEvents;

  unsigned int mCurrentRegion = 0;

  // Counters for statistics
  long mCommaCount;
  long mIdleCount;        // "Dedicated" idle word (ie. 24-bit data word starts with IDLE)
  long mIdleByteCount;    // Idle word byte counts
  long mBusyOnCount;
  long mBusyOffCount;
  long mDataShortCount;
  long mDataLongCount;
  long mRegionHeaderCount;
  long mChipHeaderCount;
  long mChipTrailerCount;
  long mChipEmptyFrameCount;
  long mUnknownDataWordCount;
  
public:
  unsigned int getNumEvents(void) const;
  const AlpideEventFrame* getNextEvent(void) const;
  void popEvent(void);
  void inputDataWord(AlpideDataWord dw);
  AlpideDataParsed parseDataWord(AlpideDataWord dw);
private:
  AlpideDataTypes parseNonHeaderBytes(uint8_t data);
};



class AlpideDataParser : sc_core::sc_module, public AlpideEventBuilder {
public:
  // SystemC signals
  sc_in<sc_uint<24> > s_serial_data_in;
  sc_in_clk s_clk_in;

private:
  void parserInputProcess(void);
  
public:
  AlpideDataParser(sc_core::sc_module_name name);
  void addTraces(sc_trace_file *wf, std::string name_prefix) const;
};


  



#endif
