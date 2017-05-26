/**
 * @file   EventXML.hpp
 * @author Simon Voigt Nesbo
 * @date   May 26, 2017
 * @brief  Class for handling events from AliRoot MC simulations, stored in an XML file
 */

#ifndef EVENT_XML_H
#define EVENT_XML_H

#include <QString>

struct detectorPosition {
  int layer_id;
  int stave_id;
  int module_id;
  int stave_chip_id;
};


class EventDigits {
  // Vector index: hit/digit number
  // Pair: <Chip ID, pixel hit coords>
  std::vector<std::pair<int, PixelData>> mHitDigits;

public:
  void addHit(int chip_id, int col, int row) {
    mHitDigits.push_back(std::pair<int, PixelData>(chip_id, PixelData(col, row)));
  }
//  std::vector<std::pair<int, PixelData>>::const_iterator getDigitsIterator(void) const {
  auto getDigitsIterator(void) const {
    return mHitDigits.begin();
  }
//  std::vector<std::pair<int, PixelData>>::const_iterator getDigitsEndIterator(void) const {
  auto getDigitsEndIterator(void) const {
    return mHitDigits.end();
  }
}


class EventXML {
  // Maps a detector position to each unique chip id
  std::map<unsigned int, detectorPosition> mDetectorPositionList;

  std::vector<EventDigits*> mEvents;

  int mEventCount;
  int mPreviousEvent;
  int mRandomSeed;
  bool mRandomEventOrder;

  bool mEventCountChanged;

  boost::random::mt19937 mRandEventIdGen;
  boost::random::uniform_int_distribution<int> *mRandEventIdDist;

  bool findXMLElementInListById(const QDomNodeElement& list, int id, QDomElement& element_out);
  bool locateChipInEventXML(const detectorPosition& chip_position,
                            const QDomElement& event_xml_dom_root,
                            QDomElement& chip_element_out);
  void updateEventIdDistribution(void);

public:
  EventXML(bool random_event_order = true, int random_seed = 0);
  void readEventXML(QString event_filename);
  const EventDigits* getNextEvent(void);
};



#endif /* EVENT_XML_H */
