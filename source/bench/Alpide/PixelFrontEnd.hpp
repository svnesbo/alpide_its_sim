/**
 * @file   PixelFrontEnd.hpp
 * @author Simon Voigt Nesbo
 * @date   July 28, 2017
 * @brief
 */

#include <deque>
#include <vector>
#include "EventFrame.hpp"

class PixelFrontEnd {
private:
  std::deque<Hit> mHitQueue;

protected:
  EventFrame getEventFrame(uint64_t event_start,
                           uint64_t event_end,
                           int event_id) const

public:
  PixelFrontEnd();
  void pixelInput(Hit& h);
  void eventInput(EventFrame& event);
  void removeInactiveHits(uint64_t time_now);
};
