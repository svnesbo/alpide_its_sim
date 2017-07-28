/**
 * @file   EventFrame.cpp
 * @author Simon Voigt Nesbo
 * @date   December 12, 2016
 * @brief  Event frame object for Alpide SystemC model.
 *         This class holds all the pixel hits for an event frame denoted
 *         by a strobing interval, which might include hits from zero to several
 *         physics event, for one chip in the detector.
 */

#include "EventFrame.hpp"

// Ignore warnings about use of auto_ptr in SystemC library
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#include <systemc.h>
#pragma GCC diagnostic pop

#include <sstream>
#include <fstream>


///@brief Standard constructor
///@param[in] event_start_time_ns Start time of trigger event (time when strobe was asserted)
///@param[in] event_end_time_ns End time of trigger event (time when strobe was deasserted)
///@param[in] event_id Event ID
EventFrame::EventFrame(int64_t event_start_time_ns, int64_t event_end_time_ns,
                       int event_id)
{
  mEventStartTimeNs = event_start_time_ns;
  mEventEndTimeNs = event_end_time_ns;
  mEventId = event_id;
}


///@brief Copy constructor
EventFrame::EventFrame(const EventFrame& e)
{
  mHitSet = e.mHitSet;
  mEventStartTimeNs = e.mEventStartTimeNs;
  mEventEndTimeNs = e.mEventEndTimeNs;
  mEventId = e.mEventId;
}


void EventFrame::addHit(const Hit& h)
{
  mHitSet.insert(h);
}


///@brief Feed this event to the pixel matrix of the specified chip.
///@param[out] matrix Pixel matrix for the chip
void EventFrame::feedHitsToPixelMatrix(PixelMatrix &matrix) const
{
#ifdef DEBUG_OUTPUT
  int64_t time_now = sc_time_stamp().value();

  std::cout << "@ " << sc_time_stamp() << ": EventFrame: feeding trigger event number: ";
  std::cout << mEventId << " to chip." << std::endl;
#endif

  for(auto it = mHitSet.begin(); it != mHitSet.end(); it++)
    matrix.setPixel(it->getCol(), it->getRow());
}
