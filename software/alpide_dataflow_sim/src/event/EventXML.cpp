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
///    The data pattern file will be read, and hits from the pattern file will be generated on the pixel(s) of the
///   corresponding layer/stave/chip.
///@}

#include "EventXML.hpp"


///@brief Find an XML DOM node element in a list of elements, where the elements have an id attribute
///@param[in] list XML DOM node element list
///@param[in] id Id of element to look for
///@param[out] element_out Reference to QDomElement object, which is used as an output to store
///            the element that was found
///@return True if element with desired ID was found, false if not.
bool findXMLElementInListById(const QDomNodeElement& list, int id, QDomElement& element_out)
{
  // Search for element in list
  for(int i = 0; i < list.size(); i++) {
    element_out = list.at(i).toElement();

    if(element_out.attribute("id").toInt() == id)
      return true;
  }

  return false;
}


///@todo Accept a triggerEvent object or something here????
readEventXML(QString event_filename)
{
  QDomDocument xml_dom_document;
  QFile event_file(event_filename);
  QString qdom_error_msg;


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

  QDomElement xml_dom_root_element = domDocument.documentElement();

  for(auto it = mChipList; it != mChipList.end(); it++) {
    int chip_id = *it;

    const detectorPosition& chip_position = mDetectorPositionList[chip_id];

    QDomElement chip_element;

    if(locateChipInEventXML(chip_position, layer_list, chip_element)) {
      // Chip was found, copy hits for this chip to triggerEvent object or something??

      // Digit nodes use the <dig> tag
      QDomNodeList digit_node_list = chip_element.elementsByTagName("dig");
      int digit_count = domDigList.size();

      for(int digit_it = 0; digit_it < digit_count; digit_it++)
      {
        QDomElement digit_element = digit_node_list.at(digit_it).toElement();

        // Elements in a digit node are stored as: col:row
        QStringList digit_entry = digit_element.text().split(":");

        int col = digit_entry.at(0).toInt();
        int row = digit_entry.at(1).toInt();

        mChipMapHitList[chip_id].addDigit(col, row);
      }
    }
  }
}


///@brief Search for a chip in the event XML
///@param[in] chip_position Chip position struct that defines the position of the chip in terms of
///           layer, stave, module, module chip_id, and so on
///@param[in] event_xml_dom_root Reference to the XML root object
///@param[out] chip_element_out Reference to a QDomElement object, which is used as an output and
///            set to the chip element in the XML DOM object (if it was found).
///@return True if chip was found, false if not.
bool locateChipInEventXML(const detectorPosition& chip_position,
                          const QDomElement& event_xml_dom_root,
                          QDomElement& chip_element_out)
{
  // Search for layer in XML file
  QDomNodeList layer_list = event_xml_dom_root.elementsByTagName("lay");
  QDomElement layer_element;

  if(findXMLElementInListById(layer_list, chip_position.layer_id, layer_element) == false)
    return false;


  // Search for stave in the layer element in the XML file
  QDomNodeList stave_list = layer_element.elementsByTagName("sta");
  QDomElement stave_element;

  if(findXMLElementInListById(stave_list, chip_position.stave_id, stave_element) == false)
    return false;


  // Search for module in the layer element in the XML file
  QDomNodeList module_list = stave_element.elementsByTagName("sta");
  QDomElement module_element;

  if(findXMLElementInListById(module_list, chip_position.module_id, module_element) == false)
    return false;


  // Search for chip in the layer element in the XML file
  QDomNodeList chip_list = module_element.elementsByTagName("sta");
  QDomElement chip_element;

  if(findXMLElementInListById(chip_list, chip_position.chip_id, chip_element) == false)
    return false;

  chip_element_out = chip_element;
  return true;
}
