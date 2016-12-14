/**
 * @file   event.h
 * @Author Simon Voigt Nesbo
 * @date   December 12, 2016
 * @brief  Event class for Alpide SystemC simulation model.
 *         This class holds all the pixel hits for an event for the whole detector.
 *         The philosophy behind this class is that the shaping etc. is performed by this
 *         class and the EventGenerator class, and that the pixel hits here can be fed
 *         directly to the Alpide chip at the given time.
 *
 * Detailed description of file.
 */

#ifndef EVENT_H
#define EVENT_H

#include "hit.h"
#include "../alpide/pixel_matrix.h"
#include <string>
#include <set>

class Event;

extern const Event NoEvent;

class Event {
private:
  //@brief Absolute time of event
  int mEventTimeNs;

  //@brief Time since previous event
  //@todo  Change to long, or maybe double? This will easily overflow..
  int mEventDeltaTimeNs;
  
  int mEventId;
  std::set<Hit> mHitSet;

  //@brief Number of hits that carried over from the previous event
  int mCarriedOverCount = 0;

  //@brief Number of hits in the previous event that did not carry over
  int mNotCarriedOverCount = 0;

  
public:
  //@brief Standard constructor
  Event(int event_time_ns, int event_delta_time_ns, int event_id) {
    mEventTimeNs = event_time_ns;
    mEventDeltaTimeNs = event_delta_time_ns;
    mEventId = event_id;
  }
  
  //@brief Copy constructor
  Event(const Event& e) {
    mHitSet = e.mHitSet;
    mEventTimeNs = e.mEventTimeNs;
    mEventDeltaTimeNs = e.mEventDeltaTimeNs;
    mEventId = e.mEventId;
  }

  void addHit(const Hit& h);  
  void addHit(int chip_id, int col, int row);
  void eventCarryOver(const Event& prev_event);
  void eventCarryOver(const std::set<Hit>& hits, int t_delta_ns);
  void feedHitsToChip(PixelMatrix &matrix, int chip_id) const;
  void writeToFile(const std::string path = "");
  int getEventSize(void) const {return mHitSet.size();}
  int getCarriedOverCount(void) const {return mCarriedOverCount;}
  int getNotCarriedOverCount(void) const {return mNotCarriedOverCount;}
  int getEventId(void) const {return mEventId;}
  int getEventTime(void) const {return mEventTimeNs;}
  int getEventDeltaTime(void) const {return mEventDeltaTimeNs;}
};

#endif
