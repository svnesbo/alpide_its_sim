/**
 * @file   PixelFrontEnd.hpp
 * @author Simon Voigt Nesbo
 * @date   July 28, 2017
 * @brief
 */

#ifndef PIXEL_FRONT_END_HPP
#define PIXEL_FRONT_END_HPP

#include <deque>
#include <vector>
#include "EventFrame.hpp"

class PixelFrontEnd {
private:
  std::deque<std::shared_ptr<PixelHit>> mHitQueue;

protected:
  EventFrame getEventFrame(uint64_t event_start,
                           uint64_t event_end,
                           uint64_t event_id) const;

public:
  PixelFrontEnd() {}
  void pixelFrontEndInput(const std::shared_ptr<PixelHit>& p);
  void removeInactiveHits(uint64_t time_now);
};


#endif
