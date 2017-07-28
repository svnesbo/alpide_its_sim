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
  // Reserve space for all chips, even if they are not used (not allocated),
  // because we access/index them by index in the vectors, and vector access is O(1).
  mChipVector.resize(CHIP_COUNT_TOTAL);

  for(int i = 0; i < N_LAYERS; i++) {
    for(int j = 0; j < config.layer[i].num_staves; j++) {
      std::string coords_str = std::to_string(i) + ":" + std::to_string(j);;
      std::string ru_name = "RU_" + coords_str;

      mReadoutUnits[i].emplace_back(new ReadoutUnit(ru_name, i, j));
      if(j > 0) {
        ///@todo Connect busy chain of mReadoutUnits[i][j] to mReadoutUnits[i][j-1] here
      }

      if(j == config.layer[i].num_staves-1) {
        ///@todo Connect busy chain of mReadoutUnits[i][j] to mReadoutUnits[i][0] here
      }

      mReadoutUnits[i][j]->s_system_clk_in(s_system_clk_in);


      if(i < 3) {
        std::string stave_name = "IB_stave_" + coords_str;
        mLayers[i].emplace_back(new InnerBarrelStave(name, i, j));
      } else if(i >= 3 && i < 5) {
        std::string stave_name = "MB_stave_" + coords_str;
        mLayers[i].emplace_back(new MiddleBarrelStave(name, i, j));
      } else {
        std::string stave_name = "OB_stave_" + coords_str;
        mLayers[i].emplace_back(new OuterBarrelStave(name, i, j));
      }

      ///@todo Connect signals from mReadoutUnits[i][j] to mLayers[]


      // Get a vector of pointer to the Alpide chips created by the new stave,
      // and add them to a map of chip id vs Alpide chip object.
      auto new_chips = mLayers[i].back()->getChips();
      for(auto chip_it = new_chips.begin(); chip_it != new_chips.end(); chip_it++) {
        unsigned int chip_id = *chip_it->getChipId();
        // Don't allow more than one instance of the same Chip ID
        if(mChipVector[chip_id]) {
          std::string error_msg = "Chip with ID ";
          error_msg += std::to_string(chip_id);
          error_msg += " created more than once..";

          throw std::runtime_error(error_msg);
        }

        mChipVector[chip_id] = *chip_it;

        // Connect the control and data links to the readout unit
        ///@todo I'm gonna need a signal between these two I think...
        mReadoutUnits[i][j]->s_alpide_data_input(mChipVector[chip_id]->s_data_output);
        mReadoutUnits[i][j]->s_alpide_control_output(mChipVector[chip_id]->s_control_input);
      }
    }
  }
}


///@brief Set a pixel in one of the detector's Alpide chip's (if it exists in the
///       detector configuration).
///@param chip_id Chip ID of Alpide chip
///@param col Column in Alpide chip pixel matrix
///@param row Row in Alpide chip pixel matrix
void ITSDetector::setPixel(unsigned int chip_id, unsigned int col, unsigned int row)
{
  // Does the chip exist in our detector/simulation configuration?
  if(mChipVector[chip_id]) {
    ///@todo Check if chip is ready?
    //if(mChipVector[chip_id]->s_chip_ready) {
    //}

    mChipVector[chip_id]->setPixel(col, row);
  }
}


///@brief Set a pixel in one of the detector's Alpide chip's (if it exists in the
///       detector configuration).
///@param pos Position of Alpide chip in detector
///@param col Column in Alpide chip pixel matrix
///@param row Row in Alpide chip pixel matrix
void ITSDetector::setPixel(const detectorPosition& pos, unsigned int col, unsigned int row)
{
  unsigned int chip_id = detector_position_to_chip_id(pos);

  setPixel(chip_id, col, row);
}
