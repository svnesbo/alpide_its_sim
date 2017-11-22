/**
 * @file   alpide_data_parser.h
 * @author Simon Voigt Nesbo
 * @date   March 3, 2017
 * @brief  Classes for parsing serial data from Alpide chip,
 *         and building/reconstructing events/frames from the data
 *         A busy signal indicates if the parser detected BUSY ON/OFF words,
 *         which makes the parser useful for readout unit simulations.
 */


///@defgroup data_parser Alpide Data Parser
///@ingroup alpide
///@{
#ifndef ALPIDE_DATA_PARSER_H
#define ALPIDE_DATA_PARSER_H

#include "Alpide/AlpideDataWord.hpp"
#include "Alpide/EventFrame.hpp"
#include <vector>

// Ignore warnings about use of auto_ptr in SystemC library
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#include <systemc.h>
#pragma GCC diagnostic pop


struct AlpideDataParsed {
  AlpideDataType data[3];
};


class AlpideEventFrame {
private:
  std::set<PixelData> mPixelDataSet;
  bool mFrameCompleted;

public:
  AlpideEventFrame() : mFrameCompleted(false) {}
  bool pixelHitInEvent(PixelData& pixel) const;
  void setFrameCompleted(bool val) {mFrameCompleted = val;}
  bool getFrameCompleted(void) const {return mFrameCompleted;}
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
  std::map<AlpideDataType, uint64_t> mProtocolStats;
  bool mSaveEvents;

protected:
  bool mBusyStatus = false;
  bool mBusyStatusChanged = false;
  bool mIncludeHitData;
  bool mFastParserEnable;

public:
  unsigned int getNumEvents(void) const;
  const AlpideEventFrame* getNextEvent(void) const;
  void popEvent(void);
  void inputDataWord(AlpideDataWord dw);
  AlpideDataParsed parseDataWord(AlpideDataWord dw);
  AlpideEventBuilder(bool save_events = false,
                     bool include_hit_data = false,
                     bool use_fast_parser = true);
  std::map<AlpideDataType, uint64_t> getProtocolStats(void) {
    return mProtocolStats;
  }
private:
  AlpideDataType parseNonHeaderBytes(uint8_t data);
};



class AlpideDataParser : sc_core::sc_module, public AlpideEventBuilder {
public:
  // SystemC signals
  sc_in<AlpideDataWord> s_serial_data_in;
  sc_in_clk s_clk_in;
  sc_export<sc_signal<bool>> s_link_busy_out;

private:
  sc_signal<bool> s_link_busy;

  void parserInputProcess(void);

public:
  AlpideDataParser(sc_core::sc_module_name name, bool save_events = true);
  void addTraces(sc_trace_file *wf, std::string name_prefix) const;
};


#endif
///@}
