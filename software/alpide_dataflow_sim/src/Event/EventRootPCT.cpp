/**
 * @file   EventRootPCT.cpp
 * @author Simon Voigt Nesbo
 * @date   March 1, 2019
 * @brief  Class for handling events for PCT stored in .root files
 */

#include <iostream>
#include <fstream>
#include <cmath>
#include <boost/random/random_device.hpp>
#include "Alpide/alpide_constants.hpp"
#include "Detector/PCT/PCT_constants.hpp"
#include "EventRootPCT.hpp"


// Hardcoded constants for ROOT file used
static const double c_event_x_min_mm = -135;
static const double c_event_x_max_mm = 135;
static const double c_event_y_min_mm = -67.5;
static const double c_event_y_max_mm = 67.5;
static const double c_event_layer_z_distance_mm = 4.18;

//static const std::vector<double> c_event_z_range = {}

///@brief Constructor for EventRootPCT class, which handles a set of events
///       stored in binary data files.
///@param config detectorConfig object which specifies which staves in ITS should
///              be included. To save time/memory the class will only read data
///              from the data files for the chips that are included in the simulation.
///@param global_chip_id_to_position_func Pointer to function used to determine global
///                                       chip id based on position
///@param position_to_global_chip_id_func Pointer to function used to determine position
///                                       based on global chip id
///@param event_filename Full path to event file
EventRootPCT::EventRootPCT(Detector::DetectorConfigBase config,
                           Detector::t_global_chip_id_to_position_func global_chip_id_to_position_func,
                           Detector::t_position_to_global_chip_id_func position_to_global_chip_id_func,
                           const QString& event_filename,
                           unsigned int event_frame_length_ns)
  : mConfig(config)
  , mGlobalChipIdToPositionFunc(global_chip_id_to_position_func)
  , mPositionToGlobalChipIdFunc(position_to_global_chip_id_func)
  , mTimeFrameLength_ns(event_frame_length_ns)
{
  mRootFile = new TFile(event_filename.toStdString().c_str());

  if(mRootFile->IsOpen() == kFALSE || mRootFile->IsZombie() == kTRUE) {
    std::cerr << "Error: Opening \"" << event_filename.toStdString() << "\" failed." << std::endl;
    exit(-1);
  }

  mTree = (TTree*)mRootFile->Get("Hits");

  mTree->SetBranchAddress("posX", &mPosX);
  mTree->SetBranchAddress("posY", &mPosY);
  mTree->SetBranchAddress("posZ", &mPosZ);
  mTree->SetBranchAddress("clockTime", &mTime);

  mNumEntries = mTree->GetEntries();

  if(mNumEntries == 0)
    mMoreEventsLeft = false;
}


///@brief Read a monte carlo event from a binary data file
///@param event_num Event number
///@return Pointer to EventDigits object with the event that was read from file
std::shared_ptr<EventDigits> EventRootPCT::getNextEvent(void)
{
  std::shared_ptr<EventDigits> event = std::make_shared<EventDigits>();

  std::cout << "Getting next event..." << std::endl;

  while(mEntryCounter < mNumEntries) {
    mTree->GetEntry(mEntryCounter);

    // Time is in 25ns clock cycles
    uint64_t time_ns = mTime*25;

    // Stop when we've reached last entry for this time frame
    if(time_ns >= (mTimeFrameCounter*mTimeFrameLength_ns)+mTimeFrameLength_ns)
      break;

    double z_mm = mPosZ;
    unsigned int layer = round(z_mm/c_event_layer_z_distance_mm);

    // Skip hits for layers that are not included in detector configuration
    if(layer < mConfig.num_layers) {
      // Simulation expects the 0,0 coord to be in the top left corner.
      // In the ROOT files the center coord is in the middle of the detector plane,
      // with positive y coords going upwards (simulation expects downwards)
      double x_mm = mPosX + c_event_x_max_mm;
      double y_mm = (c_event_y_max_mm-c_event_y_min_mm) - (mPosY + c_event_y_max_mm);

      unsigned int stave_chip_id =  x_mm / (CHIP_WIDTH_CM*10);
      unsigned int stave_id = y_mm / (CHIP_HEIGHT_CM*10);
      unsigned int global_chip_id = (layer*PCT::CHIPS_PER_LAYER)
        + (stave_id*PCT::CHIPS_PER_STAVE)
        + stave_chip_id;

      // Position of particle relative to the chip it will hit
      double chip_x_mm = x_mm - (stave_chip_id*(CHIP_WIDTH_CM*10));
      double chip_y_mm = y_mm - (stave_id*(CHIP_HEIGHT_CM*10));

      unsigned int x_coord = round(chip_x_mm*(N_PIXEL_COLS/(CHIP_WIDTH_CM*10)));
      unsigned int y_coord = round(chip_y_mm*(N_PIXEL_ROWS/(CHIP_HEIGHT_CM*10)));

      event->addHit(x_coord, y_coord, global_chip_id);
    }

    mEntryCounter++;
  }

  mTimeFrameCounter++;

  if(mEntryCounter == mNumEntries)
    mMoreEventsLeft = false;

  std::cout << "Event size: " << event->size() << std::endl;

  return event;
}
