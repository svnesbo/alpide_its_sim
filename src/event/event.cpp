#include "event.h"
#include <sstream>
#include <fstream>


const Event NoEvent(0, -1);


void Event::addHit(const Hit& h)
{
  mHitSet.insert(h);
}

void Event::addHit(int chip_id, int col, int row)
{
  Hit h(chip_id, col, row);
  addHit(h);
}


//@brief Due to the long analog shaping time following a hit, on the order of 5-10 microseconds,
//       a pixel hit is likely to be active for several "event/trigger frames". This member function
//       will copy hits that would "carry over" from the previous event, based on the time between the
//       previous event and the current one.
//@todo  These two overloaded functions are pretty bad in terms of DRY
//@param hits_prev_event Set of hits in the previous event.
//@param t_delta_ns Time between the current event and the previous event
void Event::eventCarryOver(const std::set<Hit>& hits_prev_event, int t_delta_ns)
{
  for(std::set<Hit>::iterator it = hits_prev_event.begin(); it != hits_prev_event.end(); it++) {
    if(it->timeLeft() > t_delta_ns) {
      Hit h = *it;
      h.decreaseTimers(t_delta_ns);
      mHitSet.insert(h);
      mCarriedOverCount++;
    } else {
      mNotCarriedOverCount++;
    }
  }  
}


//@brief Due to the long analog shaping time following a hit, on the order of 5-10 microseconds,
//       a pixel hit is likely to be active for several "event/trigger frames". This member function
//       will calculate the time difference between this event and the previous event, and use it to
//       determine which hits from the previous event would also lead to active pixel hits in this event.
//@param prev_event Reference to the the previous event.
void Event::eventCarryOver(const Event& prev_event)
{
  int t_delta_ns = this->mEventTimeNs - prev_event.mEventTimeNs;

  //std::cout << "Event id " << mEventId << ": t_delta_ns: " << t_delta_ns << "carrying over hits.." << std::endl;
  
  for(std::set<Hit>::iterator it = prev_event.mHitSet.begin(); it != prev_event.mHitSet.end(); it++) {
    //std::cout << "Hit with x=" << it->getRow() << " y=" << it->getCol() << " time left = " << it->timeLeft() << " ns" << std::endl;
    if(it->timeLeft() > t_delta_ns) {
      Hit h = *it;
      h.decreaseTimers(t_delta_ns);
      mHitSet.insert(h);
      mCarriedOverCount++;
      //std::cout << "Added hit.. time left now: " << h.timeLeft() << std::endl;
    } else {
      mNotCarriedOverCount++;
      //std::cout << "Skipped hit.." << std::endl;
    }
  }
}

//@brief Write this event to file, in XML format.
//       The filename will be: "path/event<mEventId>.xml"
//@param path Path to store file in. 
void Event::writeToFile(const std::string path)
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
  of << "<event id=\"" << mEventId << "\" time_ns=\"" << mEventTimeNs << "\">" << std::endl;

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
  
//@todo Implement layers etc.
  
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
