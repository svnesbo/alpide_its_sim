/**
 * @file   ReadoutUnitStats.hpp
 * @author Simon Voigt Nesbo
 * @date   November 20, 2017
 * @brief  Object for analyzing a RU_<layer>_<stave>_Trigger_stats.csv file
 *
 */

#ifndef READOUT_UNIT_STATS_HPP
#define READOUT_UNIT_STATS_HPP

#include "LinkStats.hpp"
//#include <cstdint>
#include <stdint.h>
#include <vector>
#include <map>
#include <string>

//using std::uint8_t;

static const uint8_t TRIGGER_SENT = 0;
static const uint8_t TRIGGER_NOT_SENT_BUSY = 1;
static const uint8_t TRIGGER_FILTERED = 2;

///@todo Restructure/add some new files, and include this struct
///      instead of redefining it here..
/* struct BusyEvent { */
/*   uint64_t mBusyOnTime; */
/*   uint64_t mBusyOffTime; */
/*   uint64_t mBusyOnTriggerId; */
/*   uint64_t mBusyOffTriggerId; */

/*   BusyEvent(uint64_t busy_on_time, uint64_t busy_off_time, */
/*             uint64_t busy_on_trigger, uint64_t busy_off_trigger) */
/*     : mBusyOnTime(busy_on_time) */
/*     , mBusyOffTime(busy_off_time) */
/*     , mBusyOnTriggerId(busy_on_trigger) */
/*     , mBusyOffTriggerId(busy_off_trigger) */
/*     {} */
/* }; */


class ReadoutUnitStats {
  // Indexing: [trigger_id][link_id]
  std::vector<LinkStats> mLinkStats;

  // Todo: Add busy count stats from the RU?


  // Index: [trigger_id][ctrl_link_id]
  std::vector<std::vector<uint8_t>> mTriggerActions;

  // Index: trigger id
  std::vector<double> mTriggerCoverage;

  // Trigger IDs where there was a mismatch in the
  // trigger filter status. Either all or no links should
  // have the TRIGGER_FILTERED state, if there are trigger
  // IDs where that was not the case, then they are put in this
  // vector.
  std::vector<uint64_t> mTriggerMismatch;

  uint64_t mNumTriggers;
  unsigned int mNumCtrlLinks;
  unsigned int mLayer;
  unsigned int mStave;

public:
  ReadoutUnitStats(unsigned int layer, unsigned int stave, const char* path);
  void readTrigActionsFile(std::string file_path_base);
  void readBusyEventFiles(std::string file_path_base);
  double getTriggerCoverage(uint64_t trigger_id) const;
  uint64_t getNumTriggers(void) const {return mNumTriggers;}
  void plotRU(const char* root_filename);
};


#endif
