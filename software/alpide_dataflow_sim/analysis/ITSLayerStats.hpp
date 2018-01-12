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
  std::vector<ReadoutUnitStats> mRUStats;

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

public:
  ITSLayerStats(unsigned int layer_num, unsigned int num_staves,
                const char* path,
                bool create_png, bool create_pdf);
  double getTriggerCoverage(uint64_t trigger_id) const;
};


#endif
