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
#include <memory>

#include "ITS_config.hpp"
#include "ITSPixelHit.hpp"
#include "ITSModulesStaves.hpp"
#include "../ReadoutUnit/ReadoutUnit.hpp"
#include <Alpide/Hit.hpp>

namespace ITS {

  class ITSDetector : public sc_core::sc_module {
  public:
    sc_in_clk s_system_clk_in;
    sc_event E_trigger_in;

    ///@todo Include some more global busy status etc. for the whole detector?? Maybe some stats?
    sc_out<bool> s_detector_busy_out;

  private:
    std::vector<std::shared_ptr<Alpide>> mChipVector;
    sc_vector<ReadoutUnit> mReadoutUnits[N_LAYERS];
    sc_vector<StaveInterface> mDetectorStaves[N_LAYERS];
    sc_vector<sc_signal<sc_uint<24>>> s_alpide_data_lines[N_LAYERS];

    unsigned int mNumChips;

    void buildDetector(const detectorConfig& config, unsigned int trigger_filter_time);
    void verifyDetectorConfig(const detectorConfig& config) const;

    void triggerMethod(void);

  public:
    ITSDetector(sc_core::sc_module_name name,
                const detectorConfig& config,
                unsigned int trigger_filter_time);
    void pixelInput(const ITSPixelHit& h);
    void pixelInput(unsigned int chip_id, const Hit& h);
    void setPixel(const ITSPixelHit& h);
    void setPixel(unsigned int chip_id, unsigned int row, unsigned int col);
    void setPixel(const detectorPosition& pos, unsigned int row, unsigned int col);
    unsigned int getNumChips(void) const { return mNumChips; }
    void addTraces(sc_trace_file *wf, std::string name_prefix) const;
  };

}

#endif
