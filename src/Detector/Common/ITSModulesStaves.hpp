/**
 * @file   ITSModulesStaves.hpp
 * @author Simon Voigt Nesbo
 * @date   July 11, 2017
 * @brief  This file holds a collection of classes and structs that define modules,
 *         staves and barrels/layers in the ITS detector.
 *         Obviously there are some dependencies to ITS specific files here, but the classes
 *         here can be used to define other detectors as well
 *         Much of this code is copy-paste from AlpideModule.hpp/cpp in
 *         Matthias Bonora's WP10 RU SystemC simulation code.
 */

#ifndef ITS_MODULES_STAVES_HPP
#define ITS_MODULES_STAVES_HPP

#include <Alpide/Alpide.hpp>
#include <Alpide/AlpideInterface.hpp>
#include "DetectorConfig.hpp"
#include "Detector/ITS/ITS_constants.hpp"


namespace ITS {
  struct StaveInterface : public sc_module {
    std::vector<ControlTargetSocket> socket_control_in;
    std::vector<DataInitiatorSocket> socket_data_out;
    sc_in_clk s_system_clk_in;

    StaveInterface(sc_core::sc_module_name const &name,
                   unsigned int layer_id,
                   unsigned int stave_id,
                   unsigned int n_ctrl_links,
                   unsigned int n_data_links)
      : sc_module(name)
      , socket_control_in(n_ctrl_links)
      , socket_data_out(n_data_links)
      , mLayerId(layer_id)
      , mStaveId(stave_id)
      {}

    virtual std::vector<std::shared_ptr<Alpide>> getChips(void) const = 0;
    virtual void addTraces(sc_trace_file *wf, std::string name_prefix) const = 0;

    unsigned int getStaveId(void) const { return mStaveId; }
    unsigned int getLayerId(void) const { return mLayerId; }
    unsigned int numCtrlLinks(void) const { return socket_control_in.size(); }
    unsigned int numDataLinks(void) const { return socket_data_out.size(); }

  private:
    unsigned int mLayerId;
    unsigned int mStaveId;
  };


  struct SingleChip : public StaveInterface
  {
    sc_export<sc_signal<sc_uint<24>>> s_alpide_data_out_exp;
    sc_export<sc_signal<uint64_t>> s_serial_data_trig_id_exp;

    SingleChip(sc_core::sc_module_name const &name,
               int chip_id, AlpideConfig const &chip_cfg);

    virtual void addTraces(sc_trace_file *wf, std::string name_prefix) const;

    virtual std::vector<std::shared_ptr<Alpide>> getChips(void) const
      {
        std::vector<std::shared_ptr<Alpide>> vec;

        vec.push_back(mChip);

        return vec;
      }
    void pixelInput(const std::shared_ptr<PixelHit>& p);

  private:
    ControlResponsePayload processCommand(ControlRequestPayload const &request);

    std::shared_ptr<Alpide> mChip;
    ControlInitiatorSocket socket_control_out;
  };


  struct InnerBarrelStave : public StaveInterface
  {
    InnerBarrelStave(sc_core::sc_module_name const &name,
                     Detector::DetectorPosition& pos,
                     Detector::t_position_to_global_chip_id_func position_to_global_chip_id_func,
                     const AlpideConfig& chip_cfg);

    virtual void addTraces(sc_trace_file *wf, std::string name_prefix) const;

    virtual std::vector<std::shared_ptr<Alpide>> getChips(void) const {
      return mChips;
    }

  private:
    ControlResponsePayload processCommand(ControlRequestPayload const &request);

    std::vector<std::shared_ptr<Alpide>> mChips;
    std::array<ControlInitiatorSocket, 9> socket_control_out;
  };


  struct HalfModule : public sc_module
  {
    HalfModule(sc_core::sc_module_name const &name,
               Detector::DetectorPosition& pos,
               Detector::t_position_to_global_chip_id_func position_to_global_chip_id_func,
               unsigned int half_mod_id,
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
    std::array<ControlInitiatorSocket, ITS::CHIPS_PER_HALF_MODULE> socket_control_out;
  };


  /// The middle and outer barrel staves have 1 control link and 1 data link per half-module
  template <int N_HALF_MODULES>
  struct MBOBStave : public StaveInterface
  {
    MBOBStave(sc_core::sc_module_name const &name,
              Detector::DetectorPosition& pos,
              Detector::t_position_to_global_chip_id_func position_to_global_chip_id_func,
              const Detector::DetectorConfigBase& cfg);
    void addTraces(sc_trace_file *wf, std::string name_prefix) const;
    std::vector<std::shared_ptr<Alpide>> getChips(void) const;

  private:
    std::vector<std::shared_ptr<HalfModule>> mHalfModules;
  };

  template<typename ...> using MiddleBarrelStave = MBOBStave<HALF_MODULES_PER_MB_STAVE>;
  template<typename ...> using OuterBarrelStave = MBOBStave<HALF_MODULES_PER_OB_STAVE>;

}

#endif
