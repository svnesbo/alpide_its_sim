/**
 * @file   PCT_creator.hpp
 * @author Simon Voigt Nesbo
 * @date   January 15, 2019
 * @brief  Classes for creating RU and Stave objects for PCT
 */

#ifndef PCT_CREATOR_HPP
#define PCT_CREATOR_HPP

#include "PCTDetectorConfig.hpp"
#include "Detector/Common/ITSModulesStaves.hpp"
#include "ReadoutUnit/ReadoutUnit.hpp"


namespace PCT {

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

  public:
    RUCreator(unsigned int layer_id,
              unsigned int num_data_links, unsigned int num_ctrl_links,
              unsigned int trigger_filter_time, bool trigger_filter_enable)
      : mLayerId(layer_id)
      , mTriggerFilterTime(trigger_filter_time)
      , mTriggerFilterEnabled(trigger_filter_enable)
      {
        mNumCtrlLinks = num_ctrl_links;
        mNumDataLinks = num_data_links;

        mInnerBarrelMode = true;
      }

    ///@brief The actual creator function
    ReadoutUnit* operator()(const char *name, size_t stave_id) {
      //std::string coords_str = std::to_string(mLayerId) + ":" + std::to_string(stave_id);
      std::string coords_str = std::to_string(mLayerId);
      std::string ru_name = std::string(name) + coords_str;

      // Stave id set to zero here, it is not really used in ReadoutUnit,
      // so it doesn't matter if we have connected more than one stave to it
      return new ReadoutUnit(ru_name.c_str(),
                             mLayerId,
                             0,
                             mNumCtrlLinks,
                             mNumDataLinks,
                             mTriggerFilterTime,
                             mTriggerFilterEnabled,
                             mInnerBarrelMode);
    }
  };


  ///@brief Creator class for StaveInterface objects.
  /// Used to create the right stave object depending on layer,
  /// when initializing sc_vector of StaveInterface.
  class StaveCreator {
    unsigned int mLayerId;
    PCTDetectorConfig mConfig;

  public:
    StaveCreator(unsigned int layer_id, const PCTDetectorConfig& config)
      : mLayerId(layer_id)
      , mConfig(config)
      {
      }

    ///@brief The actual creator function
    ITS::StaveInterface* operator()(const char *name, size_t stave_id) {
      std::string coords_str = std::to_string(mLayerId) + ":" + std::to_string(stave_id);
      std::string ru_name = std::string(name) + coords_str;
      ITS::StaveInterface* new_stave_ptr;

      Detector::DetectorPosition pos;
      pos.layer_id = mLayerId;
      pos.stave_id = stave_id;

      // Not used by IB/MB/OBStave objects
      pos.sub_stave_id = 0;
      pos.module_id = 0;
      pos.module_chip_id = 0;

      std::string stave_name = "IB_stave_" + coords_str;

      new_stave_ptr = new ITS::InnerBarrelStave(stave_name.c_str(),
                                                pos,
                                                &PCT::PCT_position_to_global_chip_id,
                                                mConfig.chip_cfg);
      return new_stave_ptr;
    }
  };
}


#endif
