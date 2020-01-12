/**
 * @file   Focal_creator.hpp
 * @author Simon Voigt Nesbo
 * @date   November 24, 2019
 * @brief  Classes for creating RU and Stave objects
 */

#ifndef FOCAL_CREATOR_HPP
#define FOCAL_CREATOR_HPP

#include "FocalDetectorConfig.hpp"
#include "Detector/Common/ITSModulesStaves.hpp"
#include "Detector/Focal/FocalStaves.hpp"
#include "ReadoutUnit/ReadoutUnit.hpp"

#include <vector>


namespace Focal {

  ///@brief Creator class for ReadoutUnit objects.
  /// Used to create initialized ReadoutUnit objects when
  /// initializing sc_vector of ReadoutUnit.
  class RUCreator {
    bool mInnerBarrelMode;
    unsigned int mLayerId;
    unsigned int mStavesPerQuadrant;
    unsigned int mTriggerFilterTime;
    bool mTriggerFilterEnabled;
    unsigned int mDataRateIntervalNs;

  public:
    ///@brief RU creator constructor.
    ///       The actual creator function takes in a stave_id parameter which is a counter value going
    ///       from zero to whatever value, called by the sc_vector() constructor/initializer.
    ///       Since we want to create N staves for each quadrant, with nonconsecutive stave
    ///       IDs, we have to know where how many staves we want to create for each quadrant,
    ///       and then generate the correct stave ID based on that
    RUCreator(unsigned int layer_id,
              unsigned int staves_per_quadrant,
              unsigned int trigger_filter_time,
              bool trigger_filter_enable,
              unsigned int data_rate_interval_ns)
      : mLayerId(layer_id)
      , mStavesPerQuadrant(staves_per_quadrant)
      , mTriggerFilterTime(trigger_filter_time)
      , mTriggerFilterEnabled(trigger_filter_enable)
      , mDataRateIntervalNs(data_rate_interval_ns)
      {
      }

    ///@brief The actual creator function
    ReadoutUnit* operator()(const char *name, size_t stave_num) {
      unsigned int quadrant = stave_num / mStavesPerQuadrant;
      unsigned int stave_num_in_quadrant = stave_num - quadrant*mStavesPerQuadrant;
      unsigned int stave_id_in_layer = quadrant*Focal::STAVES_PER_QUADRANT + stave_num_in_quadrant;

      std::string coords_str = std::to_string(mLayerId) + ":" + std::to_string(stave_id_in_layer);
      std::string ru_name = std::string(name) + coords_str;


      if(stave_num_in_quadrant < Focal::INNER_STAVES_PER_QUADRANT) {
        // Create RU for Focal inner stave
        std::cout << "Creating inner RU: " << name << std::endl;
        std::vector<bool> data_link_cfg;

        // 1200 Mbps links
        for(unsigned int i = 0; i < Focal::CHIPS_PER_FOCAL_IB_MODULE; i++)
          data_link_cfg.push_back(true);

        // One 400 Mbps link for the OB (half)-module
        data_link_cfg.push_back(false);

        return new ReadoutUnit(ru_name.c_str(),
                               mLayerId,
                               stave_id_in_layer,
                               Focal::CTRL_LINKS_PER_INNER_STAVE,
                               Focal::DATA_LINKS_PER_INNER_STAVE,
                               data_link_cfg,
                               mTriggerFilterTime,
                               mTriggerFilterEnabled,
                               mDataRateIntervalNs);
      } else {
        // Create RU for Focal outer stave
        std::cout << "Creating outer RU: " << name << std::endl;
        // All 400 Mbps links
        std::vector<bool> data_link_cfg(Focal::DATA_LINKS_PER_OUTER_STAVE, false);

        return new ReadoutUnit(ru_name.c_str(),
                               mLayerId,
                               stave_id_in_layer,
                               Focal::CTRL_LINKS_PER_OUTER_STAVE,
                               Focal::DATA_LINKS_PER_OUTER_STAVE,
                               data_link_cfg,
                               mTriggerFilterTime,
                               mTriggerFilterEnabled,
                               mDataRateIntervalNs);
      }
    }
  };


  ///@brief Creator class for StaveInterface objects.
  /// Used to create the right stave object depending on layer,
  /// when initializing sc_vector of StaveInterface.
  class StaveCreator {
    unsigned int mLayerId;
    unsigned int mStavesPerQuadrant;
    FocalDetectorConfig mConfig;

  public:
    StaveCreator(unsigned int layer_id,
                 unsigned int staves_per_quadrant,
                 const FocalDetectorConfig& config)
      : mLayerId(layer_id)
      , mStavesPerQuadrant(staves_per_quadrant)
      , mConfig(config)
      {
      }

    ///@brief The actual creator function
    ITS::StaveInterface* operator()(const char *name, size_t stave_num) {
      unsigned int quadrant = stave_num / mStavesPerQuadrant;
      unsigned int stave_num_in_quadrant = stave_num - quadrant*mStavesPerQuadrant;
      unsigned int stave_id_in_layer = quadrant*Focal::STAVES_PER_QUADRANT + stave_num_in_quadrant;

      std::string coords_str = std::to_string(mLayerId) + ":" + std::to_string(stave_id_in_layer);
      std::string ru_name = std::string(name) + coords_str;

      ITS::StaveInterface* new_stave_ptr;

      Detector::DetectorPosition pos;
      pos.layer_id = mLayerId;
      pos.stave_id = stave_id_in_layer;

      // Not used by IB/MB/OBStave objects
      pos.sub_stave_id = 0;
      pos.module_id = 0;
      pos.module_chip_id = 0;

      if(stave_num_in_quadrant < Focal::INNER_STAVES_PER_QUADRANT) {
        // Create Focal inner stave
        std::string stave_name = "FI_stave_" + coords_str;
        std::cout << "Creating inner stave: " << name << std::endl;
        new_stave_ptr = new Focal::FocalInnerStave(stave_name.c_str(),
                                                   pos,
                                                   &Focal::Focal_position_to_global_chip_id,
                                                   mConfig);
      } else {
        // Create Focal outer stave
        std::string stave_name = "FO_stave_" + coords_str;
        std::cout << "Creating outer stave: " << name << std::endl;
        new_stave_ptr = new Focal::FocalOuterStave(stave_name.c_str(),
                                                   pos,
                                                   &Focal::Focal_position_to_global_chip_id,
                                                   mConfig);
      }

      return new_stave_ptr;
    }
  };
}


#endif
