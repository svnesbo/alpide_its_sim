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


class eventXML {
  // Maps a detector position to each unique chip id
  std::map<unsigned int, detectorPosition> mDetectorPositionList;

};

void readEventXML(QString event_filename);
bool findXMLElementInListById(const QDomNodeElement& list, int id, QDomElement& element_out);
bool locateChipInEventXML(const detectorPosition& chip_position,
                          const QDomElement& event_xml_dom_root,
                          QDomElement& chip_element_out);

#endif /* EVENT_XML_H */
