/**
 * @file   BusyLinkWord.hpp
 * @author Simon Voigt Nesbo
 * @date   July 10, 2017
 * @brief  The structs in this file represents the data words that are transmitted
 *         on the Readout Units' BUSY IN/OUT links.
 *
 */

// Ignore warnings about use of auto_ptr in SystemC library
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#include <systemc.h>
#pragma GCC diagnostic pop


struct BusyLinkWord {
  int mOriginAddress;
  int mTimeStamp;

  BusyLinkWord(int address, int timestamp)
    : mOriginAddress(address)
    , mTimeStamp(timestamp)
  {}

  virtual ~BusyLinkWord() = 0;
};

BusyLinkWord::~BusyLinkWord()
{
}


struct BusyCountUpdate : BusyLinkWord {
  int mLinkBusyCount;
  bool mLocalBusyStatus;

  BusyCountUpdate(int address, int timestamp, int busy_link_count, bool local_busy)
    : BusyLinkWord(address, timestamp)
    , mLinkBusyCount(busy_link_count)
    , mLocalBusyStatus(local_busy)
  {}

  ~BusyCountUpdate() {}
};


struct BusyGlobalStatusUpdate : BusyLinkWord {
  bool mGlobalBusyStatus;

  BusyGlobalStatusUpdate(int address, int timestamp, bool busy_status)
    : BusyLinkWord(address, timestamp)
    , mGlobalBusyStatus(busy_status)
  {}

  ~BusyGlobalStatusUpdate() {}
};
