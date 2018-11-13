/**
 * @file   PixelFrontEnd.cpp
 * @author Simon Voigt Nesbo
 * @date   July 28, 2017
 * @brief
 */

#include "PixelFrontEnd.hpp"


///@brief Input a pixel to the pixel front end.
///       Pixels are added to the end of the "pixel queue"
///@param p Pixel hit input to front end
void PixelFrontEnd::pixelFrontEndInput(const std::shared_ptr<PixelHit>& p)
{
  std::uint64_t time_now = sc_time_stamp().value();
  mHitQueue.push_back(p);

#ifdef PIXEL_DEBUG
  p->mPixInput = true;
  p->mPixInputTime = time_now;
#endif
}


///@brief Remove old hits.
///       Start at the front of the hit queue, and pop (remove)
///       hits from the front while the hits are no longer active at current simulation time,
///       and older than the oldest event frame (so we don't delete hits that may be
///       still be used in a event frame that hasn't been processed yet).
///@param time_now Simulation time now (any that went inactive before this is deleted..)
void PixelFrontEnd::removeInactiveHits(uint64_t time_now)
{
  bool done = false;

  // Hits are ordered in time, and they are all assumed to have
  // the same "active time" (ie. time over threshold)
  while(mHitQueue.empty() == false && done == false) {
    if(mHitQueue.front()->getActiveTimeEnd() < time_now)
    {
      mHitQueue.pop_front();
    } else {
      done = true; // Done if oldest hit is still active
    }
  }
}


///@brief Create a new event frame for the desired time interval
///@param[in] event_start Start time of event frame (time when strobe signal went high).
///@param[in] event_end End time of event frame (time when strobe signal went low again).
///@param[in] event_id ID to mark event with
///@return EventFrame with pixel hits that are active between event_start and event_end
EventFrame PixelFrontEnd::getEventFrame(uint64_t event_start,
                                        uint64_t event_end,
                                        uint64_t event_id) const
{
  EventFrame e(event_start, event_end, event_id);

  for(auto pix_it = mHitQueue.begin(); pix_it != mHitQueue.end(); pix_it++) {
    // All the hits are ordered in time in the hit queue.
    // If this hit is not active, it could be that:
    // 1) We haven't reached the newer hits which would be active for this event yet
    // 2) We have gone through the hits that are active for this event, and have now
    //    reached hits that are "too new" (event queue size is larger than 0 then)
    if((*pix_it)->isActive(e.getEventStartTime(), e.getEventEndTime())) {
      e.addHit(*pix_it);
    } else if (e.getEventSize() > 0) {
      // Case 2. There won't be any more hits now, so we can break.
      /// @todo Is this check worth it performance wise, or is it better to just iterate
      ///       through the whole list?
      break;
    }
  }

  return e;
}
