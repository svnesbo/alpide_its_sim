/**
 * @file   ITSDetector.cpp
 * @author Simon Voigt Nesbo
 * @date   June 21, 2017
 * @brief  Mockup version of ITS detector.
 *         Accepts trigger input from the dummy CTP module, and communicates the trigger
 *         to the readout units, which will forward them to the Alpide objects.
 */


#include "ITSDetector.hpp"

using namespace ITS;

//Do I need SC_HAS_PROCESS if I only use SC_METHOD??
SC_HAS_PROCESS(ITSDetector);
///@param config Configuration of the ITS detector to simulate
///              (ie. number of staves per layer to include in simulation)
///@param trigger_filter_time Readout Units will filter out triggers more closely
///                           spaced than this time (specified in nano seconds).
ITSDetector::ITSDetector(sc_core::sc_module_name name,
                         const detectorConfig& config,
                         unsigned int trigger_filter_time)
  : sc_core::sc_module(name)
{
  verifyDetectorConfig(config);
  buildDetector(config, trigger_filter_time);

  SC_METHOD(triggerMethod);
  sensitive << E_trigger_in;
}


///@brief Verify that detector configuration is valid. Exit brutally if not
///@param config Configuration of the ITS detector to simulate
///              (ie. number of staves per layer to include in simulation)
///@throw runtime_error If too many staves specified for a layer, or if a total
///       of zero staves for all layers were specified.
void ITSDetector::verifyDetectorConfig(const detectorConfig& config) const
{
  unsigned int num_staves_total = 0;

  for(unsigned int i = 0; i < N_LAYERS; i++) {
    if(config.layer[i].num_staves > STAVES_PER_LAYER[i]) {
      std::string error_msg = "Too many staves specified for layer " + std::to_string(i);
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
///@param config Configuration of the ITS detector to simulate
///              (ie. number of staves per layer to include in simulation)
///@param trigger_filter_time Readout Units will filter out triggers more closely
///                           spaced than this time (specified in nano seconds).
void ITSDetector::buildDetector(const detectorConfig& config,
                                unsigned int trigger_filter_time)
{
  // Reserve space for all chips, even if they are not used (not allocated),
  // because we access/index them by index in the vectors, and vector access is O(1).
  mChipVector.resize(CHIP_COUNT_TOTAL, nullptr);

  for(unsigned int i = 0; i < N_LAYERS; i++) {
    std::cout << "Creating " << config.layer[i].num_staves << " staves for layer " << i << std::endl;
    for(unsigned int j = 0; j < config.layer[i].num_staves; j++) {
      std::cout << "Creating Stave and RU " << i << ":" << j << std::endl;

      std::string coords_str = std::to_string(i) + ":" + std::to_string(j);;
      std::string ru_name = "RU_" + coords_str;

      if(i < 3) {
        std::string stave_name = "IB_stave_" + coords_str;
        mLayers[i].push_back(std::make_shared<InnerBarrelStave>(stave_name.c_str(), i, j));
      } else if(i >= 3 && i < 5) {
        throw std::runtime_error("Middle barrel staves not implemented yet..");
        /*
        std::string stave_name = "MB_stave_" + coords_str;
        mLayers[i].push_back(std::make_shared<MiddleBarrelStave>(stave_name.c_str(), i, j));
        */
      } else {
        throw std::runtime_error("Middle barrel staves not implemented yet..");
        /*
        std::string stave_name = "OB_stave_" + coords_str;
        mLayers[i].push_back(std::make_shared<OuterBarrelStave>(stave_name.c_str(), i, j));
        */
      }

      auto stave = mLayers[i].back();

      stave->s_system_clk_in(s_system_clk_in);

      bool inner_barrel_mode = (i < 3);

      mReadoutUnits[i].push_back(std::make_shared<ReadoutUnit>(ru_name.c_str(),
                                                               i,
                                                               j,
                                                               stave->numCtrlLinks(),
                                                               stave->numDataLinks(),
                                                               trigger_filter_time,
                                                               inner_barrel_mode));



      // Connect the busy in/out signals for the RUs in a daisy chain
      if(j == config.layer[i].num_staves-1) {
        // Connect busy input of first RU to busy output of last RU.
        // If only 1 stave/RU was specified, this will actually connect
        // the busy signals in a loopback configuration on the RU.
        mReadoutUnits[i].front()->s_busy_in(mReadoutUnits[i].back()->s_busy_out);

        std::cout << "Connecting s_busy_in on front RU (" << i << ":" << 0;
        std::cout << " to s_busy_out on back RU (" << i << ":" << j << ")";
        std::cout << std::endl;
      }

      if(j > 0) {
        // Connect busy input of current RU to busy output of previous RU.
        // (Happens only when more than 1 stave/RU is specified).
        mReadoutUnits[i][j]->s_busy_in(mReadoutUnits[i][j-1]->s_busy_out);

        std::cout << "Connecting s_busy_in on RU " << i << ":" << j;
        std::cout << " to s_busy_out on RU " << i << ":" << j-1;
        std::cout << std::endl;
      }

      auto RU = mReadoutUnits[i].back();

      RU->s_system_clk_in(s_system_clk_in);

      for(unsigned int link_num = 0; link_num < stave->numCtrlLinks(); link_num++) {
        RU->s_alpide_control_output[link_num].bind(stave->socket_control_in[link_num]);
      }

      // Get a vector of pointer to the Alpide chips created by the new stave,
      // and add them to a map of chip id vs Alpide chip object.
      auto new_chips = stave->getChips();

      for(unsigned int link_num = 0; link_num < stave->numDataLinks(); link_num++) {
        stave->socket_data_out[link_num].bind(RU->s_alpide_data_input[link_num]);
        //RU->s_serial_data_input[link_num](new_chips[link_num]->s_serial_data_output);

        s_alpide_data_lines.push_back(std::make_shared<sc_signal<sc_uint<24>>>("data_line"));
        RU->s_serial_data_input[link_num](*s_alpide_data_lines.back());
        new_chips[link_num]->s_serial_data_output(*s_alpide_data_lines.back());
      }

      for(auto chip_it = new_chips.begin(); chip_it != new_chips.end(); chip_it++) {
        unsigned int chip_id = (*chip_it)->getChipId();
        // Don't allow more than one instance of the same Chip ID
        if(mChipVector[chip_id] != nullptr) {
          std::string error_msg = "Chip with ID ";
          error_msg += std::to_string(chip_id);
          error_msg += " created more than once..";

          throw std::runtime_error(error_msg);
        }

        mChipVector[chip_id] = *chip_it;
      }
    }
  }
}


///@brief Set a pixel in one of the detector's Alpide chip's (if it exists in the
///       detector configuration).
///@param chip_id Chip ID of Alpide chip
///@param h Pixel hit
void ITSDetector::pixelInput(unsigned int chip_id, const Hit& h)
{
  // Does the chip exist in our detector/simulation configuration?
  if(mChipVector[chip_id]) {
    mChipVector[chip_id]->pixelFrontEndInput(h);
  }
}


///@brief Input a pixel to the front end of one of the detector's
///       Alpide chip's (if it exists in the detector configuration).
///@param pos Position of Alpide chip in detector
///@param col Column in Alpide chip pixel matrix
///@param row Row in Alpide chip pixel matrix
void ITSDetector::pixelInput(const ITSPixelHit& h)
{
  unsigned int chip_id = detector_position_to_chip_id(h.getPosition());

  pixelInput(chip_id, h);
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


///@brief Set a pixel in one of the detector's Alpide chip's (if it exists in the
///       detector configuration).
///@param h Pixel hit data
void ITSDetector::setPixel(const ITSPixelHit& h)
{
  unsigned int chip_id = detector_position_to_chip_id(h.getPosition());
  unsigned int col = h.getCol();
  unsigned int row = h.getRow();

  setPixel(chip_id, col, row);
}


///@brief SystemC METHOD for distributing triggers to all readout units
void ITSDetector::triggerMethod(void)
{
  for(unsigned int i = 0; i < N_LAYERS; i++) {
    for(auto RU = mReadoutUnits[i].begin(); RU != mReadoutUnits[i].end(); RU++) {
      (*RU)->E_trigger_in.notify();
    }
  }
}
