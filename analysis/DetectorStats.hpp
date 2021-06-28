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
#include <memory>
#include "ITSLayerStats.hpp"
#include "../src/Settings/Settings.hpp"
#include "Detector/Common/DetectorConfig.hpp"
#include "EventData.hpp"
#include "Detector/ITS/ITS_constants.hpp"

class DetectorStats {
  Detector::DetectorConfigBase mConfig;
  std::map<std::string, double> mSimParams;
  unsigned long mSimTimeNs;
  unsigned int mNumLayers;
  std::string mSimType;
  std::string mSimRunDataPath;
  std::vector<ITSLayerStats*> mLayerStats;

  std::shared_ptr<EventData> mEventData;

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
  DetectorStats(Detector::DetectorConfigBase config,
                std::map<std::string, double> sim_params,
                unsigned long sim_time_ns,
                std::string sim_type,
                const char* sim_run_data_path,
                std::shared_ptr<EventData> event_data);

  void plotDetector(bool create_png, bool create_pdf);
};


#endif
