/**
 * @file   ITS_config.hpp
 * @author Simon Voigt Nesbo
 * @date   August 15, 2017
 * @brief  Various data structures used for configuration of ITS
 */

#ifndef ITS_CONFIG_HPP
#define ITS_CONFIG_HPP

#include "ITS_constants.hpp"
#include "ITSModulesStaves.hpp"
#include "../ReadoutUnit/ReadoutUnit.hpp"


namespace ITS {

  struct layerConfig {
    unsigned int num_staves;
  };

  struct detectorConfig {
    layerConfig layer[N_LAYERS];
  };

  struct detectorPosition {
    unsigned int layer_id;
    unsigned int stave_id;
    unsigned int module_id;
    unsigned int stave_chip_id;
  };

  inline unsigned int chip_id_to_layer_id(unsigned int chip_id) {
    unsigned int chip_num = 0;
    unsigned int layer;

    for(layer = 0; layer < N_LAYERS; layer++) {
      if(chip_id < chip_num+CHIPS_PER_LAYER[layer])
        break;
      chip_num += CHIPS_PER_LAYER[layer];
    }
    return layer;
  }


  inline unsigned int detector_position_to_chip_id(const detectorPosition &pos) {
    unsigned int chip_num;

    chip_num = CUMULATIVE_CHIP_COUNT_AT_LAYER[pos.layer_id];

    chip_num += pos.stave_id * CHIPS_PER_STAVE_IN_LAYER[pos.layer_id];

    chip_num += pos.module_id * CHIPS_PER_MODULE_IN_LAYER[pos.layer_id];

    chip_num += pos.stave_chip_id;

    return chip_num;
  }


  ///@brief Creator class for ReadoutUnit objects.
  /// Used to create initialized ReadoutUnit objects when
  /// initializing sc_vector of ReadoutUnit.
  class RUCreator {
    bool mInnerBarrelMode;
    unsigned int mLayerId;
    unsigned int mNumCtrlLinks;
    unsigned int mNumDataLinks;
    unsigned int mTriggerFilterTime;

  public:
    RUCreator(unsigned int layer_id, unsigned int trigger_filter_time)
      : mLayerId(layer_id)
      , mTriggerFilterTime(trigger_filter_time)
      {
        mNumCtrlLinks = CTRL_LINKS_PER_LAYER[layer_id]/STAVES_PER_LAYER[layer_id];
        mNumDataLinks = DATA_LINKS_PER_LAYER[layer_id]/STAVES_PER_LAYER[layer_id];

        if(layer_id < 3) {
          // Inner Barrel Readout Unit
          mInnerBarrelMode = true;
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
                             mInnerBarrelMode);
    }
  };


  ///@brief Creator class for StaveInterface objects.
  /// Used to create the right stave object depending on layer,
  /// when initializing sc_vector of StaveInterface.
  class StaveCreator {
    unsigned int mLayerId;

  public:
    StaveCreator(unsigned int layer_id)
      : mLayerId(layer_id)
      {
      }

    ///@brief The actual creator function
    StaveInterface* operator()(const char *name, size_t stave_id) {
      std::string coords_str = std::to_string(mLayerId) + ":" + std::to_string(stave_id);
      std::string ru_name = std::string(name) + coords_str;
      StaveInterface* new_stave_ptr;

      if(mLayerId < 3) {
        std::string stave_name = "IB_stave_" + coords_str;
        new_stave_ptr = new InnerBarrelStave(stave_name.c_str(), mLayerId, stave_id);
      } else if(mLayerId >= 3 && mLayerId < 5) {
        throw std::runtime_error("Middle barrel staves not implemented yet..");
        /*
          std::string stave_name = "MB_stave_" + coords_str;
          new_stave_ptr = new MiddleBarrelStave(stave_name.c_str(), mLayerId, stave_id);
        */
      } else {
        throw std::runtime_error("Outer barrel staves not implemented yet..");
        /*
          std::string stave_name = "OB_stave_" + coords_str;
          new_stave_ptr = new OuterBarrelStave(stave_name.c_str(), mLayerId, stave_id);
        */
      }


      return new_stave_ptr;
    }
  };
}


#endif
