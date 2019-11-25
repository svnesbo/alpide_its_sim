/**
 * @file   EventRootFocal.cpp
 * @author Simon Voigt Nesbo
 * @date   March 8, 2019
 * @brief  Class for handling events for Focal stored in .root files
 */

#include <iostream>
#include <fstream>
#include <cmath>
#include <boost/random/random_device.hpp>
#include "Alpide/alpide_constants.hpp"
#include "Detector/Focal/Focal_constants.hpp"
#include "Detector/Focal/FocalDetectorConfig.hpp"
#include "EventRootFocal.hpp"

using boost::random::uniform_real_distribution;
using boost::random::uniform_int_distribution;

// Hardcoded constants for ROOT file used
static const double c_macro_cell_x_size_mm = 0.5;
static const double c_macro_cell_y_size_mm = 0.5;

// Size of gap/square in the middle of the Focal plane
// which the beam pipe passes through
static const double c_focal_gap_size_mm = 40;

// The simulation is constructed of inner barrel staves extending left from the beam line
// The macro cell limits below are used to limit the macro cell hits used to those
// that fall within the staves
// The simulation data consists of 3200 x 3200 macro cells of 0.5mm x 0.5mm,
// with cell (0,0) in the upper left corner.
static const unsigned int c_det_top_left_macro_cell_x = 1600;
static const unsigned int c_det_top_left_macro_cell_y = 1600 - ceil((CHIP_HEIGHT_CM*10.0/2) /
                                                                    c_macro_cell_y_size_mm);
static const unsigned int c_det_bottom_right_macro_cell_x = 3200;
static const unsigned int c_det_bottom_right_macro_cell_y = 1600 + ceil((CHIP_HEIGHT_CM*10.0/2) /
                                                                        c_macro_cell_y_size_mm);


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
                               unsigned int random_seed,
                               bool random_event_order)
  : mConfig(config)
  , mGlobalChipIdToPositionFunc(global_chip_id_to_position_func)
  , mPositionToGlobalChipIdFunc(position_to_global_chip_id_func)
  , mRandomEventOrder(random_event_order)
{
  if(random_seed == 0) {
    boost::random::random_device r;

    std::cout << "Boost random_device entropy: " << r.entropy() << std::endl;

    unsigned int random_seed2 = r();
    mRandHitGen.seed(random_seed2);
    mRandEventIdGen.seed(random_seed2);
    std::cout << "Random event ID generator random seed: " << random_seed << std::endl;
  } else {
    mRandHitGen.seed(random_seed);
    mRandEventIdGen.seed(random_seed);
  }


  mRandHitMacroCellX = new uniform_real_distribution<double>(-c_macro_cell_x_size_mm/2,
                                                             c_macro_cell_x_size_mm/2);

  mRandHitMacroCellY = new uniform_real_distribution<double>(-c_macro_cell_y_size_mm/2,
                                                             c_macro_cell_y_size_mm/2);

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

  mRandEventIdDist = new uniform_int_distribution<int>(0, mNumEntries-1);

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
  delete mRandEventIdDist;
}


///@brief Create a number of pixel hits for ALPIDE chips, based on number of hits within a
///       macro cell in the monte carlo simulation data.
///       Recalculates macro cell column/row in the MC simulation data to x/y
///       stave ID,
///       chip ID, and x/y pixel coordinates in the chip, and generates random hits
///@param macro_cell_col Macro cell column number
///@param macro_cell_row Macro cell row number
///@param
void EventRootFocal::createHits(unsigned int macro_cell_col, unsigned int macro_cell_row,
                                unsigned int num_hits, unsigned int layer, EventDigits* event)
{
  // Skip macropixel if it is outside the bounds of the detector plane in the simulation
  if(macro_cell_col < c_det_top_left_macro_cell_x || macro_cell_col > c_det_bottom_right_macro_cell_x ||
     macro_cell_row < c_det_top_left_macro_cell_y || macro_cell_row > c_det_bottom_right_macro_cell_y)
  {
    return;
  }

  // Make macro cell (1600,1600) the center of the coordinate system: (0,0)
  int local_macro_cell_x = macro_cell_col - 1600;
  int local_macro_cell_y = macro_cell_row - 1600;

  double x_mm = local_macro_cell_x * c_macro_cell_x_size_mm;
  double y_mm = local_macro_cell_y * c_macro_cell_y_size_mm;

  // Start x outside the gap in the middle of Focal, to simplify calculations
  x_mm -= c_focal_gap_size_mm;

  // Check that x falls inside detector plane (hardcoded to 3x IB staves on a line)
  if(x_mm < 0 || (x_mm > Focal::STAVES_PER_LAYER[0] * CHIP_WIDTH_CM*10 * Focal::CHIPS_PER_STAVE_IN_LAYER[0]))
    return;

  // Check that y falls inside detector plane (the height of one chip)
  if(y_mm < ((CHIP_HEIGHT_CM*10.0)/2)-c_macro_cell_y_size_mm/2 ||
     y_mm > ((CHIP_HEIGHT_CM*10.0)/2)+c_macro_cell_y_size_mm/2)
    return;

  // Create specified number of random hits within macro cell
  for(unsigned int hit_counter = 0; hit_counter < num_hits; hit_counter++) {
    // Create a random hit within macro cell with uniform distribution
    double pixel_hit_x_mm = x_mm + (*mRandHitMacroCellX)(mRandHitGen);
    double pixel_hit_y_mm = y_mm + (*mRandHitMacroCellY)(mRandHitGen);

    unsigned int stave_id = pixel_hit_x_mm /
      (Focal::CHIPS_PER_STAVE_IN_LAYER[layer] * CHIP_WIDTH_CM*10);

    // X coordinates from start of stave
    double stave_x_mm = pixel_hit_x_mm -
      (stave_id * Focal::CHIPS_PER_STAVE_IN_LAYER[layer] * CHIP_WIDTH_CM*10);

    unsigned int stave_chip_id =  stave_x_mm / (CHIP_WIDTH_CM*10);

    // X position of particle relative to the chip it will hit
    double chip_x_mm = stave_x_mm - (stave_chip_id*(CHIP_WIDTH_CM*10));

    // Y position of particle relative to the chip,
    // with y = 0mm at the top edge of the chip
    double chip_y_mm = pixel_hit_y_mm + (CHIP_HEIGHT_CM*10.0)/2;

    // X/Y pixel coordinates in chip
    unsigned int x_coord = round(chip_x_mm*(N_PIXEL_COLS/(CHIP_WIDTH_CM*10.0)));
    unsigned int y_coord = round(chip_y_mm*(N_PIXEL_ROWS/(CHIP_HEIGHT_CM*10.0)));

    // Make sure that x and y coords are within chip boundaries
    if(x_coord >= N_PIXEL_COLS)
      x_coord = N_PIXEL_COLS-1;
    if(y_coord >= N_PIXEL_ROWS)
      y_coord = N_PIXEL_ROWS-1;

    unsigned int x_coord2, y_coord2;

    // Create a simple 2x2 pixel cluster around x_coord and y_coord
    // Makes sure that we don't get pixels below row/col 0, and not above
    // row 511 or above column 1023
    /*
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
    */

    Detector::DetectorPosition pos = {
      .layer_id = layer,
      .stave_id = stave_id,
      .sub_stave_id = 0,
      .module_id = 0,
      .module_chip_id = stave_chip_id
    };

    unsigned int global_chip_id = Focal::Focal_position_to_global_chip_id(pos);

    event->addHit(x_coord, y_coord, global_chip_id);
    /*
    event->addHit(x_coord, y_coord2, global_chip_id);
    event->addHit(x_coord2, y_coord, global_chip_id);
    event->addHit(x_coord2, y_coord2, global_chip_id);
    */
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

  if(mRandomEventOrder)
    mEntryCounter = (*mRandEventIdDist)(mRandEventIdGen);

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

  // S1: Layer 0 in simulation
  for(int i = 0; i < mEvent->nPixS1; i++) {
    createHits(mEvent->colS1[i], mEvent->rowS1[i], mEvent->ampS1[i], 0, mEventDigits);
  }
  // S3: Layer 1 in simulation
  for(int i = 0; i < mEvent->nPixS3; i++) {
    createHits(mEvent->colS3[i], mEvent->rowS3[i], mEvent->ampS3[i], 1, mEventDigits);
  }

  if(mRandomEventOrder == false) {
    mEntryCounter++;

    if(mEntryCounter == mNumEntries) {
      mEntryCounter = 0;
      //mMoreEventsLeft = false;
    }
  }

  std::cout << "Event size: " << mEventDigits->size() << std::endl;

  return mEventDigits;
}
