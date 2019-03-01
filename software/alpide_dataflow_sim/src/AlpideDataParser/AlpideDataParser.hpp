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

// Ignore warnings about use of auto_ptr and unused parameters in SystemC library
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#include <systemc.h>
#pragma GCC diagnostic pop


struct BusyEvent {
  uint64_t mBusyOnTime;
  uint64_t mBusyOffTime;
  uint64_t mBusyOnTriggerId;
  uint64_t mBusyOffTriggerId;

  BusyEvent(uint64_t busy_on_time, uint64_t busy_off_time,
            uint64_t busy_on_trigger, uint64_t busy_off_trigger)
    : mBusyOnTime(busy_on_time)
    , mBusyOffTime(busy_off_time)
    , mBusyOnTriggerId(busy_on_trigger)
    , mBusyOffTriggerId(busy_off_trigger)
  {}
};


class AlpideEventFrame {
private:
  std::set<PixelHit> mPixelHitSet;

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
  bool pixelHitInEvent(PixelHit& pixel) const;
  void setFrameCompleted(bool val) {mFrameCompleted = val;}
  bool getFrameCompleted(void) const {return mFrameCompleted;}

  void setReadoutFlags(uint8_t flags) {mReadoutFlags = flags;}
  bool getFatal(void) const;
  bool getReadoutAbort(void) const;
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

  unsigned int getEventSize(void) const {return mPixelHitSet.size();}
  void addPixelHit(const PixelHit& pixel) {
    mPixelHitSet.insert(pixel);
  }
  std::set<PixelHit>::const_iterator getPixelSetIterator(void) const {
    return mPixelHitSet.cbegin();
  }
  std::set<PixelHit>::const_iterator getPixelSetEnd(void) const {
    return mPixelHitSet.cend();
  }
};


class AlpideEventBuilder {
private:
  std::vector<AlpideEventFrame> mEvents;

  unsigned int mCurrentRegion = 0;
  std::map<AlpideDataType, uint64_t> mProtocolStats;

  /// Key: time (ns), value: number of data bytes for interval
  /// Data rate is only recorded for chip header/trailer, region header,
  /// and data long/short. Idle and busy on/off are words that the RU does
  /// not have to transmit further upstreams.
  /// Comma, unknown, and region trailer are simply ignored.
  std::map<uint64_t, unsigned int> mDataIntervalByteCounts;

  const unsigned int mDataIntervalNs;

  std::vector<uint64_t> mFatalTriggers;
  std::vector<uint64_t> mReadoutAbortTriggers;
  std::vector<uint64_t> mBusyViolationTriggers;
  std::vector<uint64_t> mFlushedIncomplTriggers;
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
  bool mDataWordStarted = false;
  uint8_t mCurrentDataWord[3];
  unsigned int mByteCounterCurrentWord;
  unsigned int mByteIndexCurrentWord;
  AlpideDataType mCurrentDwType;

public:
  AlpideEventBuilder(unsigned int data_rate_interval_ns,
                     bool save_events = true,
                     bool include_hit_data = false);

  void setCurrentTriggerId(uint64_t trigger_id) {
    mCurrentTriggerId = trigger_id;
  }

  void popEvent(void);
  void inputDataByte(std::uint8_t data, uint64_t trig_id, uint64_t time_now_ns);
  AlpideDataType parseDataByte(std::uint8_t data);

  unsigned int getNumEvents(void) const;
  const AlpideEventFrame* getNextEvent(void) const;

  unsigned int getDataIntervalNs(void) const {
    return mDataIntervalNs;
  }

  std::map<AlpideDataType, uint64_t>& getProtocolStats(void) {
    return mProtocolStats;
  }
  std::map<uint64_t, unsigned int>& getDataIntervalByteCounts(void) {
    return mDataIntervalByteCounts;
  }
  std::vector<uint64_t>& getFatalTriggers(void) {
    return mFatalTriggers;
  }
  std::vector<uint64_t>& getReadoutAbortTriggers(void) {
    return mReadoutAbortTriggers;
  }
  std::vector<uint64_t>& getBusyViolationTriggers(void) {
    return mBusyViolationTriggers;
  }
  std::vector<uint64_t>& getFlushedIncomplTriggers(void) {
    return mFlushedIncomplTriggers;
  }
  std::vector<BusyEvent>& getBusyEvents(void) {
    return mBusyEvents;
  }
};



class AlpideDataParser : sc_core::sc_module, public AlpideEventBuilder {
public:
  // SystemC signals
  sc_in<sc_uint<24>> s_serial_data_in;
  sc_in<uint64_t> s_serial_data_trig_id;
  sc_in_clk s_clk_in;
  sc_export<sc_signal<bool>> s_link_busy_out;

private:
  sc_signal<bool> s_link_busy;

  bool mWordMode;

  void parserInputProcess(void);

public:
  AlpideDataParser(sc_core::sc_module_name name, bool word_mode,
                   unsigned int data_rate_interval_ns, bool save_events = false);
  void addTraces(sc_trace_file *wf, std::string name_prefix) const;
};


#endif
///@}
