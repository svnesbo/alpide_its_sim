/**
 * @file   EventFrame.cpp
 * @author Simon Voigt Nesbo
 * @date   December 12, 2016
 * @brief  Physics event frame object for Alpide SystemC simulation model.
 *         This class holds all the pixel hits for an event frame denoted by a strobing interval,
 *         which might include hits from none up to several physics event, for one chip in
 *         the detector.
 *         The philosophy behind this class is that the shaping etc. is performed by this
 *         class and the EventGenerator class, and that the pixel hits here can be fed
 *         directly to the Alpide chip at the given time.
 */

#include "EventFrame.hpp"

// Ignore warnings about use of auto_ptr in SystemC library
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#include <systemc.h>
#pragma GCC diagnostic pop

#include <sstream>
#include <fstream>


const EventFrame NoEventFrame(0, 0, -1, -1);


///@brief Standard constructor
///@param[in] event_start_time_ns Start time of trigger event (time when strobe was asserted)
///@param[in] event_end_time_ns End time of trigger event (time when strobe was deasserted)
///@param[in] chip_id Chip ID
///@param[in] event_id Event ID
///@param[in] filter_event Flag that indicates whether this trigger should be filtered or not
///           (when trigger filtering is enabled, and trigger came too close to last event)
EventFrame::EventFrame(int64_t event_start_time_ns, int64_t event_end_time_ns,
                           int chip_id, int event_id, bool filter_event)
{
  mEventStartTimeNs = event_start_time_ns;
  mEventEndTimeNs = event_end_time_ns;
  mChipId = chip_id;
  mEventId = event_id;
  mEventFilteredFlag = filter_event;
}


///@brief Copy constructor
EventFrame::EventFrame(const EventFrame& e)
{
  mHitSet = e.mHitSet;
  mEventStartTimeNs = e.mEventStartTimeNs;
  mEventEndTimeNs = e.mEventEndTimeNs;
  mChipId = e.mChipId;
  mEventId = e.mEventId;
  mEventFilteredFlag = e.mEventFilteredFlag;
}


void EventFrame::addHit(const Hit& h)
{
  mHitSet.insert(h);
}


///@brief Feed this event to the pixel matrix of the specified chip.
///       If the trigger filter flag is set, or if there are no hits in the event,
///       nothing will be sent to the chip, and a new event/MEB will not be created
///       in the Alpide chip / pixel matrix object.
///@param[out] matrix Pixel matrix for the chip
void EventFrame::feedHitsToPixelMatrix(PixelMatrix &matrix) const
{
  // Only feed this event to the chip if it has not been filtered out and if it's not empty
  if(mEventFilteredFlag == false && mHitSet.size() > 0) {
#ifdef DEBUG_OUTPUT
    int64_t time_now = sc_time_stamp().value();

    std::cout << "@ " << sc_time_stamp() << ": EventFrame: feeding trigger event number: ";
    std::cout << mEventId << " to chip." << std::endl;
#endif

    for(auto it = mHitSet.begin(); it != mHitSet.end(); it++)
      matrix.setPixel(it->getCol(), it->getRow());
  }
}


///@todo Note in use.. Revisit this function, since I have changed this class a lot...
///@brief Write this event to file, in XML format.
///       The filename will be: "path/event<mEventId>.xml"
///@param[in] path Path to store file in.
void EventFrame::writeToFile(const std::string path)
{
  // Create XML file and header for this event, name eventX.xml, where X is event number
  std::stringstream ss;

  if(path.length() > 0)
    ss << path << "/";
  else
    ss << "./";

  ss << "event" << mEventId << ".xml";
  std::ofstream of(ss.str().c_str());

  // Write XML header
  of << "<?xml version=\"1.0\"?>" << std::endl;
  of << "<event id=\"" << mEventId << "\" time_ns=\"" << mEventStartTimeNs << "\">" << std::endl;

  int prev_chip_id = -1;

  // The set is ordered by chip id, so we can assume that hits in the same chip will be in consecutive order
  for(std::set<Hit>::iterator it = mHitSet.begin(); it != mHitSet.end(); it++) {
/*
    if(it->getChipId() != prev_chip_id) {
      prev_chip_id = it->getChipId();

      // If it's not the first chip, end the previous chip node
      if(it != mHitSet.begin())
        of << "\t</chip>" << std::endl;

      // Start next chip node
      of << "\t<chip id=\"" << it->getChipId() << "\">" << std::endl;
    }
    of << "\t\t<dig>" << it->getCol() << ":" << it->getRow() << "\t\t</dig>" << std::endl;
*/
  }

  // Don't write </chip> end tag for empty events
  if(mHitSet.size() > 0)
    of << "\t</chip>" << std::endl;

  of << "</event>" << std::endl;

///@todo Implement layers etc.

  // Write data to XML file
/*  if(imod == 0) {
    lay_prev = lay;
    of << "<ly id=\"" << lay << "\">" << std::endl;
    ld_prev = sta;
    of << "\t<ld id=\"" << sta << "\">" << std::endl;
  }
  else if(lay_prev != lay) {
    of << "\t</ld>" << std::endl;
    of << "</ly>" << std::endl;
    lay_prev = lay;
    of << "<ly id=\"" << lay << "\">" << std::endl;
    sta_prev = sta;
    of << "\t<ld id=\"" << sta << "\">" << std::endl;
    mod_prev = mod;
  }
  else if(sta != sta_prev) {
    of << "\t</ld>" << std::endl;
    sta_prev = sta;
    of << "\t<ld id=\"" << sta << "\">" << std::endl;
  }

  //of << "\t\t<dt id=\"" << imod << "\">" << std::endl;
  of << "\t\t<dt id=\"" << chip << "\">" << std::endl;

  if (ndig<1) {
    of << "\t\t</dt>" << std::endl;
    continue;
  }
*/

/*
of << "\t\t</dt>" << std::endl;
of << "\t</ld>" << std::endl;
of << "</ly>" << std::endl;
of << "</event>" << std::endl;
*/


}
