/**
 * @file   EventXML.cpp
 * @author Simon Voigt Nesbo
 * @date   May 26, 2017
 * @brief  Class for handling events from AliRoot MC simulations, stored in an XML file.
 *         Based on the loadPattern() function that parses XML event files in ITSReadoutSim,
 *         by Adam Szczepankiewicz, but rewritten and updated for the Alpide Dataflow Model.
 */


///@defgroup event_xml Event XML Format
///@{
///   Loads a data pattern file, which is an .xml file with the W3C DOM Level 2 format.
///   The data file should be organized in the tree format below.
///   For the inner layers (0 to 2), there is only one sub stave and module entry, and they
///   are always 0 (since there are no sub staves or modules within an IB stave).
///
///   Tree format:
///      - layer 0
///         - stave 0
///            - sub stave 0
///               - module 0
///                  - chip 0
///                     - hit data 1 (row:col)
///                     - hit data 2
///                     - ...
///                     - hit data i
///                  + chip 1
///                  + ...
///                  + chip j
///                  + module 1
///                  + ...
///                  + module n
///            - sub stave 1
///               - module 0
///                  - chip 0
///                     - hit data 1 (row:col)
///                     - hit data 2
///                     - ...
///                     - hit data i
///                  + chip 1
///                  + ...
///                  + chip j
///                  + module 1
///                  + ...
///                  + module n
///         + stave 2
///         + ...
///         + stave m
///      + layer 2
///      + ...
///      + layer l
///
//    Where the following tag names are used:
///   - top node: its_detector
///   - layer node: lay
///   - stave node: sta
///   - sub stave node: ssta
///   - module node: mod
///   - chip node: chip
///   - hit digit node: dig
///
//    So, for instance, the XML file may look like this:
///   @code
///   <its_detector>
///      <lay id=0>
///         <sta id=0>
///            <ssta id=0>
///               <mod id=0>
///                  <chip id=0>
///                     <dig>123:64</dig>
///                     <dig>234:12</dig>
///                     <dig>10:54</dig>
///                  </chip>
///               </mod>
///            </ssta>
///         </sta>
///      </lay>
///      <lay id=1>
///         ...
///      </lay>
///   </its_detector>
///   @endcode
///
///    The data pattern file will be read, and hits from the pattern file
///    will be generated on the pixel(s) of the corresponding layer/stave/chip.
///
///    The XML file is not required to include lay/sta/mod/chip entries for
///    layers/staves/modules/chips that does not have any digits.
///@}

#include <iostream>
#include "EventXML.hpp"


///@brief Constructor for EventXML class, which handles a set of events stored in XML files.
///@param config detectorConfig object which specifies which staves in ITS should
///              be included. To save time/memory the class will only read data
///              from the XML files for the chips that are included in the simulation.
///@param global_chip_id_to_position_func Pointer to function used to determine global
///                                       chip id based on position
///@param position_to_global_chip_id_func Pointer to function used to determine position
///                                       based on global chip id
///@param path Path to event files
///@param event_filenames String list of event file names
///@param random_event_order True to randomize which event is used, false to get events
///              in sequential order.
///@param random_seed Random seed for event sequence randomizer.
///@param load_all If set to true, load all event files into memory. If not they are read
///                from file as they are used, and do not persist in memory.
EventXML::EventXML(Detector::DetectorConfigBase config,
                   Detector::t_global_chip_id_to_position_func global_chip_id_to_position_func,
                   Detector::t_position_to_global_chip_id_func position_to_global_chip_id_func,
                   const QString& path,
                   const QStringList& event_filenames,
                   bool random_event_order,
                   int random_seed,
                   bool load_all)
  : EventBase(config,
              global_chip_id_to_position_func,
              position_to_global_chip_id_func,
              path,
              event_filenames,
              random_event_order,
              random_seed,
              load_all)
{
  if(load_all)
    readEventFiles();
}


///@brief Find an XML DOM node element in a list of elements, with the requested ID.
///       This assumes that all the elements have an id attribute.
///@param[in] list XML DOM node element list
///@param[in] id Id of element to look for
///@param[out] element_out Reference to QDomElement object, which is used as an output to store
///            the element that was found
///@return True if element with desired ID was found, false if not.
bool EventXML::findXMLElementInListById(const QDomNodeList& list, int id, QDomElement& element_out)
{
  // Search for element in list
  for(int i = 0; i < list.size(); i++) {
    element_out = list.at(i).toElement();

    if(element_out.attribute("id").toInt() == id)
      return true;
  }

  return false;
}


///@brief Read the whole list of event files into memory
void EventXML::readEventFiles()
{
  for(int i = 0; i < mEventFileNames.size(); i++) {
    std::cout << "Reading event XML file " << i+1;
    std::cout << " of " << mEventFileNames.size() << std::endl;
    EventDigits* event = readEventFile(mEventPath + QString("/") + mEventFileNames.at(i));
    mEvents.push_back(event);
  }
}


///@brief Read a monte carlo event from an XML file
///@param event_filename File name and path of .xml file
///@return Pointer to EventDigits object with the event that was read from file
EventDigits* EventXML::readEventFile(const QString& event_filename)
{
  QDomDocument xml_dom_document;
  QFile event_file(event_filename);
  QString qdom_error_msg;

  EventDigits* event = new EventDigits();


  if (!event_file.open(QIODevice::ReadOnly))
  {
    std::cerr<<"Cannot open xml file: "<< event_filename.toStdString() << std::endl;
    delete event;
    exit(-1);
  }

  if (!xml_dom_document.setContent(&event_file, &qdom_error_msg))
  {
    event_file.close();
    std::cerr << "Cannot load xml file: "<< event_filename.toStdString() << std::endl;
    std::cerr << "Error message: " << qdom_error_msg.toStdString() << std::endl;
    delete event;
    exit(-1);
  }

  QDomElement xml_dom_root_element = xml_dom_document.documentElement();

  for(auto it = mDetectorPositionList.begin(); it != mDetectorPositionList.end(); it++) {
    int global_chip_id = it->first;

    const Detector::DetectorPosition& chip_position = mDetectorPositionList[global_chip_id];

    QDomElement chip_element; // Chip element is stored here by locateChipInEventXML().

    if(locateChipInEventXML(chip_position, xml_dom_root_element, chip_element)) {
      // Digit nodes use the <dig> tag
      QDomNodeList digit_node_list = chip_element.elementsByTagName("dig");
      int digit_count = digit_node_list.size();

      for(int digit_it = 0; digit_it < digit_count; digit_it++)
      {
        QDomElement digit_element = digit_node_list.at(digit_it).toElement();

        // Elements in a digit node are stored as: col:row
        QStringList digit_entry = digit_element.text().split(":");

        int col = digit_entry.at(0).toInt();
        int row = digit_entry.at(1).toInt();

        event->addHit(col, row, global_chip_id);
      }
    }
  }

  return event;
}


///@brief Search for a chip in the event XML
///@param[in] chip_position Chip position struct that defines the position of the chip in terms of
///           layer, stave, module, module chip_id, and so on
///@param[in] event_xml_dom_root Reference to the XML root object
///@param[out] chip_element_out Reference to a QDomElement object, which is used as an output and
///            set to the chip element in the XML DOM object (if it was found).
///@return True if chip was found, false if not.
bool EventXML::locateChipInEventXML(const Detector::DetectorPosition& chip_position,
                                    const QDomElement& event_xml_dom_root,
                                    QDomElement& chip_element_out)
{
  // Search for layer in XML file
  // ---------------------------------------------------------------------------
  QDomNodeList layer_list = event_xml_dom_root.elementsByTagName("lay");
  QDomElement layer_element;

  if(findXMLElementInListById(layer_list, chip_position.layer_id, layer_element) == false)
    return false;


  // Search for stave in the layer element in the XML file
  // ---------------------------------------------------------------------------
  QDomNodeList stave_list = layer_element.elementsByTagName("sta");
  QDomElement stave_element;

  if(findXMLElementInListById(stave_list, chip_position.stave_id, stave_element) == false)
    return false;


  // Search for sub-stave in the stave element in the XML file
  // ---------------------------------------------------------------------------
  QDomNodeList sub_stave_list = layer_element.elementsByTagName("ssta");
  QDomElement sub_stave_element;

  if(findXMLElementInListById(sub_stave_list, chip_position.sub_stave_id, sub_stave_element) == false)
    return false;


  // Search for module in the layer element in the XML file
  // ---------------------------------------------------------------------------
  QDomNodeList module_list = sub_stave_element.elementsByTagName("mod");
  QDomElement module_element;

  if(findXMLElementInListById(module_list, chip_position.module_id, module_element) == false)
    return false;


  // Search for chip in the layer element in the XML file
  // ---------------------------------------------------------------------------
  QDomNodeList chip_list = module_element.elementsByTagName("chip");
  QDomElement chip_element;

  if(findXMLElementInListById(chip_list, chip_position.module_chip_id, chip_element) == false)
    return false;

  chip_element_out = chip_element;

  return true;
}
