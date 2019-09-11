/**
 * @file   EventRootFocal.cpp
 * @author Simon Voigt Nesbo
 * @date   March 8, 2019
 * @brief  Class for handling events for PCT stored in .root files
 */

#include <iostream>
#include <fstream>
#include <cmath>
#include <boost/random/random_device.hpp>
#include "Alpide/alpide_constants.hpp"
#include "Detector/PCT/PCT_constants.hpp"
#include "EventRootFocal.hpp"

using boost::random::uniform_real_distribution;

// Hardcoded constants for ROOT file used
static const unsigned int c_det_top_left_macro_cell_x = 1329;
static const unsigned int c_det_top_left_macro_cell_y = 1419;
static const unsigned int c_det_bottom_right_macro_cell_x = 1869;
static const unsigned int c_det_bottom_right_macro_cell_y = 1779;
static const double c_macro_cell_x_size_mm = 0.5;
static const double c_macro_cell_y_size_mm = 0.5;

//static const std::vector<double> c_event_z_range = {}

///@brief Constructor for EventRootFocal class, which handles a set of events
///       stored in binary data files.
///@param config detectorConfig object which specifies which staves in ITS should
///              be included. To save time/memory the class will only read data
///              from the data files for the chips that are included in the simulation.
///@param global_chip_id_to_position_func Pointer to function used to determine global
///                                       chip id based on position
///@param position_to_global_chip_id_func Pointer to function used to determine position
///                                       based on global chip id
///@param event_filename Full path to event file
///@param random_seed Random seed used to generate random hits in macro cells
EventRootFocal::EventRootFocal(Detector::DetectorConfigBase config,
                               Detector::t_global_chip_id_to_position_func global_chip_id_to_position_func,
                               Detector::t_position_to_global_chip_id_func position_to_global_chip_id_func,
                               const QString& event_filename,
                               unsigned int random_seed
  )
  : mConfig(config)
  , mGlobalChipIdToPositionFunc(global_chip_id_to_position_func)
  , mPositionToGlobalChipIdFunc(position_to_global_chip_id_func)
{
  mRandHitGen.seed(random_seed);
  mRandHitMacroCellX = new uniform_real_distribution<double>(0, c_macro_cell_x_size_mm);
  mRandHitMacroCellY = new uniform_real_distribution<double>(0, c_macro_cell_y_size_mm);

  mRootFile = new TFile(event_filename.toStdString().c_str());

  if(mRootFile->IsOpen() == kFALSE || mRootFile->IsZombie() == kTRUE) {
    std::cerr << "Error: Opening \"" << event_filename.toStdString() << "\" failed." << std::endl;
    exit(-1);
  }

  mEvent = new MacroPixelEvent;

  mTree = (TTree*)mRootFile->Get("pixTree");

  mBranch_iEvent = mTree->GetBranch("iEvent");
  mBranch_iFolder = mTree->GetBranch("iFolder");
  mBranch_nPixS1 = mTree->GetBranch("nPixS1");
  mBranch_nPixS3 = mTree->GetBranch("nPixS3");

  mBranchRowS1 = mTree->GetBranch("rowS1");
  mBranchColS1 = mTree->GetBranch("colS1");
  mBranchAmpS1 = mTree->GetBranch("ampS1");
  mBranchRowS3 = mTree->GetBranch("rowS3");
  mBranchColS3 = mTree->GetBranch("colS3");
  mBranchAmpS3 = mTree->GetBranch("ampS3");

  mBranch_iEvent->SetAddress(&mEvent->iEvent);
  mBranch_iFolder->SetAddress(&mEvent->iFolder);
  mBranch_nPixS1->SetAddress(&mEvent->nPixS1);
  mBranch_nPixS3->SetAddress(&mEvent->nPixS3);
  mBranchRowS1->SetAddress(&mEvent->rowS1);
  mBranchColS1->SetAddress(&mEvent->colS1);
  mBranchAmpS1->SetAddress(&mEvent->ampS1);
  mBranchRowS3->SetAddress(&mEvent->rowS3);
  mBranchColS3->SetAddress(&mEvent->colS3);
  mBranchAmpS3->SetAddress(&mEvent->ampS3);

  mNumEntries = mTree->GetEntries();

  if(mNumEntries == 0)
    mMoreEventsLeft = false;
}

EventRootFocal::~EventRootFocal()
{
  delete mRandHitMacroCellX;
  delete mRandHitMacroCellY;
  delete mEvent;
  delete mBranch_iEvent;
  delete mBranch_iFolder;
  delete mBranch_nPixS1;
  delete mBranch_nPixS3;
  delete mBranchRowS1;
  delete mBranchColS1;
  delete mBranchAmpS1;
  delete mBranchRowS3;
  delete mBranchColS3;
  delete mBranchAmpS3;
  delete mTree;
  delete mRootFile;
}

void EventRootFocal::createHits(unsigned int col, unsigned int row, unsigned int num_hits,
                                unsigned int layer, EventDigits* event)
{
  // Skip macropixel if it is outside the bounds of the detector plane,
  // which is hardcoded to the size of the pCT detector plane (with 12 staves),
  // centered around the middle macrocell coordinates for the Focal simulation
  if(col < c_det_top_left_macro_cell_x || col >= c_det_bottom_right_macro_cell_x ||
     row < c_det_top_left_macro_cell_y || row >= c_det_bottom_right_macro_cell_y)
  {
    return;
  }

  // Calculate macro cell coords relative to top left corner of pCT detector plane
  int local_macro_cell_x = col - c_det_top_left_macro_cell_x;
  int local_macro_cell_y = row - c_det_top_left_macro_cell_y;

  double x_mm = local_macro_cell_x * c_macro_cell_x_size_mm;
  double y_mm = local_macro_cell_y * c_macro_cell_y_size_mm;

  // Create specified number of random hits within macro cell
  for(unsigned int hit_counter = 0; hit_counter < num_hits; hit_counter++) {
    // Create a random hit within macro cell with uniform distribution
    double pixel_hit_x_mm = x_mm + (*mRandHitMacroCellX)(mRandHitGen);
    double pixel_hit_y_mm = y_mm + (*mRandHitMacroCellY)(mRandHitGen);

    unsigned int stave_chip_id =  pixel_hit_x_mm / (CHIP_WIDTH_CM*10);
    unsigned int stave_id = pixel_hit_y_mm / (CHIP_HEIGHT_CM*10);
    unsigned int global_chip_id = (layer*PCT::CHIPS_PER_LAYER)
      + (stave_id*PCT::CHIPS_PER_STAVE)
      + stave_chip_id;

    // Position of particle relative to the chip it will hit
    double chip_x_mm = pixel_hit_x_mm - (stave_chip_id*(CHIP_WIDTH_CM*10));
    double chip_y_mm = pixel_hit_y_mm - (stave_id*(CHIP_HEIGHT_CM*10));

    unsigned int x_coord = round(chip_x_mm*(N_PIXEL_COLS/(CHIP_WIDTH_CM*10)));
    unsigned int y_coord = round(chip_y_mm*(N_PIXEL_ROWS/(CHIP_HEIGHT_CM*10)));

    // Make sure that x and y coords are within chip boundaries
    if(x_coord >= N_PIXEL_COLS)
      x_coord = N_PIXEL_COLS-1;
    if(y_coord >= N_PIXEL_ROWS)
      y_coord = N_PIXEL_ROWS-1;

    unsigned int x_coord2, y_coord2;

    // Create a simple 2x2 pixel cluster around x_coord and y_coord
    // Makes sure that we don't get pixels below row/col 0, and not above
    // row 511 or above column 1023
    if(x_coord < N_PIXEL_COLS/2) {
      x_coord2 = x_coord+1;
    } else {
      x_coord2 = x_coord-1;
    }

    if(y_coord < N_PIXEL_ROWS/2) {
      y_coord2 = y_coord+1;
    } else {
      y_coord2 = y_coord-1;
    }

    event->addHit(x_coord, y_coord, global_chip_id);
    event->addHit(x_coord, y_coord2, global_chip_id);
    event->addHit(x_coord2, y_coord, global_chip_id);
    event->addHit(x_coord2, y_coord2, global_chip_id);
  }
}

///@brief Read a monte carlo event from a binary data file
///@param event_num Event number
///@return Pointer to EventDigits object with the event that was read from file
EventDigits* EventRootFocal::getNextEvent(void)
{
  if(mEventDigits != nullptr)
    delete mEventDigits;

  mEventDigits = new EventDigits();

  std::cout << "Getting next event..." << std::endl;

  mBranch_iEvent->GetEntry(mEntryCounter);
  mBranch_iFolder->GetEntry(mEntryCounter);
  mBranch_nPixS1->GetEntry(mEntryCounter);
  mBranch_nPixS3->GetEntry(mEntryCounter);

  mBranchRowS1->GetEntry(mEntryCounter);
  mBranchColS1->GetEntry(mEntryCounter);
  mBranchAmpS1->GetEntry(mEntryCounter);
  mBranchRowS3->GetEntry(mEntryCounter);
  mBranchColS3->GetEntry(mEntryCounter);
  mBranchAmpS3->GetEntry(mEntryCounter);


  for(int i = 0; i < mEvent->nPixS1; i++) {
    createHits(mEvent->colS1[i], mEvent->rowS1[i], mEvent->ampS1[i], 0, mEventDigits);
  }
  for(int i = 0; i < mEvent->nPixS3; i++) {
    createHits(mEvent->colS3[i], mEvent->rowS3[i], mEvent->ampS3[i], 1, mEventDigits);
  }

  mEntryCounter++;

  if(mEntryCounter == mNumEntries) {
    mEntryCounter = 0;
    //mMoreEventsLeft = false;
  }

  std::cout << "Event size: " << mEventDigits->size() << std::endl;

  return mEventDigits;
}
