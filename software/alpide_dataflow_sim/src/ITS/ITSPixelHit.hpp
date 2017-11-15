/**
 * @file   ITSPixelHit.hpp
 * @author Simon Voigt Nesbo
 * @date   December 12, 2016
 * @brief  Derived class for a hit in ITS detector
 */

#ifndef ITS_PIXEL_HIT_HPP
#define ITS_PIXEL_HIT_HPP

#include <Alpide/Hit.hpp>
#include "ITS_config.hpp"

namespace ITS {
  class ITSPixelHit : public Hit
  {
  private:
    detectorPosition mPosition;

  public:
    ITSPixelHit(const detectorPosition& position,
                int col,
                int row,
                int64_t time_now_ns,
                int dead_time_ns,
                int active_time_ns)
      : Hit(col, row, time_now_ns, dead_time_ns, active_time_ns)
      , mPosition(position)
      {}

    ITSPixelHit(const detectorPosition& position,
                int col,
                int row,
                int64_t time_active_start_ns,
                int64_t time_active_end_ns)
      : Hit(col, row, time_active_start_ns, time_active_end_ns)
      , mPosition(position)
      {}

    detectorPosition getPosition(void) const {
      return mPosition;
    }

  };
}

#endif
