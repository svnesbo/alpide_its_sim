/**
 * @file   FocalStaves.hpp
 * @author Simon Voigt Nesbo
 * @date   December 10, 2019
 * @brief  Classes for Focal staves
 */

#ifndef FOCAL_STAVES_HPP
#define FOCAL_STAVES_HPP

#include <Alpide/Alpide.hpp>
#include <Alpide/AlpideInterface.hpp>
#include "Detector/Common/DetectorConfig.hpp"
#include "Detector/Common/ITSModulesStaves.hpp"
#include "Detector/Focal/Focal_constants.hpp"
#include "Detector/Focal/FocalDetectorConfig.hpp"


namespace Focal {

  ///@brief A module for Focal consisting of 8 chips in IB configuration
  struct FocalIbModule : public sc_module
  {
    FocalIbModule(sc_core::sc_module_name const &name,
                  Detector::DetectorPosition pos,
                  Detector::t_position_to_global_chip_id_func position_to_global_chip_id_func,
                  const AlpideConfig& cfg);

    void addTraces(sc_trace_file *wf, std::string name_prefix) const;

    sc_in_clk s_system_clk_in;
    ControlTargetSocket socket_control_in;                                 // IB module control in
    DataInitiatorSocket socket_data_out[Focal::CHIPS_PER_FOCAL_IB_MODULE]; // IB module data out

    std::vector<std::shared_ptr<Alpide>> getChips(void) const {
      return mChips;
    }

    ControlResponsePayload processCommand(ControlRequestPayload const &request);

  private:
    std::vector<std::shared_ptr<Alpide>> mChips;

    // Distribution of ctrl to individual chips in half-module
    std::array<ControlInitiatorSocket, Focal::CHIPS_PER_FOCAL_IB_MODULE> socket_control_out;
  };


  ///@brief A module for Focal consisting of 5 chips in OB configuration
  struct FocalObModule : public sc_module
  {
    FocalObModule(sc_core::sc_module_name const &name,
                  Detector::DetectorPosition pos,
                  Detector::t_position_to_global_chip_id_func position_to_global_chip_id_func,
                  const AlpideConfig& cfg);

    void addTraces(sc_trace_file *wf, std::string name_prefix) const;

    sc_in_clk s_system_clk_in;
    ControlTargetSocket socket_control_in; // Half-module control in
    DataInitiatorSocket socket_data_out;   // Half-module data out

    std::vector<std::shared_ptr<Alpide>> getChips(void) const {
      return mChips;
    }

    ControlResponsePayload processCommand(ControlRequestPayload const &request);

  private:
    std::vector<std::shared_ptr<Alpide>> mChips;

    // Distribution of ctrl in half-module
    std::array<ControlInitiatorSocket, Focal::CHIPS_PER_FOCAL_OB_MODULE> socket_control_out;
  };


  ///@brief Focal stave consisting of 15 chips in total:
  ///       1x "Focal IB module" (8 IB chips) + 1x ITS OB half-module (7 OB chips)
  struct FocalInnerStave : public ITS::StaveInterface
  {
    FocalInnerStave(sc_core::sc_module_name const &name,
                    Detector::DetectorPosition pos,
                    Detector::t_position_to_global_chip_id_func position_to_global_chip_id_func,
                    const Detector::DetectorConfigBase& cfg);
    void addTraces(sc_trace_file *wf, std::string name_prefix) const;
    std::vector<std::shared_ptr<Alpide>> getChips(void) const;

  private:
    std::shared_ptr<Focal::FocalIbModule> mIbModule;
    std::shared_ptr<ITS::HalfModule> mObModule;
  };

  ///@brief Focal stave consisting of 15 chips in total:
  ///       3x Focal OB modules of five 5 OB chips each,
  ///       that is: 5 + 5 + 5 OB chips
  struct FocalOuterStave : public ITS::StaveInterface
  {
    FocalOuterStave(sc_core::sc_module_name const &name,
                    Detector::DetectorPosition pos,
                    Detector::t_position_to_global_chip_id_func position_to_global_chip_id_func,
                    const Detector::DetectorConfigBase& cfg);
    void addTraces(sc_trace_file *wf, std::string name_prefix) const;
    std::vector<std::shared_ptr<Alpide>> getChips(void) const;

  private:
    std::shared_ptr<Focal::FocalObModule> mObModules[MODULES_PER_OUTER_STAVE];
  };

}

#endif
