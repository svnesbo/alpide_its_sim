/**
 * @file   ITSLayerStats.cpp
 * @author Simon Voigt Nesbo
 * @date   November 20, 2017
 * @brief  Statistics for one layer in ITS detector
 *
 */

#include "ITSLayerStats.hpp"
#include <iostream>

ITSLayerStats::ITSLayerStats(unsigned int layer_num, unsigned int num_staves, const char* path)
  : mLayer(layer_num)
{
  for(unsigned int stave = 0; stave < num_staves; stave++) {
    mRUStats.emplace_back(layer_num, stave, path);

    mRUStats.back().plotRU();
  }

  // Todo: check that we have the same number of triggers in all RUs?
  uint64_t num_triggers = mRUStats[0].getNumTriggers();

  mTriggerCoverage.resize(num_triggers);

  for(uint64_t trigger_id = 0; trigger_id < num_triggers; trigger_id++) {
    double coverage = 0.0;
    for(unsigned int stave = 0; stave < num_staves; stave++) {
      coverage += mRUStats[stave].getTriggerCoverage(trigger_id);
    }

    mTriggerCoverage[trigger_id] = coverage/num_staves;

    std::cout << "Layer " << layer_num << ", trigger " << trigger_id;
    std::cout << " coverage: " << mTriggerCoverage[trigger_id] << std::endl;
  }
}
