/**
 * @file   EventXML.hpp
 * @author Simon Voigt Nesbo
 * @date   May 26, 2017
 * @brief  Class for handling events from AliRoot MC simulations, stored in an XML file
 */

#ifndef EVENT_XML_H
#define EVENT_XML_H

#include <QString>
#include <QtXml/QtXml>
#include "EventBase.hpp"


class EventXML : public EventBase {
  bool findXMLElementInListById(const QDomNodeList& list, int id, QDomElement& element_out);
  bool locateChipInEventXML(const ITS::detectorPosition& chip_position,
                            const QDomElement& event_xml_dom_root,
                            QDomElement& chip_element_out);
  void readEventFiles();
  EventDigits* readEventFile(const QString& event_filename);

public:
  EventXML(ITS::detectorConfig config,
           const QString& path,
           const QStringList& event_filenames,
           bool random_event_order = true,
           int random_seed = 0,
           bool load_all = false);
};



#endif /* EVENT_XML_H */
