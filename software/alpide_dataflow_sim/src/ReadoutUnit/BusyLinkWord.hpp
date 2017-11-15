/**
 * @file   BusyLinkWord.hpp
 * @author Simon Voigt Nesbo
 * @date   July 10, 2017
 * @brief  The structs in this file represents the data words that are transmitted
 *         on the Readout Units' BUSY IN/OUT links.
 *
 */

#ifndef BUSY_LINK_WORD_HPP
#define BUSY_LINK_WORD_HPP

// Ignore warnings about use of auto_ptr in SystemC library
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#include <systemc.h>
#pragma GCC diagnostic pop


struct BusyLinkWord {
  int mOriginAddress;
  int mTimeStamp;

  BusyLinkWord()
    : mOriginAddress(0)
    , mTimeStamp(0)
    {}

  BusyLinkWord(int address, int timestamp)
    : mOriginAddress(address)
    , mTimeStamp(timestamp)
    {}

  virtual std::string getString(void) const
    {
      return std::string("BUSY_LINK_WORD");
    };

  inline friend void sc_trace(sc_trace_file *tf, const BusyLinkWord& busy_word,
                              const std::string& name ) {
    sc_trace(tf, busy_word.mOriginAddress, name + ".OriginAddress");
    sc_trace(tf, busy_word.mTimeStamp, name + ".TimeStamp");
  }

  ///@todo Overload this in derived classes....
  inline friend std::ostream& operator<<(std::ostream& stream, const BusyLinkWord& busy_word) {
    stream << std::hex << busy_word.mTimeStamp;
    stream << ":";
    stream << std::hex << busy_word.mOriginAddress;
    return stream;
  }
};

// BusyLinkWord::~BusyLinkWord()
// {
// }


struct BusyCountUpdate : public BusyLinkWord {
  int mLinkBusyCount;
  bool mLocalBusyStatus;

  BusyCountUpdate(int address, int timestamp, int busy_link_count, bool local_busy)
    : BusyLinkWord(address, timestamp)
    , mLinkBusyCount(busy_link_count)
    , mLocalBusyStatus(local_busy)
  {}

  virtual std::string getString(void) const
    {
      return std::string("BUSY_COUNT_UPDATE");
    }

//  ~BusyCountUpdate() {}
};


struct BusyGlobalStatusUpdate : public BusyLinkWord {
  bool mGlobalBusyStatus;

  BusyGlobalStatusUpdate(int address, int timestamp, bool busy_status)
    : BusyLinkWord(address, timestamp)
    , mGlobalBusyStatus(busy_status)
  {}

  virtual std::string getString(void) const
    {
      return std::string("BUSY_GLOBAL_STATUS_UPDATE");
    }

//  ~BusyGlobalStatusUpdate() {}
};


#endif
