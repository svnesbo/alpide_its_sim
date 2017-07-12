/**
 * @file   ITSDetector.cpp
 * @author Simon Voigt Nesbo
 * @date   June 21, 2017
 * @brief  Mockup version of ITS detector.
 *         Accepts trigger input from the dummy CTP module, and communicates the trigger
 *         to the readout units, which will forward them to the Alpide objects.
 */


#include "ITSDetector.hpp"


//Do I need SC_HAS_PROCESS if I only use SC_METHOD??
//SC_HAS_PROCESS(ITSDetector);
ITSDetector::ITSDetector(sc_core::sc_module_name name, const detectorConfig& config)
  : sc_core::sc_module(name)
{
  ///@todo Construct ITS detector here.
  // In the constructor, we should specify which layers, and how many staves (readout units)
  // for each layer, that we want to include in the detector/simulation.
  // We don't specify number of Alpide chips, we always construct full staves.
  // Declare readout units and Alpide chips, and connect them together.

  verifyDetectorConfig(config);
  buildDetector(config);
}


///@brief Verify that detector configuration is valid. Exit brutally if not
///@throw runtime_error If too many staves specified for a layer, or if a total
///       of zero staves for all layers were specified.
void ITSDetector::verifyDetectorConfig(const detectorConfig& config) const
{
  unsigned int num_staves_total = 0;

  for(int i = 0; i < N_LAYERS; i++) {
    if(config.layer[i].num_staves > STAVES_PER_LAYER[i]) {
      std::string error_msg "Too many staves specified for layer " + std::to_string(i);
      throw std::runtime_error(error_msg);
    }
    num_staves_total += config.layer[i].num_staves;
  }

  if(num_staves_total == 0)
  {
    throw std::runtime_error("Detector with no staves specified.");
  }
}


///@brief Allocate memory and create the desired number of staves for each detector layer,
///       and create the chip map of chip id vs alpide chip object instance.
void ITSDetector::buildDetector(const detectorConfig& config)
{
  for(int i = 0; i < N_LAYERS; i++) {
    for(int j = 0; j < config.layer[i].num_staves; j++) {
      std::string name = "Layer " + std::to_string(i) + ", stave " + std::to_string(j);

      if(i < 3)
        layers.emplace_back(new InnerBarrelStave(name, i, j));
      else if(i >= 3 && i < 5)
        layers.emplace_back(new MiddleBarrelStave(name, i, j));
      else
        layers.emplace_back(new OuterBarrelStave(name, i, j));

      // Get a vector of pointer to the Alpide chips created by the new stave,
      // and add them to a map of chip id vs Alpide chip object.
      auto new_chips = layers.back()->getChips();
      for(auto chip_it = new_chips.begin(); chip_it != new_chips.end(); chip_it++) {
        // Don't allow more than one instance of the same Chip ID
        if(mChipMap.find(*chip_it->getChipId()) != mChipMap.end()) {
          std::string error_msg = "Chip with ID ";
          error_msg += std::to_string(*chip_it->getChipId());
          error_msg += " created more than once..";

          throw std::runtime_error(error_msg);
        }

        mChipMap[*chip_it->getChipId()] = *chip_it;
      }

      ///@todo Create ReadoutUnit here. Connect ReadoutUnit to other ReadoutUnits.
    }
  }
}
