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
///   The data file should be organized in the following tree format:
///      - layer 1
///         - stave 1
///            - module 1
///               - chip 1
///                  - hit data 1 (row:col)
///                  - hit data 2
///                  - ...
///                  - hit data i
///               + chip 2
///               + ...
///               + chip j
///            + module 2
///            + ...
///            + module n
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
///   - module node: mod
///   - chip node: chip
///   - hit digit node: dig
///
//    So, for instance, the XML file may look like this:
///   @code
///   <its_detector>
///      <lay id=0>
///         <sta id=0>
///            <mod id=0>
///               <chip id=0>
///                  <dig>123:64</dig>
///                  <dig>234:12</dig>
///                  <dig>10:54</dig>
///               </chip>
///            </mod>
///         </sta>
///      </lay>
///      <lay id=1>
///         ...
///      </lay>
///   </its_detector>
///   @endcode
///    The data pattern file will be read, and hits from the pattern file
///    will be generated on the pixel(s) of the corresponding layer/stave/chip.
///@}

#include <iostream>
#include <boost/random/random_device.hpp>
#include "EventXML.hpp"

using boost::random::uniform_int_distribution;


EventXML::EventXML(bool random_event_order, int random_seed)
  : mRandomEventOrder(random_event_order)
  , mRandomSeed(random_seed)
  , mEventCount(0)
  , mEventCountChanged(true)
{
  if(mRandomSeed == 0) {
    boost::random::random_device r;

    std::cout << "Boost random_device entropy: " << r.entropy() << std::endl;

    unsigned int random_seed = r();
    mRandEventIdGen.seed(random_seed);
    std::cout << "Random event ID generator random seed: " << random_seed << std::endl;
  } else {
    mRandEventIdGen.seed(mRandomSeed);
  }

  mRandEventIdDist = nullptr;


  ///@todo Temporary - only for testing until we have real detector geometry in place..
  //mDetectorPositionList[0] = {0, 0, 0, 4};
  for(unsigned int chip_id = 0; chip_id < 108; chip_id++) {
    unsigned int stave_id = chip_id/9;
    unsigned int module_id = 0;
    unsigned int module_chip_id = chip_id%12;

    mDetectorPositionList[chip_id] = {0, stave_id, module_id, module_chip_id};
  }
}


EventXML::~EventXML()
{
  for(unsigned int i = 0; i < mEvents.size(); i++)
    delete mEvents[i];

  delete mRandEventIdDist;
}


const EventDigits* EventXML::getNextEvent(void)
{
  EventDigits* event = nullptr;
  int next_event_index;

  if(mEvents.empty() == false) {
    if(mRandomEventOrder) {
      if(mEventCountChanged)
        updateEventIdDistribution();

      // Generate random event here
      next_event_index = (*mRandEventIdDist)(mRandEventIdGen);
    } else { // Sequential event order if not random
      mPreviousEvent++;
      mPreviousEvent = mPreviousEvent % mEvents.size();
      next_event_index = mPreviousEvent;
    }

    event = mEvents[next_event_index];

    std::cout << "XML Event number: " << next_event_index << std::endl;
  }

  return event;
}


///@brief When number of events has changed, update the uniform random distribution used to
///       pick event ID, so that the new event IDs are included in the distribution.
void EventXML::updateEventIdDistribution(void)
{
  if(mRandEventIdDist != nullptr)
    delete mRandEventIdDist;

  mRandEventIdDist = new uniform_int_distribution<int>(0, mEvents.size()-1);

  mEventCountChanged = false;
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


///@brief Read a list of .xml input files
///@param path Path of directory that holds .xml files
///@param event_filenames QStringList of .xml files
void EventXML::readEventXML(const QString& path, const QStringList& event_filenames)
{
  for(int i = 0; i < event_filenames.size(); i++)
    readEventXML(path + QString("/") + event_filenames.at(i));
}


///@todo Accept a triggerEvent object or something here????
///@brief Read a monte carlo event from an XML file
///@param event_filename File name and path of .xml file
void EventXML::readEventXML(const QString& event_filename)
{
  QDomDocument xml_dom_document;
  QFile event_file(event_filename);
  QString qdom_error_msg;

  EventDigits* event = new EventDigits();


  if (!event_file.open(QIODevice::ReadOnly))
  {
    std::cerr<<"Cannot open xml file: "<< event_filename.toStdString() << std::endl;
    ///@todo Error handling...
  }

  if (!xml_dom_document.setContent(&event_file, &qdom_error_msg))
  {
    event_file.close();
    std::cerr << "Cannot load xml file: "<< event_filename.toStdString() << std::endl;
    std::cerr << "Error message: " << qdom_error_msg.toStdString() << std::endl;
    ///@todo Error handling...
  }

  QDomElement xml_dom_root_element = xml_dom_document.documentElement();

  for(auto it = mDetectorPositionList.begin(); it != mDetectorPositionList.end(); it++) {
    int chip_id = it->first;

    const ITS::detectorPosition& chip_position = mDetectorPositionList[chip_id];

    std::cout << "Looking for chip ID: " << chip_id << std::endl;
    std::cout << "Layer: " << chip_position.layer_id;
    std::cout << "  Stave: " << chip_position.stave_id;
    std::cout << "  Module: " << chip_position.module_id;
    std::cout << "  Local chip ID: " << chip_position.stave_chip_id;
    std::cout << std::endl;

    QDomElement chip_element; // Chip element is stored here by locateChipInEventXML().

    if(locateChipInEventXML(chip_position, xml_dom_root_element, chip_element)) {
      // Chip was found, copy hits for this chip to triggerEvent object or something??
      std::cout << "Chip found in XML event.." << std::endl;

      // Digit nodes use the <dig> tag
      QDomNodeList digit_node_list = chip_element.elementsByTagName("dig");
      int digit_count = digit_node_list.size();

      std::cout << "Number of hits for this chip: " << digit_count << std::endl;

      for(int digit_it = 0; digit_it < digit_count; digit_it++)
      {
        QDomElement digit_element = digit_node_list.at(digit_it).toElement();

        // Elements in a digit node are stored as: col:row
        QStringList digit_entry = digit_element.text().split(":");

        int col = digit_entry.at(0).toInt();
        int row = digit_entry.at(1).toInt();

        event->addHit(chip_id, col, row);
      }
    }
  }

  mEvents.push_back(event);
}


///@brief Search for a chip in the event XML
///@param[in] chip_position Chip position struct that defines the position of the chip in terms of
///           layer, stave, module, module chip_id, and so on
///@param[in] event_xml_dom_root Reference to the XML root object
///@param[out] chip_element_out Reference to a QDomElement object, which is used as an output and
///            set to the chip element in the XML DOM object (if it was found).
///@return True if chip was found, false if not.
bool EventXML::locateChipInEventXML(const ITS::detectorPosition& chip_position,
                                    const QDomElement& event_xml_dom_root,
                                    QDomElement& chip_element_out)
{
  // Search for layer in XML file
  QDomNodeList layer_list = event_xml_dom_root.elementsByTagName("lay");
  QDomElement layer_element;

  if(findXMLElementInListById(layer_list, chip_position.layer_id, layer_element) == false)
    return false;

  std::cout << "Layer " << chip_position.layer_id << " found.." << std::endl;

  // Search for stave in the layer element in the XML file
  QDomNodeList stave_list = layer_element.elementsByTagName("sta");
  QDomElement stave_element;

  if(findXMLElementInListById(stave_list, chip_position.stave_id, stave_element) == false)
    return false;

  std::cout << "Stave " << chip_position.stave_id << " found.." << std::endl;

  // Search for module in the layer element in the XML file
  QDomNodeList module_list = stave_element.elementsByTagName("mod");
  QDomElement module_element;

  if(findXMLElementInListById(module_list, chip_position.module_id, module_element) == false)
    return false;

  std::cout << "Module " << chip_position.module_id << " found.." << std::endl;

  // Search for chip in the layer element in the XML file
  QDomNodeList chip_list = module_element.elementsByTagName("chip");
  QDomElement chip_element;

  if(findXMLElementInListById(chip_list, chip_position.stave_chip_id, chip_element) == false)
    return false;

  std::cout << "Chip " << chip_position.stave_chip_id << " found.." << std::endl;

  chip_element_out = chip_element;
  return true;
}
