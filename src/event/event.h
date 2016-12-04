#ifndef EVENT_H
#define EVENT_H

#include "hit.h"
#include <string>
#include <set>

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
  int getEventSize(void) {return mHitSet.size();}
  int getCarriedOverCount(void) {return mCarriedOverCount;}
  int getNotCarriedOverCount(void) {return mNotCarriedOverCount;}
};

#endif
