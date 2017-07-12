/**
 * @file   ITSModulesStaves.hpp
 * @author Simon Voigt Nesbo
 * @date   July 11, 2017
 * @brief  This file holds a collection of classes and structs that define modules,
 *         staves and barrels/layers in the ITS detector.
 */

#include <Alpide/AlpideInterface.hpp>

namespace ITS {
  template <int N_ctrl_links, int N_data_links>
  struct LinkInterface : public sc_module {
    std::array<ControlInitiatorSocket, N_ctrl_links> data;
    std::array<DataInitiatorSocket, N_data_links> data;

    LinkInterface(sc_core::sc_module_name const &name = 0) : sc_module(name) {}
  };

  struct StaveInterface : public LinkInterface {
    StaveInterface(sc_core::sc_module_name const &name = 0) : LinkInterface(name) {}
    virtual std::vector<std::weak_ptr<Alpide>> getChips(void) = 0;
    virtual void setPixel(const detectorPosition& pos) = 0;
    void physicsEventFrameInput(const EventFrame& e, const detectorPosition& pos);

    unsigned int getStaveId(void) const { return mStaveId; }
    unsigned int getLayerId(void) const { return mLayerId; }

  private:
    unsigned int mLayerId;
    unsigned int mStaveId;
  };


  template <int N_chips, int N_ctrl_links, int N_data_links>
  struct Module : public LinkInterface<N_ctrl_links, N_data_links> {
    Module(sc_core::sc_module_name const &name = 0) : LinkInterface(name) {}

  private:
    std::array<std::unique_ptr<Alpide>, N_chips> mChips;
  };


  struct SingleChip : public Module<1, 1, 1> {
    SingleChip(sc_core::sc_module_name const &name = 0) : Module(name) {}
  };


  struct InnerBarrelStave : public Module<CHIPS_PER_IB_STAVE,
                                          CTRL_LINKS_PER_IB_STAVE,
                                          DATA_LINKS_PER_IB_STAVE>
                          , public StaveInterface
  {
    InnerBarrelStave(sc_core::sc_module_name const &name = 0,
                     unsigned int layer_id, unsigned int stave_id) : Module(name) {}

  private:
    unsigned int mLayerId;
    unsigned int mStaveId;
    //ControlResponsePayload processCommand(ControlRequestPayload const &request);
  };


  struct OuterBarrelModule : public Module<CHIPS_PER_FULL_MODULE,
                                           CTRL_LINKS_PER_FULL_MODULE,
                                           DATA_LINKS_PER_FULL_MODULE>
  {
  public:
    OuterBarrelModule(sc_core::sc_module_name const &name = 0) : Module(name) {}
  };


  ///@brief Base class (struct) for middle and outer barrel staves.
  template <int N_modules>
  struct MBOBStave : public LinkInterface {
    MBOBStave(sc_core::sc_module_name const &name = 0) : LinkInterface(name) {}

  private:
    std::array<std::unique_ptr<OuterBarrelModule>, N_modules> mModules;
  };


  typedef MiddleBarrelStave MBOBStave<MODULES_PER_MB_STAVE>;
  typedef OuterBarrelStave MBOBStave<MODULES_PER_OB_STAVE>;
}
