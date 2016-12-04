#ifndef EVENT_H
#define EVENT_H

#include "hit.h"
#include <string>
#include <set>

class Event;

extern const Event NoEvent;

class Event {
private:
  int mEventTimeNs;
  int mEventId;
  std::set<Hit> mHitSet;

  //@brief Number of hits that carried over from the previous event
  int mCarriedOverCount = 0;

  //@brief Number of hits in the previous event that did not carry over
  int mNotCarriedOverCount = 0;

  
public:
  //@brief Standard constructor
  Event(int event_time_ns, int event_id) {
    mEventTimeNs = event_time_ns;
    mEventId = event_id;
  }
  
  //@brief Copy constructor
  Event(const Event& e) {
    mHitSet = e.mHitSet;
    mEventTimeNs = e.mEventTimeNs;
    mEventId = e.mEventId;
  }

  void addHit(const Hit& h);  
  void addHit(int chip_id, int col, int row);
  void eventCarryOver(const Event& prev_event);
  void eventCarryOver(const std::set<Hit>& hits, int t_delta_ns);
  void writeToFile(const std::string path = "");
  int getEventSize(void) const {return mHitSet.size();}
  int getCarriedOverCount(void) const {return mCarriedOverCount;}
  int getNotCarriedOverCount(void) const {return mNotCarriedOverCount;}
  int getEventId(void) const {return mEventId;}
  int getEventTime(void) const {return mEventTimeNs;}
};

#endif
