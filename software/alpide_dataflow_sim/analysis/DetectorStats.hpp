/**
 * @file   DetectorStats.hpp
 * @author Simon Voigt Nesbo
 * @date   January 16, 2018
 * @brief  Statistics for whole ITS detector
 *
 */

#ifndef DETECTOR_STATS_HPP
#define DETECTOR_STATS_HPP

#include <string>
#include <map>
#include "ITSLayerStats.hpp"
#include "../src/settings/Settings.hpp"
#include "../src/ITS/ITS_config.hpp"

class DetectorStats {
  ITS::detectorConfig mConfig;
  unsigned int mEventRateKhz;
  unsigned int mNumLayers;
  std::string mSimRunDataPath;
  std::vector<ITSLayerStats*> mLayerStats;

  // Index: trigger id
  // For each trigger it has the ratio between number of links a
  // trigger was sent to, and the total number of links,
  // averaged for all RUs in a layer, and then averaged for all layers
  std::vector<double> mTrigSentCoverage;

  // Index: trigger id
  // For each trigger it has the ratio between number of links a
  // trigger was sent to, and the total number of links minus filtered links/triggers,
  // averaged for all RUs in a layer, and then averaged for all layers
  std::vector<double> mTrigSentExclFilteringCoverage;

  std::vector<double> mTrigReadoutCoverage;

  std::vector<double> mTrigReadoutExclFilteringCoverage;

public:
  DetectorStats(ITS::detectorConfig config,
                unsigned int event_rate_khz,
                const char* sim_run_data_path);

  void plotDetector(bool create_png, bool create_pdf);
};


#endif
