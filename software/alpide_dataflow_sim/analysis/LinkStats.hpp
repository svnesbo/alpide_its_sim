/**
 * @file   LinkStats.hpp
 * @author Simon Voigt Nesbo
 * @date   December 11, 2017
 * @brief  Statistics for one ALPIDE links
 *
 */

#ifndef LINK_STATS_HPP
#define LINK_STATS_HPP

//#include <cstdint>
#include <stdint.h>
#include <vector>
#include <map>

struct BusyTime {
  uint64_t mStartTimeNs;
  uint64_t mEndTimeNs;
  uint64_t mBusyTimeNs; // mEndTimeNs - mStartTimeNs
};

struct LinkStats {
  unsigned int mLayer; // Layer id
  unsigned int mStave; // Stave id
  unsigned int mLink;  // Link id

  // Triggers for which the link was busy
  // (regardless of for how long in time it was actually busy)
  std::vector<uint64_t> mBusyTriggers;

  // Triggers that had busy violations
  std::vector<uint64_t> mBusyVTriggers;

  // Triggers that had flushed incomplete
  std::vector<uint64_t> mFlushTriggers;

  // Triggers that the chip was in readout abort mode
  std::vector<uint64_t> mAbortTriggers;

  // Triggers that the chip was in fatal mode
  std::vector<uint64_t> mFatalTriggers;

  // Distribution of for how many triggers the busy signals is asserted
  // ie. busy_off_trigger_id - busy_on_trigger_id
  std::vector<uint64_t> mBusyTriggerLengths;

  // Distribution of space/distance in
  // numbers of triggers between busy violations
  std::vector<uint64_t> mBusyVTriggerDistances;

  // Distribution of space/distance in
  // numbers of triggers between flushed incompletes
  std::vector<uint64_t> mFlushTriggerDistances;

  // Distribution of space/distance in
  // numbers of triggers between readout abort events
  std::vector<uint64_t> mAbortTriggerDistances;

  // Distribution of space/distance in
  // numbers of triggers between fatal events
  std::vector<uint64_t> mFatalTriggerDistances;

  // Distribution of how long sequences of busy violations
  // we have, in terms of triggers.
  std::vector<uint64_t> mBusyVTriggerSequences;

  // Distribution of how long sequences of flushed incompletes
  // we have, in terms of triggers.
  std::vector<uint64_t> mFlushTriggerSequences;

  // Distribution of how long sequences of readout abort
  // events we have, in terms of triggers.
  std::vector<uint64_t> mAbortTriggerSequences;

  // Distribution of how long sequences of fatal mode
  // events we have, in terms of triggers.
  std::vector<uint64_t> mFatalTriggerSequences;

  // When, in time, that the link was busy
  std::vector<BusyTime> mBusyTime;

  // Protocol utilization date for each header field
  std::map<std::string, unsigned long> mProtocolUtilization;

  // Index in CSV file versus header field
  std::map<unsigned int, std::string> mProtUtilIndex;

  LinkStats(unsigned int layer_id,
            unsigned int stave_id,
            unsigned int link_id)
    : mLayer(layer_id)
    , mStave(stave_id)
    , mLink(link_id)
    {}

  void plotLink(void);
};


#endif
