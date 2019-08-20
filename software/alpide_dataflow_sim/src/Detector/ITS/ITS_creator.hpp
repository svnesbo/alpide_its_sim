/**
 * @file   ITS_creator.hpp
 * @author Simon Voigt Nesbo
 * @date   January 18, 2018
 * @brief  Classes for creating RU and Stave objects
 */

#ifndef ITS_CREATOR_HPP
#define ITS_CREATOR_HPP

#include "ITSDetectorConfig.hpp"
#include "Detector/Common/ITSModulesStaves.hpp"
#include "ReadoutUnit/ReadoutUnit.hpp"


namespace ITS {

  ///@brief Creator class for ReadoutUnit objects.
  /// Used to create initialized ReadoutUnit objects when
  /// initializing sc_vector of ReadoutUnit.
  class RUCreator {
    bool mInnerBarrelMode;
    unsigned int mLayerId;
    unsigned int mNumCtrlLinks;
    unsigned int mNumDataLinks;
    unsigned int mTriggerFilterTime;
    bool mTriggerFilterEnabled;
    unsigned int mDataRateIntervalNs;

  public:
    RUCreator(unsigned int layer_id, unsigned int trigger_filter_time,
              bool trigger_filter_enable, unsigned int data_rate_interval_ns)
      : mLayerId(layer_id)
      , mTriggerFilterTime(trigger_filter_time)
      , mTriggerFilterEnabled(trigger_filter_enable)
      , mDataRateIntervalNs(data_rate_interval_ns)
      {
        mNumCtrlLinks = CTRL_LINKS_PER_LAYER[layer_id]/STAVES_PER_LAYER[layer_id];
        mNumDataLinks = DATA_LINKS_PER_LAYER[layer_id]/STAVES_PER_LAYER[layer_id];

        if(layer_id < 3) {
          // Inner Barrel Readout Unit
          mInnerBarrelMode = true;
        } else {
          mInnerBarrelMode = false;
        }
      }

    ///@brief The actual creator function
    ReadoutUnit* operator()(const char *name, size_t stave_id) {
      std::string coords_str = std::to_string(mLayerId) + ":" + std::to_string(stave_id);
      std::string ru_name = std::string(name) + coords_str;

      return new ReadoutUnit(ru_name.c_str(),
                             mLayerId,
                             stave_id,
                             mNumCtrlLinks,
                             mNumDataLinks,
                             mTriggerFilterTime,
                             mTriggerFilterEnabled,
                             mInnerBarrelMode,
                             mDataRateIntervalNs);
    }
  };


  ///@brief Creator class for StaveInterface objects.
  /// Used to create the right stave object depending on layer,
  /// when initializing sc_vector of StaveInterface.
  class StaveCreator {
    unsigned int mLayerId;
    ITSDetectorConfig mConfig;

  public:
    StaveCreator(unsigned int layer_id, const ITSDetectorConfig& config)
      : mLayerId(layer_id)
      , mConfig(config)
      {
      }

    ///@brief The actual creator function
    StaveInterface* operator()(const char *name, size_t stave_id) {
      std::string coords_str = std::to_string(mLayerId) + ":" + std::to_string(stave_id);
      std::string ru_name = std::string(name) + coords_str;
      StaveInterface* new_stave_ptr;

      Detector::DetectorPosition pos;
      pos.layer_id = mLayerId;
      pos.stave_id = stave_id;

      // Not used by IB/MB/OBStave objects
      pos.sub_stave_id = 0;
      pos.module_id = 0;
      pos.module_chip_id = 0;

      if(mLayerId < 3) {
        std::string stave_name = "IB_stave_" + coords_str;

        new_stave_ptr = new InnerBarrelStave(stave_name.c_str(),
                                             pos,
                                             &ITS::ITS_position_to_global_chip_id,
                                             mConfig.chip_cfg);
      } else if(mLayerId >= 3 && mLayerId < 5) {
        std::string stave_name = "MB_stave_" + coords_str;
        new_stave_ptr = new MiddleBarrelStave<>(stave_name.c_str(),
                                                pos,
                                                &ITS::ITS_position_to_global_chip_id,
                                                mConfig);
      } else {
        std::string stave_name = "OB_stave_" + coords_str;
        new_stave_ptr = new OuterBarrelStave<>(stave_name.c_str(),
                                               pos,
                                               &ITS::ITS_position_to_global_chip_id,
                                               mConfig);
      }

      return new_stave_ptr;
    }
  };
}


#endif
