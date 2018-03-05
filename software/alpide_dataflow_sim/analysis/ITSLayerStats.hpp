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
  unsigned long mSimTimeNs;
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

  // Number of links with flushed incomplete vs trigger
  // Initialized in plotLayer() is called.
  std::vector<unsigned int> mFlushLinkCount;

  // Number of links in readout abort vs trigger
  // Initialized in plotLayer() is called.
  std::vector<unsigned int> mAbortLinkCount;

  // Number of links in fatal mode vs trigger
  // Initialized in plotLayer() is called.
  std::vector<unsigned int> mFatalLinkCount;

  // Total number of busy events for this layer
  unsigned int mNumBusyEvents = 0;

  // Total number of busy violation events for this layer
  unsigned int mNumBusyVEvents = 0;

  // Total number of flushed incomplete events for this layer
  unsigned int mNumFlushEvents = 0;

  // Total number of readout abort events for this layer
  // Each trigger a link is in readout abort counts as a
  // "readout abort event".
  unsigned int mNumAbortEvents = 0;

  // Total number of fatal mode events for this layer
  // Each trigger a link is in fatal mode counts as a
  // "fatal mode event".
  unsigned int mNumFatalEvents = 0;

  double mAvgTrigDistrEfficiency = 0;
  double mAvgTrigReadoutEfficiency = 0;

  std::vector<double> mDataRatesMbps;
  std::vector<double> mProtocolRatesMbps;

public:
  ITSLayerStats(unsigned int layer_num, unsigned int num_staves,
                unsigned long sim_time_ns, const char* path);
  void plotLayer(bool create_png, bool create_pdf);
  double getTriggerCoverage(uint64_t trigger_id) const;
  uint64_t getNumTriggers(void) {return mNumTriggers;}
  unsigned int getNumBusyEvents(void) {return mNumBusyEvents;}
  unsigned int getNumBusyVEvents(void) {return mNumBusyVEvents;}
  unsigned int getNumFlushEvents(void) {return mNumFlushEvents;}
  unsigned int getNumAbortEvents(void) {return mNumAbortEvents;}
  unsigned int getNumFatalEvents(void) {return mNumFatalEvents;}
  double getAvgTrigDistrEfficiency(void) {return mAvgTrigDistrEfficiency;}
  double getAvgTrigReadoutEfficiency(void) {return mAvgTrigReadoutEfficiency;}

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
  std::vector<unsigned int> getFlushLinkCount (void) const {
    return mFlushLinkCount;
  }
  std::vector<unsigned int> getAbortLinkCount (void) const {
    return mAbortLinkCount;
  }
  std::vector<unsigned int> getFatalLinkCount (void) const {
    return mFatalLinkCount;
  }
  std::vector<double> getDataRatesMbps(void) const {
    return mDataRatesMbps;
  }
  std::vector<double> getProtocolRatesMbps(void) const {
    return mProtocolRatesMbps;
  }
};


#endif
