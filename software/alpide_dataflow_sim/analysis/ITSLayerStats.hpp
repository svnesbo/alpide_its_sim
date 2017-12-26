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
  std::vector<double> mTriggerCoverage;

public:
  ITSLayerStats(unsigned int layer_num, unsigned int num_staves,
                const char* path,
                bool create_png, bool create_pdf);
  double getTriggerCoverage(uint64_t trigger_id) const;
};


#endif
