/**
 * @file   FocalStaves.cpp
 * @author Simon Voigt Nesbo
 * @date   December 10, 2019
 * @brief  Classes for Focal staves
 */

#include "FocalStaves.hpp"
#include <misc/vcd_trace.hpp>

using namespace ITS;
using namespace Focal;

///@brief Constructor for Focal IB module (8 IB chips)
///@param name SystemC module name
///@param pos DetectorPosition object with position information.
///@param position_to_global_chip_id_func Pointer to function used to determine position
///                                       based on global chip id
///@param cfg Alpide chip config passed to Alpide constructor
FocalIbModule::FocalIbModule(sc_core::sc_module_name const &name,
                             Detector::DetectorPosition pos,
                             Detector::t_position_to_global_chip_id_func position_to_global_chip_id_func,
                             const AlpideConfig& cfg)
  : sc_module(name)
{
  socket_control_in.register_transport(std::bind(&FocalIbModule::processCommand,
                                                 this, std::placeholders::_1));

  // Create chips
  for(unsigned int i = 0; i < Focal::CHIPS_PER_FOCAL_IB_MODULE; i++) {
    pos.module_chip_id = i;

    unsigned int global_chip_id = (*position_to_global_chip_id_func)(pos);

    std::string chip_name = "Chip_" + std::to_string(global_chip_id);
    std::cout << "Creating chip with global ID " << global_chip_id;
    std::cout << ", layer " << pos.layer_id << ", stave " << pos.stave_id;
    std::cout << ", module " << pos.module_id;
    std::cout << ", module chip id " << pos.module_chip_id << std::endl;

    mChips.push_back(std::make_shared<Alpide>(chip_name.c_str(),
                                              global_chip_id,
                                              pos.module_chip_id,
                                              cfg,
                                              false)); // Inner barrel mode

    auto &chip = *mChips.back();
    chip.s_system_clk_in(s_system_clk_in);
    chip.s_data_output(socket_data_out[i]);
    socket_control_out[i].bind(chip.s_control_input);
  }
}


ControlResponsePayload
FocalIbModule::processCommand(const ControlRequestPayload &request) {
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
void FocalIbModule::addTraces(sc_trace_file *wf, std::string name_prefix) const
{
  for(unsigned int i = 0; i < mChips.size(); i++) {
    std::stringstream ss_chip;
    ss_chip << name_prefix << "Chip_" << i << ".";
    std::string chip_prefix = ss_chip.str();
    mChips[i]->addTraces(wf, chip_prefix);
  }
}


///@brief Constructor for Focal OB module (5 OB chips)
///@param name SystemC module name
///@param pos DetectorPosition object with position information.
///@param position_to_global_chip_id_func Pointer to function used to determine position
///                                       based on global chip id
///@param cfg Alpide chip config passed to Alpide constructor
FocalObModule::FocalObModule(sc_core::sc_module_name const &name,
                             Detector::DetectorPosition pos,
                             Detector::t_position_to_global_chip_id_func position_to_global_chip_id_func,
                             const AlpideConfig& cfg)
  : sc_module(name)
{
  socket_control_in.register_transport(std::bind(&FocalObModule::processCommand,
                                                 this, std::placeholders::_1));

  // Create OB master chip first
  pos.module_chip_id = 0;
  unsigned int global_chip_id = (*position_to_global_chip_id_func)(pos);

  std::string chip_name = "Chip_" + std::to_string(global_chip_id);
  std::cout << "Creating chip with global ID " << global_chip_id;
  std::cout << ", layer " << pos.layer_id << ", stave " << pos.stave_id;
  std::cout << ", module " << pos.module_id;
  std::cout << ", module chip id " << pos.module_chip_id << std::endl;

  mChips.push_back(std::make_shared<Alpide>(chip_name.c_str(),
                                            global_chip_id,
                                            pos.module_chip_id,
                                            cfg,
                                            true, // Outer barrel mode
                                            true, // Outer barrel master
                                            Focal::CHIPS_PER_FOCAL_OB_MODULE-1)); // number of
                                                                                  // slave chips

  auto &master_chip = *mChips.back();
  master_chip.s_system_clk_in(s_system_clk_in);
  master_chip.s_data_output(socket_data_out);
  socket_control_out[0].bind(master_chip.s_control_input);


  // Create slave chips
  for(unsigned int i = 0; i < Focal::CHIPS_PER_FOCAL_OB_MODULE-1; i++) {
    pos.module_chip_id = i+1;

    unsigned int global_chip_id = (*position_to_global_chip_id_func)(pos);

    std::string chip_name = "Chip_" + std::to_string(global_chip_id);
    std::cout << "Creating chip with global ID " << global_chip_id;
    std::cout << ", layer " << pos.layer_id << ", stave " << pos.stave_id;
    std::cout << ", module " << pos.module_id;
    std::cout << ", module chip id " << pos.module_chip_id << std::endl;

    mChips.push_back(std::make_shared<Alpide>(chip_name.c_str(),
                                              global_chip_id,
                                              pos.module_chip_id,
                                              cfg,
                                              true,    // Outer barrel mode
                                              false)); // Inner barrel slave

    auto &chip = *mChips.back();
    chip.s_system_clk_in(s_system_clk_in);
    socket_control_out[i+1].bind(chip.s_control_input);

    // Connect data and busy to master chip
    master_chip.s_local_busy_in[i](chip.s_local_busy_out);
    master_chip.s_local_bus_data_in[i](chip.s_local_bus_data_out);
  }
}


ControlResponsePayload
FocalObModule::processCommand(const ControlRequestPayload &request) {
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
void FocalObModule::addTraces(sc_trace_file *wf, std::string name_prefix) const
{
  for(unsigned int i = 0; i < mChips.size(); i++) {
    std::stringstream ss_chip;
    ss_chip << name_prefix << "Chip_" << i << ".";
    std::string chip_prefix = ss_chip.str();
    mChips[i]->addTraces(wf, chip_prefix);
  }
}


///@brief Create an "inner stave" object for Focal. Focal inner stave consists of 15 chips in total:
///       1x IB chip + 1x IB "module" (7 IB chips) + 1x OB half-module
///@param name SystemC module name
///@param pos DetectorPositionBase object with layer and stave information. The other position
///           parameters (sub stave, module id, chip id) are ignored, as chips are generated
///           for all the possible sub-positions by this function.
///@param position_to_global_chip_id_func Pointer to function used to determine position
///                                       based on global chip id
///@param cfg Detector configuration object
FocalInnerStave::FocalInnerStave(sc_core::sc_module_name const &name,
                                 Detector::DetectorPosition pos,
                                 Detector::t_position_to_global_chip_id_func position_to_global_chip_id_func,
                                 const Detector::DetectorConfigBase& cfg)
  : StaveInterface(name, pos.layer_id, pos.stave_id,
                   Focal::CTRL_LINKS_PER_INNER_STAVE, Focal::DATA_LINKS_PER_INNER_STAVE)
{
  pos.sub_stave_id = 0;
  pos.module_id = 0;

  //----------------------------------------------------------------------------
  // Create first module ("IB module")
  //----------------------------------------------------------------------------
  std::string mod_name = "Mod_";
  mod_name += std::to_string(pos.layer_id) + ":";
  mod_name += std::to_string(pos.stave_id) + ":";
  mod_name += std::to_string(pos.module_id);

  std::cout << "Creating: " << mod_name << std::endl;

  mIbModule = std::make_shared<FocalIbModule>(mod_name.c_str(),
                                              pos,
                                              position_to_global_chip_id_func,
                                              cfg.chip_cfg);

  mIbModule->s_system_clk_in(s_system_clk_in);

  // Bind incoming control sockets to processCommand() in FocalIbModule object
  // There is only one control link in this stave
  socket_control_in[0].register_transport(std::bind(&FocalIbModule::processCommand,
                                                    mIbModule,
                                                    std::placeholders::_1));

  // Forward data from chips in FocalIbModule object to StaveInterface
  for(unsigned int link_id = 0; link_id < Focal::CHIPS_PER_FOCAL_IB_MODULE; link_id++)
    mIbModule->socket_data_out[link_id](socket_data_out[link_id]);


  //----------------------------------------------------------------------------
  // Create second module (OB half-module)
  //----------------------------------------------------------------------------
  pos.module_id++;
  mod_name = "Mod_";
  mod_name += std::to_string(pos.layer_id) + ":";
  mod_name += std::to_string(pos.stave_id) + ":";
  mod_name += std::to_string(pos.module_id);

  std::cout << "Creating: " << mod_name << std::endl;

  mObModule = std::make_shared<HalfModule>(mod_name.c_str(),
                                           pos,
                                           position_to_global_chip_id_func,
                                           0, // half-mod not used for Focal sim..
                                           cfg.chip_cfg);

  mObModule->s_system_clk_in(s_system_clk_in);

  // Bind incoming control sockets to processCommand() in HalfModule object
  //  Note: There is only one control link in this stave
  socket_control_in[1].register_transport(std::bind(&HalfModule::processCommand,
                                                    mObModule,
                                                    std::placeholders::_1));

  // Forward data from HalfModule object to StaveInterface
  mObModule->socket_data_out(socket_data_out[Focal::CHIPS_PER_FOCAL_IB_MODULE]);
}


///@brief Add SystemC signals to log in VCD trace file.
///@param[in,out] wf Pointer to VCD trace file object
///@param[in] name_prefix Name prefix to be added to all the trace names
void FocalInnerStave::addTraces(sc_trace_file *wf, std::string name_prefix) const
{
  std::stringstream ss;
  ss << name_prefix << "FIbS_" << getLayerId() << "_" << getStaveId() << ".";
  std::string FIbS_stave_name_prefix = ss.str();

  for(unsigned int i = 0; i < 2; i++) {
    std::stringstream ss_mod;
    ss_mod << FIbS_stave_name_prefix << "Mod_" << i << ".";
    std::string mod_prefix = ss_mod.str();

    if(i==0)
      mIbModule->addTraces(wf, mod_prefix);
    else
      mObModule->addTraces(wf, mod_prefix);
  }
}

std::vector<std::shared_ptr<Alpide>> FocalInnerStave::getChips(void) const
{
  std::vector<std::shared_ptr<Alpide>> chips;

  auto ib_mod_chips = mIbModule->getChips();
  chips.insert(chips.end(), ib_mod_chips.begin(), ib_mod_chips.end());

  auto ob_mod_chips = mObModule->getChips();
  chips.insert(chips.end(), ob_mod_chips.begin(), ob_mod_chips.end());

  return chips;
}


///@brief Create an "outer stave" object for Focal. Focal outer stave consists of 15 chips in total:
///       1x IB chip + 2x OB half-module (14 OB chips)
///@param name SystemC module name
///@param pos DetectorPositionBase object with layer and stave information. The other position
///           parameters (sub stave, module id, chip id) are ignored, as chips are generated
///           for all the possible sub-positions by this function.
///@param position_to_global_chip_id_func Pointer to function used to determine position
///                                       based on global chip id
///@param cfg Detector configuration object
FocalOuterStave::FocalOuterStave(sc_core::sc_module_name const &name,
                                 Detector::DetectorPosition pos,
                                 Detector::t_position_to_global_chip_id_func position_to_global_chip_id_func,
                                 const Detector::DetectorConfigBase& cfg)
  : StaveInterface(name, pos.layer_id, pos.stave_id,
                   Focal::CTRL_LINKS_PER_OUTER_STAVE, Focal::DATA_LINKS_PER_OUTER_STAVE)
{
  unsigned int num_sub_staves = cfg.layer[pos.layer_id].num_sub_staves_per_full_stave;

  for(pos.sub_stave_id = 0;
      pos.sub_stave_id < num_sub_staves;
      pos.sub_stave_id++)
  {
    // Create half of the half modules for one sub stave, and half for other sub stave
    // In hindsight it would have made more sense to have a Module object instead of creating
    // two HalfModule objects, since it got pretty complicated with the indexes and positions here..
    for (unsigned int i = 0; i < MODULES_PER_OUTER_STAVE; i++) {
      pos.module_id = i;

      std::string mod_name = "Mod_";
      mod_name += std::to_string(pos.layer_id) + ":";
      mod_name += std::to_string(pos.stave_id) + ":";
      mod_name += std::to_string(pos.module_id);

      std::cout << "Creating: " << mod_name << std::endl;

      mObModules[i] = std::make_shared<FocalObModule>(mod_name.c_str(),
                                                      pos,
                                                      position_to_global_chip_id_func,
                                                      cfg.chip_cfg);

      mObModules[i]->s_system_clk_in(s_system_clk_in);

      // Bind incoming control sockets to processCommand() in respective FocalObModule objects
      // Note: There is only one control link in this stave
      socket_control_in[i].register_transport(std::bind(&FocalObModule::processCommand,
                                                        mObModules[i],
                                                        std::placeholders::_1));

      // Forward data from HalfModule object to StaveInterface
      mObModules[i]->socket_data_out(socket_data_out[i]);
    }
  }
}


///@brief Add SystemC signals to log in VCD trace file.
///@param[in,out] wf Pointer to VCD trace file object
///@param[in] name_prefix Name prefix to be added to all the trace names
void FocalOuterStave::addTraces(sc_trace_file *wf, std::string name_prefix) const
{
  std::stringstream ss;
  ss << name_prefix << "FOS_" << getLayerId() << "_" << getStaveId() << ".";
  std::string FOS_stave_name_prefix = ss.str();

  for(unsigned int i = 0; i < MODULES_PER_OUTER_STAVE; i++) {
    std::stringstream ss_mod;
    ss_mod << FOS_stave_name_prefix << "Mod_" << i << ".";
    std::string mod_prefix = ss_mod.str();
    mObModules[i]->addTraces(wf, mod_prefix);
  }
}

std::vector<std::shared_ptr<Alpide>> FocalOuterStave::getChips(void) const
{
  std::vector<std::shared_ptr<Alpide>> chips;

  for(unsigned int mod = 0; mod < MODULES_PER_OUTER_STAVE; mod++) {
    auto mod_chips = mObModules[mod]->getChips();
    chips.insert(chips.end(), mod_chips.begin(), mod_chips.end());
  }

  return chips;
}
