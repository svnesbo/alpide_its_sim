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

  // Distribution of for how long the links are busy
  // ie. busy_off_time - busy_on_time
  // This vector has contributions from all of this RUs links
  std::vector<uint64_t> mAllBusyTime;

  // Distribution of for how many triggers the busy signals is asserted
  // ie. busy_off_trigger_id - busy_on_trigger_id
  // This vector has contributions from all of this RUs links
  std::vector<uint64_t> mAllBusyTriggerLengths;

  // Distribution of space/distance in
  // numbers of triggers between busy violations
  // This vector has contributions from all of this RUs links
  std::vector<uint64_t> mAllBusyVTriggerDistances;

  // Distribution of numbers of triggers with
  // busy violations in a sequence of busy violations
  // This vector has contributions from all of this RUs links
  std::vector<uint64_t> mAllBusyVTriggerSequences;


  // Protocol utilization date for each header field, for all links in this RU
  std::map<std::string, unsigned long> mProtocolUtilization;

  // Index in CSV file versus header field
  std::map<unsigned int, std::string> mProtUtilIndex;

  // Index: [trigger_id][ctrl_link_id]
  std::vector<std::vector<uint8_t>> mTriggerActions;

  // Index: trigger id
  // For each trigger it has the ratio between number of links a
  // trigger was sent to, and the total number of links.
  std::vector<double> mTrigSentCoverage;

  // Index: trigger id
  // For each trigger it has the ratio between number of links a
  // trigger was sent to, and the total number of links minus filtered links/triggers.
  std::vector<double> mTrigSentExclFilteringCoverage;


  std::vector<double> mTrigReadoutCoverage;

  std::vector<double> mTrigReadoutExclFilteringCoverage;

  // Trigger IDs where there was a mismatch in the
  // trigger filter status. Either all or no links should
  // have the TRIGGER_FILTERED state, if there are trigger
  // IDs where that was not the case, then they are put in this
  // vector. So this shouldn't really happen, and this vector should
  // always be empty for a ReadoutUnit.
  std::vector<uint64_t> mTriggerMismatch;

  uint64_t mNumTriggers;
  unsigned int mNumCtrlLinks;
  unsigned int mLayer;
  unsigned int mStave;

  std::string mSimDataPath;


  void readTrigActionsFile(std::string file_path_base);
  void readBusyEventFiles(std::string file_path_base);
  void readProtocolUtilizationFile(std::string file_path_base);

public:
  ReadoutUnitStats(unsigned int layer, unsigned int stave, const char* path);
  double getTrigSentCoverage(uint64_t trigger_id) const;
  double getTrigSentExclFilteringCoverage(uint64_t trigger_id) const;
  double getTrigReadoutCoverage(uint64_t trigger_id) const;
  double getTrigReadoutExclFilteringCoverage(uint64_t trigger_id) const;
  uint64_t getNumTriggers(void) const {return mNumTriggers;}
  void plotRU(bool create_png, bool create_pdf);
};


#endif
