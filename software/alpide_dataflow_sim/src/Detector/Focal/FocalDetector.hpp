/**
 * @file   FocalDetector.hpp
 * @author Simon Voigt Nesbo
 * @date   November 24, 2019
 * @brief  Mockup version of Focal detector.
 *         Accepts trigger input from the dummy CTP module, and communicates the trigger
 *         to the readout units, which will forward them to the Alpide objects.
 */

#ifndef FOCAL_DETECTOR_HPP
#define FOCAL_DETECTOR_HPP

#include <vector>
#include <memory>

#include "FocalDetectorConfig.hpp"
#include "Detector/Common/ITSModulesStaves.hpp"
#include "ReadoutUnit/ReadoutUnit.hpp"
#include <Alpide/PixelHit.hpp>

namespace Focal {

  class FocalDetector : public sc_core::sc_module {
  public:
    sc_in_clk s_system_clk_in;
    sc_event_queue E_trigger_in;

    ///@todo Include some more global busy status etc. for the whole detector?? Maybe some stats?
    sc_out<bool> s_detector_busy_out;

  private:
    std::vector<std::shared_ptr<Alpide>> mChipVector;
    sc_vector<sc_vector<ReadoutUnit>> mReadoutUnits;
    sc_vector<sc_vector<ITS::StaveInterface>> mDetectorStaves;

    FocalDetectorConfig mConfig;

    unsigned int mNumChips;
    void verifyDetectorConfig(const FocalDetectorConfig& config) const;
    void buildDetector(const FocalDetectorConfig& config, unsigned int trigger_filter_time,
                       bool trigger_filter_enable, unsigned int data_rate_interval_ns);

    void triggerMethod(void);

  public:
    FocalDetector(sc_core::sc_module_name name,
                  const FocalDetectorConfig& config,
                  unsigned int trigger_filter_time,
                  bool trigger_filter_enable,
                  unsigned int data_rate_interval_ns);
    void pixelInput(const std::shared_ptr<PixelHit>& pix);
    void setPixel(const std::shared_ptr<PixelHit>& p);
    void setPixel(unsigned int chip_id, unsigned int row, unsigned int col);
    void setPixel(const Detector::DetectorPosition& pos,
                  unsigned int row, unsigned int col);
    unsigned int getNumChips(void) const { return mNumChips; }
    void addTraces(sc_trace_file *wf, std::string name_prefix) const;
    void writeSimulationStats(const std::string output_path) const;
  };

}

#endif
