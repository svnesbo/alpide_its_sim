/**
 * @file   PCTDetector.cpp
 * @author Simon Voigt Nesbo
 * @date   January 14, 2019
 * @brief  Mockup version of PCT detector.
 *         Accepts trigger inputs and communicates the trigger
 *         to the readout units, which will forward them to the Alpide objects.
 */


#include "Detector/PCT/PCTDetector.hpp"
#include "Detector/Common/DetectorSimulationStats.hpp"
#include "Detector/PCT/PCT_creator.hpp"
#include <misc/vcd_trace.hpp>


using namespace PCT;

SC_HAS_PROCESS(PCTDetector);
///@param name SystemC module name
///@param config Configuration of the PCT detector to simulate
///              (ie. number of staves per layer to include in simulation)
///@param trigger_filter_time Readout Units will filter out triggers more closely
///                           spaced than this time (specified in nano seconds).
///@param trigger_filter_enable Enable/disable trigger filtering
PCTDetector::PCTDetector(sc_core::sc_module_name name,
                         const PCTDetectorConfig& config,
                         unsigned int trigger_filter_time,
                         bool trigger_filter_enable)
  : sc_core::sc_module(name)
  , mReadoutUnits("RU", PCT::N_LAYERS)
  , mDetectorStaves("Stave", PCT::N_LAYERS)
  , mConfig(config)
{
  verifyDetectorConfig(config);
  buildDetector(config, trigger_filter_time, trigger_filter_enable);

  SC_METHOD(triggerMethod);
  sensitive << E_trigger_in;
  dont_initialize();
}


///@brief Verify that detector configuration is valid. Exit brutally if not
///@param config Configuration of the PCT detector to simulate
///              (ie. number of staves per layer to include in simulation)
///@throw runtime_error If too many staves specified for a layer, or if a total
///       of zero staves for all layers were specified.
void PCTDetector::verifyDetectorConfig(const PCTDetectorConfig& config) const
{
  unsigned int num_staves_total = 0;

  if(config.num_layers == 0) {
    std::string error_msg = "No layers specified for PCT simulation.";
    throw std::runtime_error(error_msg);
  } else if(config.num_layers > PCT::N_LAYERS) {
    std::string error_msg = "Too many layers specified for PCT simulation.";
    throw std::runtime_error(error_msg);
  }

  for(unsigned int i = 0; i < config.num_layers; i++) {
    if(config.layer[i].num_sub_staves_per_full_stave != SUB_STAVES_PER_STAVE) {
      std::string error_msg = "Incorrect number of sub-staves specified for layer " + std::to_string(i);
      throw std::runtime_error(error_msg);
    }

    if(config.layer[i].num_staves > STAVES_PER_LAYER) {
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
///@param config Configuration of the PCT detector to simulate
///              (ie. number of staves per layer to include in simulation)
///@param trigger_filter_time Readout Units will filter out triggers more closely
///                           spaced than this time (specified in nano seconds).
///@param trigger_filter_enable Enable/disable trigger filtering
void PCTDetector::buildDetector(const PCTDetectorConfig& config,
                                unsigned int trigger_filter_time,
                                bool trigger_filter_enable)
{
  // Reserve space for all chips, even if they are not used (not allocated),
  // because we access/index them by index in the vectors, and vector access is O(1).
  mChipVector.resize(PCT::CHIP_COUNT_TOTAL, nullptr);

  for(unsigned int lay_id = 0; lay_id < config.num_layers; lay_id++) {
    unsigned int num_staves = config.layer[lay_id].num_staves;
    unsigned int num_data_links = num_staves * PCT::DATA_LINKS_PER_STAVE;
    unsigned int num_ctrl_links = num_staves * PCT::CTRL_LINKS_PER_STAVE;

    std::cout << "Creating " << num_staves;
    std::cout << " staves and one RU for layer " << lay_id;
    std::cout << std::endl;

    // Create sc_vectors with ReadoutUnit and Staves for this layer
    mReadoutUnits[lay_id].init(PCT::READOUT_UNITS_PER_LAYER, RUCreator(lay_id,
                                                                       num_data_links,
                                                                       num_ctrl_links,
                                                                       trigger_filter_time,
                                                                       trigger_filter_enable));

    mReadoutUnits[lay_id][0].s_system_clk_in(s_system_clk_in);

    // Connect the busy in to the busy out signal on the RU,
    // because it's not really used currently
    mReadoutUnits[lay_id][0].s_busy_in(mReadoutUnits[lay_id][0].s_busy_out);

    // Create staves
    mDetectorStaves[lay_id].init(num_staves, StaveCreator(lay_id, mConfig));

    for(unsigned int sta_id = 0; sta_id < config.layer[lay_id].num_staves; sta_id++) {
      auto &RU = mReadoutUnits[lay_id][0];
      auto &stave = mDetectorStaves[lay_id][sta_id];

      stave.s_system_clk_in(s_system_clk_in);

      // Only 1 control link for IB stave
      RU.s_alpide_control_output[sta_id].bind(stave.socket_control_in[0]);

      // Get a vector of pointer to the Alpide chips created by the new stave,
      // and add them to a map of chip id vs Alpide chip object.
      auto new_chips = stave.getChips();

      for(unsigned int link_num = 0; link_num < stave.numDataLinks(); link_num++) {
        unsigned int RU_data_link_id = stave.numDataLinks()*sta_id + link_num;

        stave.socket_data_out[link_num].bind(RU.s_alpide_data_input[RU_data_link_id]);
        RU.s_serial_data_input[RU_data_link_id](new_chips[link_num]->s_serial_data_out_exp);
        RU.s_serial_data_trig_id[RU_data_link_id](new_chips[link_num]->s_serial_data_trig_id_exp);
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
        mNumChips++;
      }
    }
  }
}


///@brief Input a pixel to the front end of one of the detector's
///       Alpide chip's (if it exists in the detector configuration).
///@param pix PixelHit object with pixel matrix coordinates and chip id
void PCTDetector::pixelInput(const std::shared_ptr<PixelHit>& pix)
{
  // Does the chip exist in our detector/simulation configuration?
  if(mChipVector[pix->getChipId()]) {
    mChipVector[pix->getChipId()]->pixelFrontEndInput(pix);
  } else {
    std::cout << "Chip " << pix->getChipId() << " does not exist.";
  }
}


///@brief Set a pixel in one of the detector's Alpide chip's (if it exists in the
///       detector configuration).
///       This function will call the chip object's setPixel() function, which directly sets
///       a pixel in the last MEB in the chip.
///       Generally you would NOT want to use this function for simulations.
///@param chip_id Chip ID of Alpide chip
///@param col Column in Alpide chip pixel matrix
///@param row Row in Alpide chip pixel matrix
void PCTDetector::setPixel(unsigned int chip_id, unsigned int col, unsigned int row)
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
///       This function will call the chip object's setPixel() function, which directly sets
///       a pixel in the last MEB in the chip.
///       Generally you would NOT want to use this function for simulations.
///@param pos Position of Alpide chip in detector
///@param col Column in Alpide chip pixel matrix
///@param row Row in Alpide chip pixel matrix
void PCTDetector::setPixel(const Detector::DetectorPosition& pos,
                           unsigned int col, unsigned int row)
{
  unsigned int chip_id = PCT_position_to_global_chip_id(pos);

  setPixel(chip_id, col, row);
}


///@brief Set a pixel in one of the detector's Alpide chip's (if it exists in the
///       detector configuration).
///@param h Pixel hit data
void PCTDetector::setPixel(const std::shared_ptr<PixelHit>& p)
{
  // Does the chip exist in our detector/simulation configuration?
  if(mChipVector[p->getChipId()]) {
    ///@todo Check if chip is ready?
    //if(mChipVector[chip_id]->s_chip_ready) {
    //}

    mChipVector[p->getChipId()]->setPixel(p);
  }
}


///@brief SystemC METHOD for distributing triggers to all readout units
void PCTDetector::triggerMethod(void)
{
  int64_t time_now = sc_time_stamp().value();
  std::cout << "@ " << time_now << " ns: \tPCT Detector triggered!" << std::endl;

  for(unsigned int i = 0; i < PCT::N_LAYERS; i++) {
    for(auto RU = mReadoutUnits[i].begin(); RU != mReadoutUnits[i].end(); RU++) {
      RU->E_trigger_in.notify(SC_ZERO_TIME);
    }
  }
}


///@brief Add SystemC signals to log in VCD trace file.
///@param[in,out] wf Pointer to VCD trace file object
///@param[in] name_prefix Name prefix to be added to all the trace names
void PCTDetector::addTraces(sc_trace_file *wf, std::string name_prefix) const
{
  std::stringstream ss;
  ss << name_prefix << "PCT.";
  std::string PCT_name_prefix = ss.str();

  addTrace(wf, PCT_name_prefix, "detector_busy_out", s_detector_busy_out);

  for(unsigned int layer = 0; layer < PCT::N_LAYERS; layer++) {
    for(unsigned int RU_num_in_layer = 0;
        RU_num_in_layer < mReadoutUnits[layer].size();
        RU_num_in_layer++)
    {
      mReadoutUnits[layer][RU_num_in_layer].addTraces(wf, PCT_name_prefix);
    }

    for(unsigned int stave_num_in_layer = 0;
        stave_num_in_layer < mDetectorStaves[layer].size();
        stave_num_in_layer++)
    {
      mDetectorStaves[layer][stave_num_in_layer].addTraces(wf, PCT_name_prefix);
    }
  }
}


///@brief Write simulation stats/data to file
///@param[in] output_path Path to simulation output directory
void PCTDetector::writeSimulationStats(const std::string output_path) const
{
  Detector::writeAlpideStatsToFile(output_path,
                                   mChipVector,
                                   &PCT::PCT_global_chip_id_to_position);

  for(unsigned int layer = 0; layer < PCT::N_LAYERS; layer++) {
    for(unsigned int RU_num_in_layer = 0;
        RU_num_in_layer < mReadoutUnits[layer].size();
        RU_num_in_layer++)
    {
      std::stringstream ss;
      ss << output_path << "/RU_" << layer << "_" << RU_num_in_layer;

      mReadoutUnits[layer][RU_num_in_layer].writeSimulationStats(ss.str());
    }
  }
}
