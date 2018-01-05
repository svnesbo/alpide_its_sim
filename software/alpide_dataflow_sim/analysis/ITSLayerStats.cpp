/**
 * @file   ITSLayerStats.cpp
 * @author Simon Voigt Nesbo
 * @date   November 20, 2017
 * @brief  Statistics for one layer in ITS detector
 *
 */

#include "ITSLayerStats.hpp"
#include <iostream>
#include "TFile.h"
#include "TDirectory.h"

ITSLayerStats::ITSLayerStats(unsigned int layer_num, unsigned int num_staves,
                             const char* path,
                             bool create_png, bool create_pdf)
  : mLayer(layer_num)
{
  TDirectory* current_dir = gDirectory;

  if(gDirectory == nullptr) {
    std::cout << "ITSLayerStats::ITSLayerStats() error: gDirectory not initialized." << std::endl;
    exit(-1);
  }

  gDirectory->mkdir(Form("Layer_%i", mLayer));

  // Create and parse RU data, and generate plots in TFile
  for(unsigned int stave = 0; stave < num_staves; stave++) {
    // Keep changing back to this layer's directory,
    // because the plotRU() function changes the current directory.
    current_dir->cd(Form("Layer_%i", mLayer));

    mRUStats.emplace_back(layer_num, stave, path);

    mRUStats.back().plotRU(create_png, create_pdf);
  }

  // Todo: check that we have the same number of triggers in all RUs?
  uint64_t num_triggers = mRUStats[0].getNumTriggers();

  mTriggerCoverage.resize(num_triggers);

  for(uint64_t trigger_id = 0; trigger_id < num_triggers; trigger_id++) {
    double coverage = 0.0;
    for(unsigned int stave = 0; stave < num_staves; stave++) {
      coverage += mRUStats[stave].getTrigSentCoverage(trigger_id);
    }

    mTriggerCoverage[trigger_id] = coverage/num_staves;

    std::cout << "Layer " << layer_num << ", trigger " << trigger_id;
    std::cout << " coverage: " << mTriggerCoverage[trigger_id] << std::endl;
  }
}
