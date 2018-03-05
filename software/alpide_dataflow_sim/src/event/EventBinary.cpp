/**
 * @file   EventBinary.cpp
 * @author Simon Voigt Nesbo
 * @date   March 5, 2018
 * @brief  Class for handling events from AliRoot MC simulations,
 *         stored in a binary data file.
 */

#include <iostream>
#include <fstream>
#include <boost/random/random_device.hpp>
#include "EventBinary.hpp"
#include "EventBinaryFormat.hpp"

///@brief Constructor for EventBinary class, which handles a set of events
///       stored in binary data files.
///@param config detectorConfig object which specifies which staves in ITS should
///              be included. To save time/memory the class will only read data
///              from the data files for the chips that are included in the simulation.
///@param random_event_order True to randomize which event is used, false to get events
///              in sequential order.
///@param random_seed Random seed for event sequence randomizer.
EventBinary::EventBinary(ITS::detectorConfig config,
                         const QString& path,
                         const QStringList& event_filenames,
                         bool random_event_order,
                         int random_seed,
                         bool load_all)
  : EventBase(config,
              path,
              event_filenames,
              random_event_order,
              random_seed,
              load_all)
{
  if(load_all)
    readEventFiles();
}


///@brief Read the whole list of event files into memory
void EventBinary::readEventFiles()
{
  for(int i = 0; i < mEventFileNames.size(); i++) {
    std::cout << "Reading event binary data file " << i+1;
    std::cout << " of " << mEventFileNames.size() << std::endl;
    EventDigits* event = readEventFile(mEventPath + QString("/") + mEventFileNames.at(i));
    mEvents.push_back(event);
  }
}


///@brief Read a monte carlo event from a binary data file
///@param event_filename File name and path of binary data file
///@return Pointer to EventDigits object with the event that was read from file
EventDigits* EventBinary::readEventFile(const QString& event_filename)
{
  std::ifstream event_file(event_filename.toStdString(),
                           std::ios_base::in | std::ios_base::binary);

  if(!event_file.is_open()) {
    std::cerr << "Error: opening file " << event_filename.toStdString() << std::endl;
    exit(-1);
  }

  std::uint8_t code_id;

  event_file.read((char*)&code_id, sizeof(uint8_t));

  if(code_id != DETECTOR_START) {
    std::cerr << "Error: file " << event_filename.toStdString();
    std::cerr << " did not start with DETECTOR_START code." << std::endl;
    exit(-1);
  }

  EventDigits* event = new EventDigits();
  bool done = false;

  while(done == false && event_file.good()) {
    event_file.read((char*)&code_id, sizeof(uint8_t));

    if(code_id == LAYER_START) {
      readLayer(event_filename.toStdString(), event_file, event);
    } else if(code_id == DETECTOR_END) {
      done = true;
    } else {
      std::cerr << "Error: unexpected code " << std::hex << code_id;
      std::cerr << " in file " << event_filename.toStdString() << std::endl;
      delete event;
      exit(-1);
    }
  }


  if(done != true || event_file.good() == false) {
    std::cerr << "Error: reading file " << event_filename.toStdString();
    std::cerr << " ended unexpectedly" << std::endl;
    delete event;
    exit(-1);
  }

  return event;
}


void EventBinary::readLayer(std::string event_filename,
                            std::ifstream& event_file,
                            EventDigits* event)
{
  uint8_t code_id;
  uint8_t layer_id;
  bool done = false;

  event_file.read((char*)&layer_id, sizeof(uint8_t));

  while(done == false && event_file.good()) {
    event_file.read((char*)&code_id, sizeof(uint8_t));

    if(code_id == STAVE_START) {
      readStave(event_filename, event_file, event, layer_id);
    } else if(code_id == LAYER_END) {
      done = true;
    } else {
      std::cerr << "Error: unexpected code " << std::hex << code_id;
      std::cerr << " in file " << event_filename << std::endl;
      exit(-1);
    }
  }
}


void EventBinary::readStave(std::string event_filename,
                            std::ifstream& event_file,
                            EventDigits* event,
                            std::uint8_t layer_id)
{
  uint8_t code_id;
  uint8_t stave_id;
  bool done = false;

  event_file.read((char*)&stave_id, sizeof(uint8_t));

  while(done == false && event_file.good()) {
    event_file.read((char*)&code_id, sizeof(uint8_t));

    if(code_id == MODULE_START) {
      readModule(event_filename, event_file, event, layer_id, stave_id);
    } else if(code_id == STAVE_END) {
      done = true;
    } else {
      std::cerr << "Error: unexpected code " << std::hex << code_id;
      std::cerr << " in file " << event_filename << std::endl;
      exit(-1);
    }
  }
}


void EventBinary::readModule(std::string event_filename,
                             std::ifstream& event_file,
                             EventDigits* event,
                             std::uint8_t layer_id,
                             std::uint8_t stave_id)
{
  uint8_t code_id;
  uint8_t mod_id;
  bool done = false;

  event_file.read((char*)&mod_id, sizeof(uint8_t));

  while(done == false && event_file.good()) {
    event_file.read((char*)&code_id, sizeof(uint8_t));

    if(code_id == CHIP_START) {
      readChip(event_filename, event_file, event, layer_id, stave_id, mod_id);
    } else if(code_id == MODULE_END) {
      done = true;
    } else {
      std::cerr << "Error: unexpected code " << std::hex << code_id;
      std::cerr << " in file " << event_filename << std::endl;
      exit(-1);
    }
  }
}


void EventBinary::readChip(std::string event_filename,
                           std::ifstream& event_file,
                           EventDigits* event,
                           std::uint8_t layer_id,
                           std::uint8_t stave_id,
                           std::uint8_t mod_id)
{
  uint8_t code_id;
  uint8_t chip_id;
  uint16_t col, row;
  bool done = false;

  event_file.read((char*)&chip_id, sizeof(uint16_t));

  ITS::detectorPosition pos = {layer_id, stave_id, mod_id, chip_id};
  unsigned int global_chip_id = ITS::detector_position_to_chip_id(pos);

  while(done == false && event_file.good()) {
    event_file.read((char*)&code_id, sizeof(uint8_t));

    if(code_id == DIGIT) {
      event_file.read((char*)&col, sizeof(uint16_t));
      event_file.read((char*)&row, sizeof(uint16_t));
      event->addHit(global_chip_id, col, row);
    } else if(code_id == CHIP_END) {
      done = true;
    } else {
      std::cerr << "Error: unexpected code " << std::hex << code_id;
      std::cerr << " in file " << event_filename << std::endl;
      exit(-1);
    }
  }
}
