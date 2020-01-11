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

bool macro_cell_coords_to_chip_coords(const unsigned int macro_cell_x, const unsigned int macro_cell_y,
                                      const unsigned int layer, unsigned int staves_per_quadrant,
                                      unsigned int& global_chip_id, double& chip_x_mm,
                                      double& chip_y_mm);

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
///@param staves_per_quadrant Number of staves per quadrant in simulation
///@param random_seed Random seed used to generate random hits in macro cells
///@param random_event_order Process monte carlo events in random order or not
EventRootFocal::EventRootFocal(Detector::DetectorConfigBase config,
                               Detector::t_global_chip_id_to_position_func global_chip_id_to_position_func,
                               Detector::t_position_to_global_chip_id_func position_to_global_chip_id_func,
                               const QString& event_filename,
                               unsigned int staves_per_quadrant,
                               unsigned int random_seed,
                               bool random_event_order)
  : mConfig(config)
  , mGlobalChipIdToPositionFunc(global_chip_id_to_position_func)
  , mPositionToGlobalChipIdFunc(position_to_global_chip_id_func)
  , mRandomEventOrder(random_event_order)
  , mStavesPerQuadrant(staves_per_quadrant)
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


  mRandHitMacroCellX = new uniform_real_distribution<double>(0, Focal::MACRO_CELL_SIZE_X_MM);
  mRandHitMacroCellY = new uniform_real_distribution<double>(0, Focal::MACRO_CELL_SIZE_Y_MM);

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
///@param[in] macro_cell_col Macro cell column number
///@param[in] macro_cell_row Macro cell row number
///@param[in] layer Layer of the macro cell hit (0 or 1)
///@param[out] event Pointer to EventDigits object to add hits to
void EventRootFocal::createHits(unsigned int macro_cell_col, unsigned int macro_cell_row,
                                unsigned int num_hits, unsigned int layer, EventDigits* event)
{
  unsigned int global_chip_id;
  double chip_x_mm;
  double chip_y_mm;

  bool hit_valid = macro_cell_coords_to_chip_coords(macro_cell_col, macro_cell_row, layer,
                                                    mStavesPerQuadrant, global_chip_id,
                                                    chip_x_mm, chip_y_mm);

  if(hit_valid) {
    // Create specified number of random hits within macro cell
    for(unsigned int hit_counter = 0; hit_counter < num_hits; hit_counter++) {
      // Create a random hit within macro cell with uniform distribution
      double rand_hit_x_mm = chip_x_mm + (*mRandHitMacroCellX)(mRandHitGen);
      double rand_hit_y_mm = chip_y_mm + (*mRandHitMacroCellY)(mRandHitGen);

      // Convert random coords in macro cell to coords in units of ALPIDE pixels
      int chip_col = round(rand_hit_x_mm * ((double)N_PIXEL_COLS / (CHIP_WIDTH_CM*10)));
      int chip_row = round(rand_hit_y_mm * ((double)N_PIXEL_ROWS / (CHIP_HEIGHT_CM*10)));

      // Make sure that x and y coords are within chip boundaries
      if(chip_col >= N_PIXEL_COLS)
        chip_col = N_PIXEL_COLS-1;
      else if(chip_col < 0)
        chip_col = 0;

      if(chip_row >= N_PIXEL_ROWS)
        chip_row = N_PIXEL_ROWS-1;
      else if(chip_row < 0)
        chip_row = 0;

      event->addHit(chip_col, chip_row, global_chip_id);
    }
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

///@brief Calculate global chip id and chip row/column for macro cell hit coordinates
///       The coordinates of the macro cells go from 0,0 (bottom left) to 3200,3200 (top right)
///@param[in] macro_cell_x X coords of hit (in units of macro cells)
///@param[in] macro_cell_y Y coords of hit (in units of macro cells)
///@param[in] layer Focal layer (0 or 1)
///@param[in] staves_per_qudrant Number of staves per quadrant in simulation
///@param[out] global_chip_id Global chip ID of the chip at those coordinates
///@param[out] chip_x_mm X-coordinate of hit in chip
///@param[out] chip_y_mm Y-coordinate of hit in chip
///@return True when the macro cell is within the detector plane, and the output coordinates are
///       valid.
bool macro_cell_coords_to_chip_coords(const unsigned int macro_cell_x, const unsigned int macro_cell_y,
                                      const unsigned int layer, unsigned int staves_per_quadrant,
                                      unsigned int& global_chip_id, double& chip_x_mm,
                                      double& chip_y_mm)
{
  int i_macro_cell_x = macro_cell_x - 1600;
  int i_macro_cell_y = macro_cell_y - 1600;

  // Skip hits that fall inside the gap (though the data set shouldn't really include hits there..)
  if(abs(i_macro_cell_x) < Focal::GAP_SIZE_X_MM/2 && abs(i_macro_cell_y) < Focal::GAP_SIZE_Y_MM/2)
    return false;

  double macro_cell_x_mm = i_macro_cell_x * Focal::MACRO_CELL_SIZE_X_MM;
  double macro_cell_y_mm = i_macro_cell_y * Focal::MACRO_CELL_SIZE_Y_MM;

  unsigned int quadrant;

  if(macro_cell_x_mm > 0 && macro_cell_y_mm > 0) {
    quadrant = 0;
  } else if(macro_cell_x_mm < 0 && macro_cell_y_mm > 0) {
    quadrant = 1;
    macro_cell_x_mm = -macro_cell_x_mm;
  } else if(macro_cell_x_mm < 0 && macro_cell_y_mm < 0) {
    quadrant = 2;
    macro_cell_x_mm = -macro_cell_x_mm;
    macro_cell_y_mm = -macro_cell_y_mm;
  } else {
    quadrant = 3;
    macro_cell_y_mm = -macro_cell_y_mm;
  }

  global_chip_id += quadrant*Focal::CHIPS_PER_QUADRANT;

  // Skip hit if its y-coord falls above or beyond detector plane
  if(macro_cell_y_mm > Focal::STAVES_PER_QUADRANT*Focal::STAVE_SIZE_Y_MM)
    return false;

  // If the hit is in one of the two patches to the right or left of the gap,
  // then subtract the half gap size to "align" them with the rest of the patches,
  // which simplifies the calculations..
  if(macro_cell_y_mm < Focal::STAVES_PER_HALF_PATCH*Focal::STAVE_SIZE_Y_MM)
    macro_cell_x_mm -= Focal::GAP_SIZE_X_MM/2;

  // Skip hit if its x-coord falls outside the detector plane
  if(macro_cell_x_mm > Focal::STAVE_SIZE_X_MM)
    return false;

  unsigned int stave_num_in_quadrant = macro_cell_y_mm / Focal::STAVE_SIZE_Y_MM;

  // Skip stave if it is not included in the simulation
  if(stave_num_in_quadrant >= staves_per_quadrant)
    return false;

  double stave_y_mm = macro_cell_y_mm - stave_num_in_quadrant*Focal::STAVE_SIZE_Y_MM;
  double stave_x_mm = macro_cell_x_mm;

  unsigned int chip_num_in_stave = stave_x_mm / (CHIP_WIDTH_CM*10);

  chip_x_mm = stave_x_mm - chip_num_in_stave*(CHIP_WIDTH_CM*10);
  chip_y_mm = stave_y_mm;

  // Calculate global chip id
  global_chip_id = 0;

  if(layer > 0)
    global_chip_id += Focal::CHIPS_PER_LAYER;

  global_chip_id += quadrant * Focal::CHIPS_PER_QUADRANT;
  global_chip_id += stave_num_in_quadrant * Focal::CHIPS_PER_STAVE;
  global_chip_id += chip_num_in_stave;

  return true;
}
