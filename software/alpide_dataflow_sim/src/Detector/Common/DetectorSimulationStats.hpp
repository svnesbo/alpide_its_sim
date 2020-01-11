/**
 * @file   DetectorSimulationStats.hpp
 * @author Simon Voigt Nesbo
 * @date   August 28, 2017
 * @brief  Functions etc. for writing simulation stats to file for Detector/ALPIDE
 */

#ifndef DETECTOR_STIMULATION_STATS_HPP
#define DETECTOR_STIMULATION_STATS_HPP

#include <string>
#include <Alpide/Alpide.hpp>
#include "DetectorConfig.hpp"

namespace Detector
{
  void writeAlpideStatsToFile(std::string output_path,
                              const std::map<unsigned int, std::shared_ptr<Alpide>>& alpide_map,
                              t_global_chip_id_to_position_func global_chip_id_to_position_func);
}

#endif
