/**
 * @file   PCTDetector.hpp
 * @author Simon Voigt Nesbo
 * @date   January 14, 2019
 * @brief  Mockup version of PCT detector.
 *         Accepts trigger inputs and communicates the trigger
 *         to the readout units, which will forward them to the Alpide objects.
 */

#ifndef PCT_DETECTOR_HPP
#define PCT_DETECTOR_HPP

#include <vector>
#include <memory>

#include "ITS_config.hpp"
#include "ITSModulesStaves.hpp"
#include "../ReadoutUnit/ReadoutUnit.hpp"
#include <Alpide/PixelHit.hpp>

namespace PCT {

  class PCTDetector : public sc_core::sc_module {
  public:
    sc_in_clk s_system_clk_in;
    sc_event_queue E_trigger_in;
    sc_out<bool> s_detector_busy_out;

  private:
    std::vector<std::shared_ptr<Alpide>> mChipVector;
    sc_vector<ReadoutUnit> mReadoutUnits[N_LAYERS];
    sc_vector<StaveInterface> mDetectorStaves[N_LAYERS];
    sc_vector<sc_signal<sc_uint<24>>> s_alpide_data_lines[N_LAYERS];

    detectorConfig mConfig;

    unsigned int mNumChips;

    void buildDetector(const detectorConfig& config, unsigned int trigger_filter_time,
                       bool trigger_filter_enable);
    void verifyDetectorConfig(const detectorConfig& config) const;

    void triggerMethod(void);

  public:
    ITSDetector(sc_core::sc_module_name name,
                const detectorConfig& config,
                unsigned int trigger_filter_time,
                bool trigger_filter_enable);
    void pixelInput(const std::shared_ptr<PixelHit>& pix);
    void setPixel(const std::shared_ptr<PixelHit>& p);
    void setPixel(unsigned int chip_id, unsigned int row, unsigned int col);
    void setPixel(const detectorPosition& pos, unsigned int row, unsigned int col);
    unsigned int getNumChips(void) const { return mNumChips; }
    void addTraces(sc_trace_file *wf, std::string name_prefix) const;
    void writeSimulationStats(const std::string output_path) const;
  };

}

#endif
