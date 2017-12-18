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
  // Triggers that had busy violations
  std::vector<uint64_t> mBusyVTriggers;

  // Triggers for which the link was busy
  // (regardless of for how long in time it was actually busy)
  std::vector<uint64_t> mBusyTriggers;

  // Distribution of for how many triggers the busy signals is asserted
  // ie. busy_off_trigger_id - busy_on_trigger_id
  std::vector<uint64_t> mBusyTriggerLengths;


  // Distribution of space/distance in numbers of triggers
  // between busy violations
  std::vector<uint64_t> mBusyVTriggerDistances;

  // When, in time, that the link was busy
  std::vector<BusyTime> mBusyTime;
};


#endif
