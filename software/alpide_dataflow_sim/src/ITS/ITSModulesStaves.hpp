/**
 * @file   ITSModulesStaves.hpp
 * @author Simon Voigt Nesbo
 * @date   July 11, 2017
 * @brief  This file holds a collection of classes and structs that define modules,
 *         staves and barrels/layers in the ITS detector.
 */

#include <Alpide/AlpideInterface.hpp>

namespace ITS {
  struct StaveInterface : public sc_module {
    std::vector<ControlTargetSocket> control;
    std::vector<DataInitiatorSocket> data;

    StaveInterface(sc_core::sc_module_name const &name = 0,
                   unsigned int layer_id,
                   unsigned int stave_id,
                   unsigned int n_ctrl_links,
                   unsigned int n_data_links)
      : sc_module(name)
      , mLayerId(layer_id)
      , mStaveId(stave_id)
      , control(n_ctrl_links)
      , data(n_data_links)
      {}

    //virtual std::vector<std::shared_ptr<Alpide>> getChips(void) = 0;
    virtual void setPixel(const detectorPosition& pos) = 0;
    void physicsEventFrameInput(const EventFrame& e, const detectorPosition& pos);

    unsigned int getStaveId(void) const { return mStaveId; }
    unsigned int getLayerId(void) const { return mLayerId; }
    unsigned int numCtrlLinks(void) const { return control.size(); }
    unsigned int numDataLinks(void) const { return data.size(); }

  private:
    unsigned int mLayerId;
    unsigned int mStaveId;
  };


  struct InnerBarrelStave : public StaveInterface
  {
    InnerBarrelStave(sc_core::sc_module_name const &name = 0,
                     unsigned int layer_id, unsigned int stave_id)
      : StaveInterface(name, layer_id, stave_id, 1, 9) {}

  private:
    std::array<std::shared_ptr<Alpide>, 9> mChips;
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
    MBOBStave(sc_core::sc_module_name const &name = 0,
              unsigned int layer_id, unsigned int stave_id)
      : StaveInterface(name, layer_id, stave_id, N_HALF_MODULES, N_HALF_MODULES) {}

  private:
    std::array<std::unique_ptr<HalfModule>, N_HALF_MODULES> mHalfModules;
  };

  typedef MiddleBarrelStave MBOBStave<16>;
  typedef OuterBarrelStave MBOBStave<28>;

}
