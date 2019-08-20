/**
 * @file   EventRootPCT.hpp
 * @author Simon Voigt Nesbo
 * @date   March 1, 2019
 * @brief  Class for handling events for PCT stored in .root files
 */

#ifndef EVENT_ROOT_PCT_H
#define EVENT_ROOT_PCT_H

#include <TFile.h>
#include <TTree.h>
#include <QString>
#include <memory>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int_distribution.hpp>
#include "Detector/Common/DetectorConfig.hpp"
#include "EventDigits.hpp"


class EventRootPCT {
private:
  Detector::DetectorConfigBase mConfig;

  Detector::t_global_chip_id_to_position_func mGlobalChipIdToPositionFunc;
  Detector::t_position_to_global_chip_id_func mPositionToGlobalChipIdFunc;

  TFile* mRootFile;
  TTree* mTree;

  bool mMoreEventsLeft = true;
  uint64_t mNumEntries; // Number of entries in TTree
  uint64_t mEntryCounter = 0;
  uint64_t mTimeFrameLength_ns;
  uint64_t mTimeFrameCounter = 0;

  // Data from TTree is read into these variables
  Float_t mPosX;
  Float_t mPosY;
  Float_t mPosZ;
  Int_t mTime;

public:
  EventRootPCT(Detector::DetectorConfigBase config,
               Detector::t_global_chip_id_to_position_func global_chip_id_to_position_func,
               Detector::t_position_to_global_chip_id_func position_to_global_chip_id_func,
               const QString& event_filename,
               unsigned int event_frame_length_ns);

  /// Indicates if there are more events left, or if we reached the end
  bool getMoreEventsLeft() const {return mMoreEventsLeft;}
  std::shared_ptr<EventDigits> getNextEvent(void);
};


#endif /* EVENT_ROOT_PCT_H */
