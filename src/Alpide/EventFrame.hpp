/**
 * @file   EventFrame.hpp
 * @author Simon Voigt Nesbo
 * @date   January 2, 2017
 * @brief  Event frame object for Alpide SystemC model.
 *         This class holds all the pixel hits for an event frame denoted
 *         by a strobing interval, which might include hits from zero to several
 *         physics event, for one chip in the detector.
 */


///@addtogroup event_generation
///@{
#ifndef EVENT_FRAME_H
#define EVENT_FRAME_H

#include "Alpide/PixelHit.hpp"
#include "Alpide/PixelMatrix.hpp"
#include <string>
#include <set>
#include <cstdint>


using std::uint64_t;


class EventFrame {
private:
  ///@brief Absolute start time of event
  uint64_t mEventStartTimeNs;

  ///@brief Absolute end time of event
  uint64_t mEventEndTimeNs;

  int mEventId;
  int mChipId;
  std::set<std::shared_ptr<PixelHit>> mHitSet;

public:
  EventFrame(uint64_t event_start_time_ns, uint64_t event_end_time_ns, uint64_t event_id);
  EventFrame(const EventFrame& e);
  void addHit(const std::shared_ptr<PixelHit>& p);
  void feedHitsToPixelMatrix(PixelMatrix &matrix) const;
  int getEventSize(void) const {return mHitSet.size();}
  int getEventId(void) const {return mEventId;}
  uint64_t getEventStartTime(void) const {return mEventStartTimeNs;}
  uint64_t getEventEndTime(void) const {return mEventEndTimeNs;}
};


#endif
///@}
