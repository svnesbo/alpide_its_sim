/**
 * @file   EventFrame.hpp
 * @author Simon Voigt Nesbo
 * @date   January 2, 2017
 * @brief  Physics event frame object for Alpide SystemC simulation model.
 *         This class holds all the pixel hits for an event frame denoted by a strobing interval,
 *         which might include hits from none up to several physics event, for one chip in
 *         the detector.
 *         The philosophy behind this class is that the shaping etc. is performed by this
 *         class and the EventGenerator class, and that the pixel hits here can be fed
 *         directly to the Alpide chip at the given time.
 *
 * @todo   Use SystemC time data type instead of int64_t?
 */


///@addtogroup event_generation
///@{
#ifndef EVENT_FRAME_H
#define EVENT_FRAME_H

#include "Hit.hpp"
#include "Alpide/PixelMatrix.hpp"
#include <string>
#include <set>
#include <cstdint>

///@todo Change all int64_t to sc_time?
using std::int64_t;

class EventFrame;

///@brief A EventFrame that equals NoEventFrame is returned by some of the EventGenerator's
///       functions which return a reference to an event, when there is no EventFrame to return.
extern const EventFrame NoEventFrame;

class EventFrame {
private:
  ///@brief Absolute start time of event
  int64_t mEventStartTimeNs;

  ///@brief Absolute end time of event
  int64_t mEventEndTimeNs;

  int mEventId;
  int mChipId;
  std::set<Hit> mHitSet;

  ///@brief This flag indicates that this event/trigger came too soon, and that it has been filtered out.
  ///       The class object is still created to keep track of the pixels that are hit, but they will
  ///       not be fed to the Alpide chip.
  ///@todo With the new way of doing things, I don't need to have an Event object to keep track of hits,
  ///      they are stored in the EventGenerator object. So I can get rid off this?
  bool mEventFilteredFlag;

public:
  EventFrame(int64_t event_start_time_ns, int64_t event_end_time_ns,
               int chip_id, int event_id, bool filter_event = false);
  EventFrame(const EventFrame& e);
  void addHit(const Hit& h);
  void feedHitsToPixelMatrix(PixelMatrix &matrix) const;
  void writeToFile(const std::string path = "");
  void setEventFilteredFlag(bool value) {mEventFilteredFlag = value;}
  int getEventSize(void) const {return mHitSet.size();}
  int getChipId(void) const {return mChipId;}
  int getEventId(void) const {return mEventId;}
  int64_t getEventStartTime(void) const {return mEventStartTimeNs;}
  int64_t getEventEndTime(void) const {return mEventEndTimeNs;}
  bool getEventFilteredFlag(void) const {return mEventFilteredFlag;}
};


#endif
///@}
