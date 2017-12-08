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


struct BusyEvent {
  uint64_t mBusyOnTime;
  uint64_t mBusyOffTime;
  uint64_t mTriggerId;

  BusyEvent(uint64_t busy_on_time, uint64_t busy_off_time, uint64_t trigger_id)
    : mBusyOnTime(busy_on_time)
    , mBusyOffTime(busy_off_time)
    , mTriggerId(trigger_id)
  {}
};


struct AlpideDataParsed {
  AlpideDataType data[3];
};


class AlpideEventFrame {
private:
  std::set<PixelData> mPixelDataSet;

  ///@brief Indicates that we got the CHIP_TRAILER word,
  ///       and received all the data there is for this frame
  bool mFrameCompleted = false;

  // Readout status flags from ALPIDE CHIP_TRAILER word
  uint8_t mReadoutFlags;

  uint8_t mChipId = 0;
  uint64_t mTriggerId = 0;
  uint16_t mBunchCounterValue = 0;

public:
  AlpideEventFrame() {}
  bool pixelHitInEvent(PixelData& pixel) const;
  void setFrameCompleted(bool val) {mFrameCompleted = val;}
  bool getFrameCompleted(void) const {return mFrameCompleted;}

  void setReadoutFlags(uint8_t flags) {mReadoutFlags = flags;}
  bool getBusyViolation(void) const;
  bool getFlushedIncomplete(void) const;
  bool getStrobeExtended(void) const;
  bool getBusyTransition(void) const;

  void setChipId(uint8_t id) {mChipId = id;}
  void setTriggerId(uint64_t trigger_id) {mTriggerId = trigger_id;}
  void setBunchCounterValue(uint16_t bc_val) {mBunchCounterValue = bc_val;}
  uint8_t getChipId(void) const {return mChipId;}
  uint64_t getTriggerId(void) const {return mTriggerId;}
  uint16_t getBunchCounterValue(void) const {return mBunchCounterValue;}

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
  std::vector<uint64_t> mBusyViolationTriggers;
  std::vector<BusyEvent> mBusyEvents;
  bool mSaveEvents;

  ///@brief Current trigger ID, should be updated by e.g. ReadoutUnit.
  ///       Not to be confused with the trigger ID for an event that
  ///       is read out. Basically just used to have current trigger ID
  ///       in mBusyEvents.
  uint64_t mCurrentTriggerId = 0;

protected:
  bool mBusyStatus = false;
  bool mBusyStatusChanged = false;
  bool mIncludeHitData;
  bool mFastParserEnable;

public:
  AlpideEventBuilder(bool save_events = true,
                     bool include_hit_data = false,
                     bool use_fast_parser = true);

  void setCurrentTriggerId(uint64_t trigger_id) {
    mCurrentTriggerId = trigger_id;
  }

  void popEvent(void);
  void inputDataWord(AlpideDataWord dw);
  AlpideDataParsed parseDataWord(AlpideDataWord dw);

  unsigned int getNumEvents(void) const;
  const AlpideEventFrame* getNextEvent(void) const;

  std::map<AlpideDataType, uint64_t> getProtocolStats(void) {
    return mProtocolStats;
  }
  std::vector<uint64_t> getBusyViolationTriggers(void) {
    return mBusyViolationTriggers;
  }
  std::vector<BusyEvent> getBusyEvents(void) {
    return mBusyEvents;
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
  AlpideDataParser(sc_core::sc_module_name name, bool save_events = false);
  void addTraces(sc_trace_file *wf, std::string name_prefix) const;
};


#endif
///@}
