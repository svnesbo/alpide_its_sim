/**
 * @file   ITSSimulationStats.hpp
 * @author Simon Voigt Nesbo
 * @date   August 28, 2017
 * @brief  Functions etc. for writing simulation stats to file for ITS/ALPIDE
 */

#ifndef ITS_STIMULATION_STATS_HPP
#define ITS_STIMULATION_STATS_HPP

#include <string>
#include <Alpide/Alpide.hpp>

void writeAlpideStatsToFile(std::string output_path,
                            const std::vector<std::shared_ptr<Alpide>>& alpide_vec);


#endif
