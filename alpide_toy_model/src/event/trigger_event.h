/**
 * @file   trigger_event.h
 * @author Simon Voigt Nesbo
 * @date   January 2, 2017
 * @brief  Trigger event class for Alpide SystemC simulation model.
 *         This class holds all the pixel hits for a trigger event for the whole detector.
 *         The philosophy behind this class is that the shaping etc. is performed by this
 *         class and the EventGenerator class, and that the pixel hits here can be fed
 *         directly to the Alpide chip at the given time.
 *
 * @todo   Use SystemC time data type instead of int64_t?
 */

#ifndef TRIGGER_EVENT_H
#define TRIGGER_EVENT_H

#include "hit.h"
#include "../alpide/pixel_matrix.h"
#include <string>
#include <set>
#include <cstdint>

///@todo Change all int64_t to sc_time?
using std::int64_t;

class TriggerEvent;

///@brief A TriggerEvent that equals NoTriggerEvent is returned by some of the EventGenerator's
///       functions which return a reference to an event, when there is no TriggerEvent to return.
extern const TriggerEvent NoTriggerEvent;

class TriggerEvent {
private:
  ///@brief Absolute start time of event
  int64_t mEventStartTimeNs;

  ///@brief Absolute end time of event
  int64_t mEventEndTimeNs;  
  
  int mEventId;
  std::set<Hit> mHitSet;

  ///@brief This flag indicates that this event/trigger came too soon, and that it has been filtered out.
  ///       The class object is still created to keep track of the pixels that are hit, but they will
  ///       not be fed to the Alpide chip.
  ///@todo With the new way of doing things, I don't need to have an Event object to keep track of hits,
  ///      they are stored in the EventGenerator object. So I can get rid off this?
  bool mEventFilteredFlag;

public:
  TriggerEvent(int64_t event_time_ns, int event_id, bool filter_event = false);
  TriggerEvent(const TriggerEvent& e);
  void addHit(const Hit& h);  
  void feedHitsToChip(PixelMatrix &matrix, int chip_id) const;
  void writeToFile(const std::string path = "");
  void setEventFilteredFlag(bool value) {mEventFilteredFlag = value;}
  void setTriggerEventEndTime(int64_t end_time) {mEventEndTimeNs = end_time;}
  int getEventSize(void) const {return mHitSet.size();}
  int getEventId(void) const {return mEventId;}
  int64_t getEventStartTime(void) const {return mEventStartTimeNs;}
  int64_t getEventEndTime(void) const {return mEventEndTimeNs;}
  bool getEventFilteredFlag(void) const {return mEventFilteredFlag;}
};

#endif
