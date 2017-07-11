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


const int BW_BUSY_COUNT_UPDATE          = 0;
const int BW_BUSY_GLOBAL_STATUS_UPDATE  = 1;

struct BusyLinkWord {
  int mOriginAddress;
  sc_time mTimestamp;

  BusyLinkWord(int address, sc_time timestamp)
    : mOriginAddress(address)
    , mTimeStamp(timestamp)
    {}

  virtual int getBusyWordType(void) = 0;
};


struct BusyCountUpdate : BusyLinkWord {
  int mLinkBusyCount;
  bool mLocalBusyStatus;

  BusyCountUpdate(int address, sc_time timestamp, int busy_link_count, bool local_busy)
    : BusyLinkWord(address, timestamp)
    , mLinkBusyCount(busy_link_count)
    , mLocalBusy(local_busy)
    {}

  int getBusyWordType(void) {
    return BW_BUSY_COUNT_UPDATE;
  }
};


struct BusyGlobalStatusUpdate : BusyLinkWord {
  bool mGlobalBusyStatus;

  BusyLinkStatusUpdate(int address, sc_time timestamp, bool busy_status)
    : BusyLinkWord(address, timestamp)
    , mGlobalBusyStatus(busy_status)
    {}

  int getBusyWordType(void) {
    return BW_BUSY_GLOBAL_STATUS_UPDATE;
  }
}
