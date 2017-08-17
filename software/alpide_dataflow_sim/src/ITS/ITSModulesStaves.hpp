/**
 * @file   ITSModulesStaves.hpp
 * @author Simon Voigt Nesbo
 * @date   July 11, 2017
 * @brief  This file holds a collection of classes and structs that define modules,
 *         staves and barrels/layers in the ITS detector.
 *         Much of this code is copy-paste from AlpideModule.hpp/cpp in
 *         Matthias Bonora's WP10 RU SystemC simulation code.
 */

#include <Alpide/Alpide.hpp>
#include <Alpide/AlpideInterface.hpp>

namespace ITS {
  struct StaveInterface : public sc_module {
    std::vector<ControlTargetSocket> socket_control_in;
    std::vector<DataInitiatorSocket> socket_data_out;

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


  struct InnerBarrelStave : public StaveInterface
  {
    InnerBarrelStave(sc_core::sc_module_name const &name,
                     unsigned int layer_id,
                     unsigned int stave_id)
      : StaveInterface(name, layer_id, stave_id, 1, 9) {}

    virtual void addTraces(sc_trace_file *wf, std::string name_prefix) const
      {

      }

    virtual std::vector<std::shared_ptr<Alpide>> getChips(void) const
      {
        return std::vector<std::shared_ptr<Alpide>>(0);
      }

  private:
    ControlResponsePayload processCommand(ControlRequestPayload const &request);

    std::array<std::shared_ptr<Alpide>, 9> mChips;
    std::array<ControlInitiatorSocket, 9> socket_control_out;
  };


  struct HalfModule : public sc_module
  {
    HalfModule(sc_core::sc_module_name const &name = 0)
      : sc_module(name) {
      ///@todo Connect Alpide parallel bus together here..
    }

    ControlTargetSocket control;
    DataInitiatorSocket data;

  private:
    std::array<std::shared_ptr<Alpide>, 7> mChips;
  };


  /// The middle and outer barrel staves have 1 control link and 1 data link per half-module
  template <int N_HALF_MODULES>
  struct MBOBStave : public StaveInterface
  {
    MBOBStave(sc_core::sc_module_name const &name,
              unsigned int layer_id, unsigned int stave_id)
      : StaveInterface(name, layer_id, stave_id, N_HALF_MODULES, N_HALF_MODULES) {}

    virtual void addTraces(sc_trace_file *wf, std::string name_prefix) const
      {

      }

  private:
    std::array<std::unique_ptr<HalfModule>, N_HALF_MODULES> mHalfModules;
  };

  typedef MBOBStave<16> MiddleBarrelStave;
  typedef MBOBStave<28> OuterBarrelStave;

}
