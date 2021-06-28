/**
 * @file   ITSDetector.cpp
 * @author Simon Voigt Nesbo
 * @date   June 21, 2017
 * @brief  Mockup version of ITS detector.
 *         Accepts trigger input from the dummy CTP module, and communicates the trigger
 *         to the readout units, which will forward them to the Alpide objects.
 */


#include "Detector/ITS/ITSDetector.hpp"
#include "Detector/Common/DetectorSimulationStats.hpp"
#include "Detector/ITS/ITS_creator.hpp"
#include <misc/vcd_trace.hpp>

using namespace ITS;

SC_HAS_PROCESS(ITSDetector);
///@param name SystemC module name
///@param config Configuration of the ITS detector to simulate
///              (ie. number of staves per layer to include in simulation)
///@param trigger_filter_time Readout Units will filter out triggers more closely
///                           spaced than this time (specified in nano seconds).
///@param trigger_filter_enable Enable/disable trigger filtering
///@param data_rate_interval_ns Interval in nanoseconds over which number of data bytes should
///                             be counted, to be used for data rate calculations
ITSDetector::ITSDetector(sc_core::sc_module_name name,
                         const ITSDetectorConfig& config,
                         unsigned int trigger_filter_time,
                         bool trigger_filter_enable,
                         unsigned int data_rate_interval_ns)
  : sc_core::sc_module(name)
  , mReadoutUnits("RU", ITS::N_LAYERS)
  , mDetectorStaves("Stave", ITS::N_LAYERS)
  , mConfig(config)
{
  verifyDetectorConfig(config);
  buildDetector(config, trigger_filter_time, trigger_filter_enable, data_rate_interval_ns);

  SC_METHOD(triggerMethod);
  sensitive << E_trigger_in;
  dont_initialize();
}


///@brief Verify that detector configuration is valid. Exit brutally if not
///@param config Configuration of the ITS detector to simulate
///              (ie. number of staves per layer to include in simulation)
///@throw runtime_error If too many staves specified for a layer, or if a total
///       of zero staves for all layers were specified.
void ITSDetector::verifyDetectorConfig(const ITSDetectorConfig& config) const
{
  unsigned int num_staves_total = 0;

  if(config.num_layers != N_LAYERS) {
    std::string error_msg = "Incorrect number of layers specified for ITS simulation.";
    throw std::runtime_error(error_msg);
  }

  for(unsigned int i = 0; i < N_LAYERS; i++) {
    if(config.layer[i].num_sub_staves_per_full_stave != SUB_STAVES_PER_STAVE[i]) {
      std::string error_msg = "Incorrect number of sub-staves specified for layer " + std::to_string(i);
      throw std::runtime_error(error_msg);
    }

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
///@param trigger_filter_enable Enable/disable trigger filtering
///@param data_rate_interval_ns Interval in nanoseconds over which number of data bytes should
///                             be counted, to be used for data rate calculations
void ITSDetector::buildDetector(const ITSDetectorConfig& config,
                                unsigned int trigger_filter_time,
                                bool trigger_filter_enable,
                                unsigned int data_rate_interval_ns)
{
  for(unsigned int lay_id = 0; lay_id < N_LAYERS; lay_id++) {
    unsigned int num_staves = config.layer[lay_id].num_staves;

    std::cout << "Creating " << num_staves;
    std::cout << " RUs and staves for layer " << lay_id;
    std::cout << std::endl;

    // Create sc_vectors with ReadoutUnit and Staves for this layer
    mReadoutUnits[lay_id].init(num_staves, RUCreator(lay_id,
                                                     trigger_filter_time,
                                                     trigger_filter_enable,
                                                     data_rate_interval_ns));
    mDetectorStaves[lay_id].init(num_staves, StaveCreator(lay_id, mConfig));

    for(unsigned int sta_id = 0; sta_id < config.layer[lay_id].num_staves; sta_id++) {
      // Connect the busy in/out signals for the RUs in a daisy chain
      // ------------------------------------------------------------
      if(sta_id == num_staves-1) {
        // Connect busy input of first RU to busy output of last RU.
        // If only 1 stave/RU was specified, this will actually connect
        // the busy signals in a loopback configuration on the RU.
        mReadoutUnits[lay_id][0].s_busy_in(mReadoutUnits[lay_id][sta_id].s_busy_out);

        std::cout << "Connecting s_busy_in on front RU (" << lay_id << ":" << 0;
        std::cout << " to s_busy_out on back RU (" << lay_id << ":" << sta_id << ")";
        std::cout << std::endl;
      }
      if(sta_id > 0) {
        // Connect busy input of current RU to busy output of previous RU.
        // (Happens only when more than 1 stave/RU is specified).
        mReadoutUnits[lay_id][sta_id].s_busy_in(mReadoutUnits[lay_id][sta_id-1].s_busy_out);

        std::cout << "Connecting s_busy_in on RU " << lay_id<< ":" << sta_id;
        std::cout << " to s_busy_out on RU " << lay_id<< ":" << sta_id-1;
        std::cout << std::endl;
      }

      auto &RU = mReadoutUnits[lay_id][sta_id];
      auto &stave = mDetectorStaves[lay_id][sta_id];

      RU.s_system_clk_in(s_system_clk_in);
      stave.s_system_clk_in(s_system_clk_in);

      for(unsigned int link_num = 0; link_num < stave.numCtrlLinks(); link_num++) {
        RU.s_alpide_control_output[link_num].bind(stave.socket_control_in[link_num]);
      }

      // Get a vector of pointer to the Alpide chips created by the new stave,
      // and add them to a map of chip id vs Alpide chip object.
      auto new_chips = stave.getChips();

      for(unsigned int link_num = 0; link_num < stave.numDataLinks(); link_num++) {
        stave.socket_data_out[link_num].bind(RU.s_alpide_data_input[link_num]);

        if(lay_id < 3) {
          // Inner Barrel
          RU.s_serial_data_input[link_num](new_chips[link_num]->s_serial_data_out_exp);
          RU.s_serial_data_trig_id[link_num](new_chips[link_num]->s_serial_data_trig_id_exp);
        } else {
          // Outer Barrel. Only master chips have data links to RU
          if(new_chips.size() != stave.numDataLinks()*7) {
            throw std::runtime_error("OB stave created with incorrect number of chips.");
          }

          RU.s_serial_data_input[link_num](new_chips[link_num*7]->s_serial_data_out_exp);
          RU.s_serial_data_trig_id[link_num](new_chips[link_num*7]->s_serial_data_trig_id_exp);
        }
      }

      for(auto chip_it = new_chips.begin(); chip_it != new_chips.end(); chip_it++) {
        unsigned int chip_id = (*chip_it)->getGlobalChipId();
        // Don't allow more than one instance of the same Chip ID
        if(mChipMap.find(chip_id) != mChipMap.end()) {
          std::string error_msg = "Chip with ID ";
          error_msg += std::to_string(chip_id);
          error_msg += " created more than once..";

          throw std::runtime_error(error_msg);
        }

        mChipMap[chip_id] = *chip_it;
        mNumChips++;
      }
    }
  }
}


///@brief Input a pixel to the front end of one of the detector's
///       Alpide chip's (if it exists in the detector configuration).
///@param pix PixelHit object with pixel matrix coordinates and chip id
void ITSDetector::pixelInput(const std::shared_ptr<PixelHit>& pix)
{
  // Does the chip exist in our detector/simulation configuration?
  if(mChipMap.find(pix->getChipId()) != mChipMap.end()) {
    mChipMap[pix->getChipId()]->pixelFrontEndInput(pix);
  } else {
    std::cout << "Chip " << pix->getChipId() << " does not exist." << std::endl;
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
void ITSDetector::setPixel(unsigned int chip_id, unsigned int col, unsigned int row)
{
  // Does the chip exist in our detector/simulation configuration?
  if(mChipMap.find(chip_id) != mChipMap.end()) {
    mChipMap[chip_id]->setPixel(col, row);
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
void ITSDetector::setPixel(const Detector::DetectorPosition& pos,
                           unsigned int col, unsigned int row)
{
  unsigned int chip_id = ITS_position_to_global_chip_id(pos);

  setPixel(chip_id, col, row);
}


///@brief Set a pixel in one of the detector's Alpide chip's (if it exists in the
///       detector configuration).
///@param h Pixel hit data
void ITSDetector::setPixel(const std::shared_ptr<PixelHit>& p)
{
  // Does the chip exist in our detector/simulation configuration?
  if(mChipMap.find(p->getChipId()) != mChipMap.end()) {
    mChipMap[p->getChipId()]->setPixel(p);
  }
}


///@brief SystemC METHOD for distributing triggers to all readout units
void ITSDetector::triggerMethod(void)
{
  int64_t time_now = sc_time_stamp().value();
  std::cout << "@ " << time_now << " ns: \tITS Detector triggered!" << std::endl;

  for(unsigned int i = 0; i < N_LAYERS; i++) {
    for(auto RU = mReadoutUnits[i].begin(); RU != mReadoutUnits[i].end(); RU++) {
      RU->E_trigger_in.notify(SC_ZERO_TIME);
    }
  }
}


///@brief Add SystemC signals to log in VCD trace file.
///@param[in,out] wf Pointer to VCD trace file object
///@param[in] name_prefix Name prefix to be added to all the trace names
void ITSDetector::addTraces(sc_trace_file *wf, std::string name_prefix) const
{
  std::stringstream ss;
  ss << name_prefix << "ITS.";
  std::string ITS_name_prefix = ss.str();

  addTrace(wf, ITS_name_prefix, "detector_busy_out", s_detector_busy_out);

  for(unsigned int layer = 0; layer < ITS::N_LAYERS; layer++) {
    // mReadoutUnits[layer] and mDetectorStaves[layer] should
    // always be the same size.
    for(unsigned int stave = 0; stave < mReadoutUnits[layer].size(); stave++) {
      mReadoutUnits[layer][stave].addTraces(wf, ITS_name_prefix);
      mDetectorStaves[layer][stave].addTraces(wf, ITS_name_prefix);
    }
  }
}


///@brief Write simulation stats/data to file
///@param[in] output_path Path to simulation output directory
void ITSDetector::writeSimulationStats(const std::string output_path) const
{
  Detector::writeAlpideStatsToFile(output_path,
                                   mChipMap,
                                   &ITS::ITS_global_chip_id_to_position);

  for(unsigned int layer = 0; layer < N_LAYERS; layer++) {
    for(unsigned int stave = 0; stave < mDetectorStaves[layer].size(); stave++){
      std::stringstream ss;
      ss << output_path << "/RU_" << layer << "_" << stave;

      mReadoutUnits[layer][stave].writeSimulationStats(ss.str());
    }
  }

  ///@todo More ITS/RU stats here..
}
