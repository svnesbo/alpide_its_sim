/**
 * @file   EventRootFocal.hpp
 * @author Simon Voigt Nesbo
 * @date   March 8, 2019
 * @brief  Class for handling events for Focal stored in .root files
 */

#ifndef EVENT_ROOT_FOCAL_H
#define EVENT_ROOT_FOCAL_H

#include <TFile.h>
#include <TTree.h>
#include <QString>
#include <memory>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int_distribution.hpp>
#include <boost/random/uniform_real_distribution.hpp>
#include "Detector/Common/DetectorConfig.hpp"
#include "EventDigits.hpp"

#define C_MAX_HITS 1000000

typedef struct {
  Int_t iEvent;
  Int_t iFolder;

  Int_t nPixS1;
  Int_t rowS1[C_MAX_HITS];
  Int_t colS1[C_MAX_HITS];
  Int_t ampS1[C_MAX_HITS];

  Int_t nPixS3;
  Int_t rowS3[C_MAX_HITS];
  Int_t colS3[C_MAX_HITS];
  Int_t ampS3[C_MAX_HITS];
} MacroPixelEvent;

class EventRootFocal {
private:
  Detector::DetectorConfigBase mConfig;

  Detector::t_global_chip_id_to_position_func mGlobalChipIdToPositionFunc;
  Detector::t_position_to_global_chip_id_func mPositionToGlobalChipIdFunc;

  TFile* mRootFile;
  TTree* mTree;

  TBranch *mBranch_iEvent;
  TBranch *mBranch_iFolder;
  TBranch *mBranch_nPixS1;
  TBranch *mBranch_nPixS3;

  TBranch *mBranchRowS1;
  TBranch *mBranchColS1;
  TBranch *mBranchAmpS1;
  TBranch *mBranchRowS3;
  TBranch *mBranchColS3;
  TBranch *mBranchAmpS3;

  MacroPixelEvent* mEvent;

  EventDigits* mEventDigits = nullptr;

  bool mRandomEventOrder;
  bool mMoreEventsLeft = true;
  uint64_t mNumEntries; // Number of entries in TTree
  uint64_t mEntryCounter = 0;

  boost::random::mt19937 mRandHitGen;
  boost::random::uniform_real_distribution<double> *mRandHitMacroCellX, *mRandHitMacroCellY;

  boost::random::mt19937 mRandEventIdGen;
  boost::random::uniform_int_distribution<int> *mRandEventIdDist;

  void createHits(unsigned int col, unsigned int row, unsigned int num_hits,
                  unsigned int layer, EventDigits* event);

public:
  EventRootFocal(Detector::DetectorConfigBase config,
                 Detector::t_global_chip_id_to_position_func global_chip_id_to_position_func,
                 Detector::t_position_to_global_chip_id_func position_to_global_chip_id_func,
                 const QString& event_filename,
                 unsigned int random_seed,
                 bool random_event_order = true);
  ~EventRootFocal();
  /// Indicates if there are more events left, or if we reached the end
  bool getMoreEventsLeft() const {return mMoreEventsLeft;}
  EventDigits* getNextEvent(void);
};


#endif /* EVENT_ROOT_FOCAL_H */
