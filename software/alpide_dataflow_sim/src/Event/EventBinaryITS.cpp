/**
 * @file   EventBinaryITS.cpp
 * @author Simon Voigt Nesbo
 * @date   March 5, 2018
 * @brief  Class for handling events from AliRoot MC simulations,
 *         stored in a binary data file.
 */

#include <iostream>
#include <fstream>
#include <boost/random/random_device.hpp>
#include "EventBinaryITS.hpp"
#include "EventBinaryITSFormat.hpp"

///@brief Constructor for EventBinaryITS class, which handles a set of events
///       stored in binary data files.
///@param config detectorConfig object which specifies which staves in ITS should
///              be included. To save time/memory the class will only read data
///              from the data files for the chips that are included in the simulation.
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
EventBinaryITS::EventBinaryITS(Detector::DetectorConfigBase config,
                               Detector::t_global_chip_id_to_position_func global_chip_id_to_position_func,
                               Detector::t_position_to_global_chip_id_func position_to_global_chip_id_func,
                               const QString& path,
                               const QStringList& event_filenames,
                               bool random_event_order,
                               int random_seed,
                               bool load_all)
  : EventBaseDiscrete(config,
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


///@brief Read the whole list of event files into memory
void EventBinaryITS::readEventFiles()
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
EventDigits* EventBinaryITS::readEventFile(const QString& event_filename)
{
  std::uint8_t code_id;
  std::ifstream event_file(event_filename.toStdString(),
                           std::ios_base::in | std::ios_base::ate);
  std::streamsize size = event_file.tellg();
  event_file.seekg(0, std::ios::beg);

  mFileBuffer.clear();
  mFileBuffer.resize(size);
  mFileBufferIdx = 0;

  if(event_file.read((char*)mFileBuffer.data(), size))
  {
    code_id = mFileBuffer[mFileBufferIdx++];

    if(code_id != DETECTOR_START) {
      std::cerr << "Error: file " << event_filename.toStdString();
      std::cerr << " did not start with DETECTOR_START code." << std::endl;
      exit(-1);
    }

    EventDigits* event = new EventDigits();
    bool done = false;

    while(done == false && mFileBufferIdx < mFileBuffer.size()) {
      code_id = mFileBuffer[mFileBufferIdx++];

      if(code_id == LAYER_START) {
        done = readLayer(event_filename.toStdString(), event);
      } else if(code_id == DETECTOR_END) {
        done = true;
      } else {
        std::cerr << "Error: unexpected code " << std::hex << code_id;
        std::cerr << " in file " << event_filename.toStdString() << std::endl;
        delete event;
        exit(-1);
      }
    }
    return event;
  }
  else {
    std::cerr << "Error reading from file " << event_filename.toStdString() << std::endl;
    exit(-1);
  }

  return nullptr;
}


bool EventBinaryITS::readLayer(std::string event_filename, EventDigits* event)
{
  bool done = true;
  std::uint8_t code_id;
  std::uint8_t layer_id = mFileBuffer[mFileBufferIdx++];

  // Stop reading the event file if we have read all the layers
  // that are included in the simulation,
  for(std::uint8_t layer = layer_id; layer < mConfig.num_layers; layer++) {
    if(mConfig.layer[layer].num_staves > 0) {
      done = false;
      break;
    }
  }

  if(done)
    return true;

  while(done == false && mFileBufferIdx < mFileBuffer.size()) {
    code_id = mFileBuffer[mFileBufferIdx++];

    if(code_id == STAVE_START) {
      readStave(event_filename, event, layer_id);
    } else if(code_id == LAYER_END) {
      done = true;
    } else {
      std::cerr << "Error: unexpected code " << std::hex << code_id;
      std::cerr << " in file " << event_filename << std::endl;
      exit(-1);
    }
  }

  // Not done yet, have to check next layer
  return false;
}


void EventBinaryITS::readStave(std::string event_filename,
                            EventDigits* event,
                            std::uint8_t layer_id)
{
  bool done = false;
  bool skip_stave = false;
  std::uint8_t code_id;
  std::uint8_t stave_id = mFileBuffer[mFileBufferIdx] & 0x7F;
  std::uint8_t sub_stave_id = mFileBuffer[mFileBufferIdx++] >> 7;

  // Skip stave if not included in simulation
  if(stave_id >= mConfig.layer[layer_id].num_staves)
    skip_stave = true;

  while(done == false && mFileBufferIdx < mFileBuffer.size()) {
    code_id = mFileBuffer[mFileBufferIdx++];

    if(code_id == MODULE_START) {
      readModule(event_filename, event, layer_id, stave_id, sub_stave_id, skip_stave);
    } else if(code_id == STAVE_END) {
      done = true;
    } else {
      std::cerr << "Error: unexpected code " << std::hex << code_id;
      std::cerr << " in file " << event_filename << std::endl;
      exit(-1);
    }
  }
}


void EventBinaryITS::readModule(std::string event_filename,
                                EventDigits* event,
                                std::uint8_t layer_id,
                                std::uint8_t stave_id,
                                std::uint8_t sub_stave_id,
                                bool skip)
{
  bool done = false;
  std::uint8_t code_id;
  std::uint8_t mod_id  = mFileBuffer[mFileBufferIdx++];

  while(done == false && mFileBufferIdx < mFileBuffer.size()) {
    code_id = mFileBuffer[mFileBufferIdx++];

    if(code_id == CHIP_START) {
      readChip(event_filename, event, layer_id, stave_id, sub_stave_id, mod_id, skip);
    } else if(code_id == MODULE_END) {
      done = true;
    } else {
      std::cerr << "Error: unexpected code " << std::hex << code_id;
      std::cerr << " in file " << event_filename << std::endl;
      exit(-1);
    }
  }
}


void EventBinaryITS::readChip(std::string event_filename,
                              EventDigits* event,
                              std::uint8_t layer_id,
                              std::uint8_t stave_id,
                              std::uint8_t sub_stave_id,
                              std::uint8_t mod_id,
                              bool skip)
{
  bool done = false;
  std::uint16_t col, row;
  std::uint8_t code_id;
  std::uint8_t chip_id = mFileBuffer[mFileBufferIdx++];

  Detector::DetectorPosition pos = {layer_id, stave_id, sub_stave_id, mod_id, chip_id};
  unsigned int global_chip_id = (*mPositionToGlobalChipIdFunc)(pos);

  while(done == false && mFileBufferIdx < mFileBuffer.size()) {
    code_id = mFileBuffer[mFileBufferIdx++];

    if(code_id == DIGIT) {
      if(skip == false) {
        std::uint8_t* col_ptr = (mFileBuffer.data()+mFileBufferIdx);
        col = *((std::uint16_t*) col_ptr);
        mFileBufferIdx += 2;

        std::uint8_t* row_ptr = (mFileBuffer.data()+mFileBufferIdx);
        row = *((std::uint16_t*) row_ptr);
        mFileBufferIdx += 2;

        event->addHit(col, row, global_chip_id);
      } else {
        mFileBufferIdx += 4;
      }
    } else if(code_id == CHIP_END) {
      done = true;
    } else {
      std::cerr << "Error: unexpected code " << std::hex << code_id;
      std::cerr << " in file " << event_filename << std::endl;
      exit(-1);
    }
  }
}
