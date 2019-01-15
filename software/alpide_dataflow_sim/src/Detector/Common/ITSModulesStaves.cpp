/**
 * @file   ITSModulesStaves.cpp
 * @author Simon Voigt Nesbo
 * @date   August 15, 2017
 * @brief  This file holds a collection of classes and structs that define modules,
 *         staves and barrels/layers in the ITS detector.
 *         Much of this code is copy-paste from AlpideModule.hpp/cpp in
 *         Matthias Bonora's WP10 RU SystemC simulation code.
 */

#include "ITSModulesStaves.hpp"
#include <misc/vcd_trace.hpp>

using namespace ITS;
//using std::placeholders::_1;

template class MBOBStave<HALF_MODULES_PER_MB_STAVE>;
template class MBOBStave<HALF_MODULES_PER_OB_STAVE>;

SingleChip::SingleChip(sc_core::sc_module_name const &name,
                       int chip_id, AlpideConfig const &chip_cfg)
  : StaveInterface(name, 0, 0, 1, 1)
{
  socket_control_in[0].register_transport(
    std::bind(&SingleChip::processCommand, this, std::placeholders::_1));

  mChip = std::make_shared<Alpide>("Alpide",
                                   chip_id,
                                   chip_cfg);

    socket_control_out.bind(mChip->s_control_input);
    mChip->s_data_output(socket_data_out[0]);
    mChip->s_system_clk_in(s_system_clk_in);
    s_alpide_data_out_exp(mChip->s_serial_data_out_exp);
}


ControlResponsePayload
SingleChip::processCommand(const ControlRequestPayload &request) {
  ControlResponsePayload b = socket_control_out->transport(request);
  return b;
}


///@brief Set a pixel in the Alpide chip
///@param h Pixel hit
void SingleChip::pixelInput(const std::shared_ptr<PixelHit>& p)
{
  mChip->pixelFrontEndInput(p);
}


///@brief Add SystemC signals to log in VCD trace file.
///@param[in,out] wf Pointer to VCD trace file object
///@param[in] name_prefix Name prefix to be added to all the trace names
void SingleChip::addTraces(sc_trace_file *wf, std::string name_prefix) const
{
  std::stringstream ss;
  ss << name_prefix << "SingleChip";
  std::string single_chip_name_prefix = ss.str();

  mChip->addTraces(wf, single_chip_name_prefix);
}


///@brief Create an inner barrel stave object with 9 chips
///@param name SystemC module name
///@param pos Detector position with layer and stave information. The other position
///           parameters (sub stave, module id, chip id) are ignored, as chips are generated
///           for all the possible sub-positions by this function.
///@param position_to_global_chip_id_func Pointer to function used to determine position
///                                       based on global chip id
///@param chip_cfg Alpide chip config passed to Alpide constructor
InnerBarrelStave::InnerBarrelStave(sc_core::sc_module_name const &name,
                                   Detector::DetectorPosition& pos,
                                   Detector::t_position_to_global_chip_id_func position_to_global_chip_id_func,
                                   const AlpideConfig& chip_cfg)
  : StaveInterface(name, pos.layer_id, pos.stave_id, 1, 9)
{
  socket_control_in[0].register_transport(
    std::bind(&InnerBarrelStave::processCommand, this, std::placeholders::_1));

  for (unsigned int i = 0; i < 9; i++) {
    pos.module_chip_id = i;

    unsigned int global_chip_id = (*position_to_global_chip_id_func)(pos);
    std::string chip_name = "Chip_" + std::to_string(global_chip_id);

    std::cout << "Creating chip with global ID " << global_chip_id;
    std::cout << ", layer " << pos.layer_id << ", stave " << pos.stave_id << std::endl;

    mChips.push_back(std::make_shared<Alpide>(chip_name.c_str(),
                                              global_chip_id,
                                              chip_cfg));

    auto &chip = *mChips.back();
    socket_control_out[i].bind(chip.s_control_input);
    chip.s_data_output(socket_data_out[i]);
    chip.s_system_clk_in(s_system_clk_in);
  }
}


ControlResponsePayload
InnerBarrelStave::processCommand(const ControlRequestPayload &request) {
  ControlResponsePayload b;
  // SC_REPORT_INFO_VERB(name(), "Received Command", sc_core::SC_DEBUG);
  for (size_t i = 0; i < socket_control_out.size(); ++i) {
    auto result = socket_control_out[i]->transport(request);
    if (request.chipId == i)
      b = result;
  }

  return b;
}


///@brief Add SystemC signals to log in VCD trace file.
///@param[in,out] wf Pointer to VCD trace file object
///@param[in] name_prefix Name prefix to be added to all the trace names
void InnerBarrelStave::addTraces(sc_trace_file *wf, std::string name_prefix) const
{
  std::stringstream ss;
  ss << name_prefix << "IB_" << getLayerId() << "_" << getStaveId() << ".";
  std::string IB_stave_name_prefix = ss.str();

  //addTrace(wf, IB_stave_name_prefix, "socket_control_in", socket_control_in);
  //addTrace(wf, IB_stave_name_prefix, "socket_data_out", socket_data_out);

  for(unsigned int i = 0; i < mChips.size(); i++) {
    std::stringstream ss_chip;
    ss_chip << IB_stave_name_prefix << "Chip_" << i << ".";
    std::string IB_stave_chip_prefix = ss_chip.str();
    mChips[i]->addTraces(wf, IB_stave_chip_prefix);
  }
}


///@brief Constructor for outer/middle barrel half module
///@param name SystemC module name
///@param pos DetectorPositionBase object with position information. The module_chip_id member
///           in this struct is ignored, as all 7 chips for the half module are generated by
///           this function.
///@param position_to_global_chip_id_func Pointer to function used to determine position
///                                       based on global chip id
///@param half_mod_id Half module ID (0 or 1)
///@param cfg Alpide chip config passed to Alpide constructor
HalfModule::HalfModule(sc_core::sc_module_name const &name,
                       Detector::DetectorPosition& pos,
                       Detector::t_position_to_global_chip_id_func position_to_global_chip_id_func,
                       unsigned int half_mod_id,
                       const AlpideConfig& cfg)
  : sc_module(name)
{
  socket_control_in.register_transport(std::bind(&HalfModule::processCommand,
                                                 this, std::placeholders::_1));

  pos.module_chip_id = ITS::CHIPS_PER_HALF_MODULE*half_mod_id;

  // Create OB master chip
  unsigned int global_chip_id = (*position_to_global_chip_id_func)(pos);
  std::string chip_name = "Chip_" + std::to_string(global_chip_id);
  std::cout << "Creating chip with global ID " << global_chip_id;
  std::cout << ", layer " << pos.layer_id << ", stave " << pos.stave_id;
  std::cout << ", module " << pos.module_id << ", half-mod " << half_mod_id << std::endl;

  mChips.push_back(std::make_shared<Alpide>(chip_name.c_str(),
                                            global_chip_id,
                                            cfg,
                                            true, // Outer barrel mode
                                            true, // Outer barrel master
                                            6));  // 6 outer barrel slaves

  auto &master_chip = *mChips.back();
  master_chip.s_system_clk_in(s_system_clk_in);
  master_chip.s_data_output(socket_data_out);
  socket_control_out[0].bind(master_chip.s_control_input);

  pos.module_chip_id++;

  // Create slave chips
  for(unsigned int i = 0; i < 6; i++, pos.module_chip_id++) {
    global_chip_id = (*position_to_global_chip_id_func)(pos);

    std::string chip_name = "Chip_" + std::to_string(global_chip_id);

    std::cout << "Creating chip with global ID " << global_chip_id;
    std::cout << ", layer " << pos.layer_id << ", stave " << pos.stave_id;
    std::cout << ", module " << pos.module_id << ", half-mod " << half_mod_id << std::endl;

    mChips.push_back(std::make_shared<Alpide>(chip_name.c_str(),
                                              global_chip_id,
                                              cfg,
                                              true, // Outer barrel mode
                                              false)); // Outer barrel slave



    auto &chip = *mChips.back();
    chip.s_system_clk_in(s_system_clk_in);
    socket_control_out[i+1].bind(chip.s_control_input);

    // Connect data and busy to master chip
    master_chip.s_local_busy_in[i](chip.s_local_busy_out);
    master_chip.s_local_bus_data_in[i](chip.s_local_bus_data_out);
  }
}


ControlResponsePayload
HalfModule::processCommand(const ControlRequestPayload &request) {
  ControlResponsePayload b;
  // SC_REPORT_INFO_VERB(name(), "Received Command", sc_core::SC_DEBUG);
  for (size_t i = 0; i < socket_control_out.size(); ++i) {
    auto result = socket_control_out[i]->transport(request);
    if (request.chipId == i)
      b = result;
  }

  return b;
}


///@brief Add SystemC signals to log in VCD trace file.
///@param[in,out] wf Pointer to VCD trace file object
///@param[in] name_prefix Name prefix to be added to all the trace names
void HalfModule::addTraces(sc_trace_file *wf, std::string name_prefix) const
{
  for(unsigned int i = 0; i < mChips.size(); i++) {
    std::stringstream ss_chip;
    ss_chip << name_prefix << "Chip_" << i << ".";
    std::string chip_prefix = ss_chip.str();
    mChips[i]->addTraces(wf, chip_prefix);
  }
}


///@brief Create an outer/middle barrel stave object
///@param name SystemC module name
///@param pos DetectorPositionBase object with layer and stave information. The other position
///           parameters (sub stave, module id, chip id) are ignored, as chips are generated
///           for all the possible sub-positions by this function.
///@param position_to_global_chip_id_func Pointer to function used to determine position
///                                       based on global chip id
///@param cfg Detector configuration object
template <int N_HALF_MODULES>
MBOBStave<N_HALF_MODULES>::MBOBStave(sc_core::sc_module_name const &name,
                                     Detector::DetectorPosition& pos,
                                     Detector::t_position_to_global_chip_id_func position_to_global_chip_id_func,
                                     const Detector::DetectorConfigBase& cfg)
  : StaveInterface(name, pos.layer_id, pos.stave_id, N_HALF_MODULES, N_HALF_MODULES)
{
  unsigned int num_sub_staves = cfg.layer[pos.layer_id].num_sub_staves_per_full_stave;

  for(pos.sub_stave_id = 0;
      pos.sub_stave_id < num_sub_staves;
      pos.sub_stave_id++)
  {
    // Create half of the half modules for one sub stave, and half for other sub stave
    // In hindsight it would have made more sense to have a Module object instead of creating
    // two HalfModule objects, since it got pretty complicated with the indexes and positions here..
    for (unsigned int i = 0; i < N_HALF_MODULES/2; i++) {
      pos.module_id = i/2;
      unsigned int half_mod_id = i%2;

      std::string half_mod_name = "HalfMod_";
      half_mod_name += std::to_string(pos.layer_id) + ":";
      half_mod_name += std::to_string(pos.stave_id) + ":";
      half_mod_name += std::to_string(pos.sub_stave_id) + ":";
      half_mod_name += std::to_string(pos.module_id) + ":";
      half_mod_name += std::to_string(half_mod_id);

      std::cout << "Creating: " << half_mod_name << std::endl;

      mHalfModules.emplace_back(std::make_shared<HalfModule>(half_mod_name.c_str(),
                                                             pos,
                                                             position_to_global_chip_id_func,
                                                             half_mod_id,
                                                             cfg.chip_cfg));

      // Account for modules already created for first sub stave when
      // calculating indexes in vectors here..
      unsigned int mod_index = i + (pos.sub_stave_id*(N_HALF_MODULES/2));

      mHalfModules[mod_index]->s_system_clk_in(s_system_clk_in);

      // Bind incoming control sockets to processCommand() in respective HalfModule objects
      socket_control_in[mod_index].register_transport(std::bind(&HalfModule::processCommand,
                                                                mHalfModules[mod_index],
                                                                std::placeholders::_1));

      // Forward data from HalfModule object to StaveInterface
      mHalfModules[mod_index]->socket_data_out(socket_data_out[mod_index]);
    }
  }
}


///@brief Add SystemC signals to log in VCD trace file.
///@param[in,out] wf Pointer to VCD trace file object
///@param[in] name_prefix Name prefix to be added to all the trace names
template <int N_HALF_MODULES>
void MBOBStave<N_HALF_MODULES>::addTraces(sc_trace_file *wf, std::string name_prefix) const
{
  std::stringstream ss;
  ss << name_prefix << "OB_" << getLayerId() << "_" << getStaveId() << ".";
  std::string OB_stave_name_prefix = ss.str();

  for(unsigned int i = 0; i < mHalfModules.size(); i++) {
    std::stringstream ss_half_mod;
    ss_half_mod << OB_stave_name_prefix << "Half_Mod_" << i << ".";
    std::string half_mod_prefix = ss_half_mod.str();
    mHalfModules[i]->addTraces(wf, half_mod_prefix);
  }
}

template <int N_HALF_MODULES>
std::vector<std::shared_ptr<Alpide>> MBOBStave<N_HALF_MODULES>::getChips(void) const
{
  std::vector<std::shared_ptr<Alpide>> chips;

  for(auto mod_it = mHalfModules.begin(); mod_it != mHalfModules.end(); mod_it++) {
    auto mod_chips = (*mod_it)->getChips();
    chips.insert(chips.end(), mod_chips.begin(), mod_chips.end());
  }

  return chips;
}
