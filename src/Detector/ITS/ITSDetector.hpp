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

#include <vector>
#include <map>
#include <memory>

#include "ITSDetectorConfig.hpp"
#include "Detector/Common/ITSModulesStaves.hpp"
#include "ReadoutUnit/ReadoutUnit.hpp"
#include <Alpide/PixelHit.hpp>

namespace ITS {

  class ITSDetector : public sc_core::sc_module {
  public:
    sc_in_clk s_system_clk_in;
    sc_event_queue E_trigger_in;

    ///@todo Include some more global busy status etc. for the whole detector?? Maybe some stats?
    sc_out<bool> s_detector_busy_out;

  private:
    std::map<unsigned int, std::shared_ptr<Alpide>> mChipMap;
    sc_vector<sc_vector<ReadoutUnit>> mReadoutUnits;
    sc_vector<sc_vector<StaveInterface>> mDetectorStaves;

    ITSDetectorConfig mConfig;

    unsigned int mNumChips;

    void buildDetector(const ITSDetectorConfig& config, unsigned int trigger_filter_time,
                       bool trigger_filter_enable, unsigned int data_rate_interval_ns);
    void verifyDetectorConfig(const ITSDetectorConfig& config) const;

    void triggerMethod(void);

  public:
    ITSDetector(sc_core::sc_module_name name,
                const ITSDetectorConfig& config,
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
