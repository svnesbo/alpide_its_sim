/**
 * @file   ITSDetector.hpp
 * @author Simon Voigt Nesbo
 * @date   June 21, 2017
 * @brief  Mockup version of ITS detector.
 *         Accepts trigger input from the dummy CTP module, and communicates the trigger
 *         to the readout units, which will forward them to the Alpide objects.
 */

#ifndef ITS_DETECTOR_HPP
#define ITS_DETECTOR_HPP


#include "ITS_constants.hpp"
#include "ITSModulesStaves.hpp"

namespace ITS {

  struct detectorPosition {
    unsigned int layer_id;
    unsigned int stave_id;
    unsigned int module_id;
    unsigned int stave_chip_id;
  };


  class ITSDetector : public sc_core::sc_module {
  public:
    sc_in_clk s_system_clk_in;
    sc_event E_trigger_in;

    ///@todo Include some more global busy status etc. for the whole detector?? Maybe some stats?
    sc_out<bool> s_detector_busy_out;

  private:

    std::map<unsigned int, std::weak_ptr<Alpide>> mChips;
    std::map<unsigned int, std::unique_ptr<ReadoutUnit>> mReadoutUnits;

    std::vector<std::unique_ptr<InnerBarrelStave>>  layer0;
    std::vector<std::unique_ptr<InnerBarrelStave>>  layer1;
    std::vector<std::unique_ptr<InnerBarrelStave>>  layer2;
    std::vector<std::unique_ptr<MiddleBarrelStave>> layer3;
    std::vector<std::unique_ptr<MiddleBarrelStave>> layer4;
    std::vector<std::unique_ptr<OuterBarrelStave>>  layer5;
    std::vector<std::unique_ptr<OuterBarrelStave>>  layer6;

  public:
    ITSDetector(sc_core::sc_module_name name);
    physicsEventFrameInput(const EventFrame& e);
    setPixel(unsigned int chip_id, unsigned int row, unsigned int col);
    setPixel(const detectorPosition& pos);
  };



}

#endif
