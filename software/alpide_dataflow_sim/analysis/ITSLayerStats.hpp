/**
 * @file   ITSLayerStats.hpp
 * @author Simon Voigt Nesbo
 * @date   November 20, 2017
 * @brief  Statistics for one layer in ITS detector
 *
 */

#ifndef ITS_LAYER_STATS_HPP
#define ITS_LAYER_STATS_HPP

#include "ReadoutUnitStats.hpp"

class ITSLayerStats {
  unsigned int mLayer;
  unsigned int mNumStaves;
  std::string mSimDataPath;

  std::vector<ReadoutUnitStats> mRUStats;

  uint64_t mNumTriggers;

  // Index: trigger id
  // For each trigger it has the ratio between number of links a
  // trigger was sent to, and the total number of links,
  // averaged for all RUs in this layer
  std::vector<double> mTrigSentCoverage;

  // Index: trigger id
  // For each trigger it has the ratio between number of links a
  // trigger was sent to, and the total number of links minus filtered links/triggers,
  // averaged for all RUs in this layer.
  std::vector<double> mTrigSentExclFilteringCoverage;

  std::vector<double> mTrigReadoutCoverage;

  std::vector<double> mTrigReadoutExclFilteringCoverage;

  // Number of busy links vs trigger
  // Initialized in plotLayer() is called.
  std::vector<unsigned int> mBusyLinkCount;


  // Number of links with busy violation vs trigger
  // Initialized in plotLayer() is called.
  std::vector<unsigned int> mBusyVLinkCount;

public:
  ITSLayerStats(unsigned int layer_num, unsigned int num_staves,
                const char* path);
  void plotLayer(bool create_png, bool create_pdf);
  double getTriggerCoverage(uint64_t trigger_id) const;
  uint64_t getNumTriggers(void) {return mNumTriggers;}

  double getTrigSentCoverage(uint64_t trigger_id) const {
    return mTrigSentCoverage[trigger_id];
  }
  double getTrigSentExclFilteringCoverage(uint64_t trigger_id) const {
    return mTrigSentExclFilteringCoverage[trigger_id];
  }
  double getTrigReadoutCoverage(uint64_t trigger_id) const {
    return mTrigReadoutCoverage[trigger_id];
  }
  double getTrigReadoutExclFilteringCoverage(uint64_t trigger_id) const {
    return mTrigReadoutExclFilteringCoverage[trigger_id];
  }
  std::vector<unsigned int> getBusyLinkCount(void) const {
    return mBusyLinkCount;
  }
  std::vector<unsigned int> getBusyVLinkCount (void) const {
    return mBusyVLinkCount;
  }
};


#endif
