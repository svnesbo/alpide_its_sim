/**
 * @file   trigger_event.cpp
 * @author Simon Voigt Nesbo
 * @date   December 12, 2016
 * @brief  Event class for Alpide SystemC simulation model.
 *         This class holds all the pixel hits for an event for the whole detector.
 *         The philosophy behind this class is that the shaping etc. is performed by this
 *         class and the EventGenerator class, and that the pixel hits here can be fed
 *         directly to the Alpide chip at the given time.
 *
 */

#include "trigger_event.h"
#include <systemc.h>
#include <sstream>
#include <fstream>


const TriggerEvent NoTriggerEvent(0, 0, -1);


///@brief Standard constructor
TriggerEvent::TriggerEvent(int64_t event_time_ns, int event_id, bool filter_event) {
  mEventStartTimeNs = event_time_ns;
  mEventEndTimeNs = mEventStartTimeNs;
  mEventId = event_id;
  mEventFilteredFlag = filter_event;
}


///@brief Copy constructor
TriggerEvent::TriggerEvent(const TriggerEvent& e) {
  mHitSet = e.mHitSet;
  mEventStartTimeNs = e.mEventStartTimeNs;
  mEventEndTimeNs = e.mEventEndTimeNs;  
  mEventId = e.mEventId;
  mEventFilteredFlag = e.mEventFilteredFlag;
}


void TriggerEvent::addHit(const Hit& h)
{
  mHitSet.insert(h);
}


///@brief Feed this event to the pixel matrix of the specified chip. Only the hits in
///       this event corresponding to chip_id will be fed to the chip.
///@param matrix Pixel matrix for the chip
///@param chip_id Chip ID to feed hits to.
void TriggerEvent::feedHitsToChip(PixelMatrix &matrix, int chip_id) const
{
  // Only feed this event to the chip if it has not been filtered out
  if(mEventFilteredFlag == false) {
    matrix.newEvent();

    std::cout << "@ " << sc_time_stamp() << ": TriggerEvent: feeding trigger event number: ";
    std::cout << mEventId << " to chip." << std::endl;
    
    for(auto it = mHitSet.begin(); it != mHitSet.end(); it++) {
      if(it->getChipId() == chip_id) {
        matrix.setPixel(it->getCol(), it->getRow());
      }
    }
  }
}


///@todo Revisit this function, since I have changed this class a lot...
///@brief Write this event to file, in XML format.
///       The filename will be: "path/event<mEventId>.xml"
///@param path Path to store file in. 
void TriggerEvent::writeToFile(const std::string path)
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
    if(it->getChipId() != prev_chip_id) {
      prev_chip_id = it->getChipId();      

      // If it's not the first chip, end the previous chip node
      if(it != mHitSet.begin())
        of << "\t</chip>" << std::endl;

      // Start next chip node
      of << "\t<chip id=\"" << it->getChipId() << "\">" << std::endl;
    }
    of << "\t\t<dig>" << it->getCol() << ":" << it->getRow() << "\t\t</dig>" << std::endl;
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
