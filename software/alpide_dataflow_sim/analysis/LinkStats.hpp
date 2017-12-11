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
  std::vector<uint64_t> mBusyViolationTriggers;
  std::vector<uint64_t> mBusyTriggers;
  std::vector<BusyTime> mBusyTime;
};


#endif
