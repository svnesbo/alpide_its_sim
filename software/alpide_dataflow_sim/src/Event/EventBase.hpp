/**
 * @file   EventBase.hpp
 * @author Simon Voigt Nesbo
 * @date   March 5, 2018
 * @brief  Class for handling events from AliRoot MC simulations, stored in an XML file
 */

#ifndef EVENT_BASE_H
#define EVENT_BASE_H

#include <map>
#include <memory>
#include <QString>
#include <QStringList>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int_distribution.hpp>
#include "Alpide/PixelHit.hpp"
#include "Alpide/PixelReadoutStats.hpp"
#include "Detector/Common/DetectorConfig.hpp"

class EventDigits {
  std::vector<PixelHit> mHitDigits;

public:
  void addHit(int col, int row, unsigned int chip_id) {
    mHitDigits.emplace_back(col, row, chip_id);
  }

  auto getDigitsIterator(void) const -> std::vector<PixelHit>::const_iterator
  {
    return mHitDigits.begin();
  }

  auto getDigitsEndIterator(void) const -> std::vector<PixelHit>::const_iterator
  {
    return mHitDigits.end();
  }

  size_t size(void) const {return mHitDigits.size();}

  void printEvent(void) const {
    for(auto it = mHitDigits.begin(); it != mHitDigits.end(); it++) {
      std::cout << "Chip  " << it->getChipId() << "  ";
      std::cout << it->getCol() << ":";
      std::cout << it->getRow() << std::endl;
    }
  }
};


class EventBase {
protected:
  Detector::DetectorConfigBase mConfig;

  Detector::t_global_chip_id_to_position_func mGlobalChipIdToPositionFunc;
  Detector::t_position_to_global_chip_id_func mPositionToGlobalChipIdFunc;

  // Maps a detector position to each unique chip id
  std::map<unsigned int, Detector::DetectorPosition> mDetectorPositionList;

  std::vector<EventDigits*> mEvents;

  EventDigits* mSingleEvent = nullptr;

  QString mEventPath;
  QStringList mEventFileNames;

  bool mRandomEventOrder;
  int mRandomSeed;
  int mEventCount;
  int mNextEvent;

  /// Load all events to memory if true, read one at a time from file if false
  bool mLoadAllEvents;

  boost::random::mt19937 mRandEventIdGen;
  boost::random::uniform_int_distribution<int> *mRandEventIdDist;

  void createEventIdDistribution(void);

public:
  EventBase(Detector::DetectorConfigBase config,
            Detector::t_global_chip_id_to_position_func global_chip_id_to_position_func,
            Detector::t_position_to_global_chip_id_func position_to_global_chip_id_func,
            const QString& path,
            const QStringList& event_filenames,
            bool random_event_order = true,
            int random_seed = 0,
            bool load_all = false);
  ~EventBase();
  virtual void readEventFiles() = 0;
  virtual EventDigits* readEventFile(const QString& event_filename) = 0;
  const EventDigits* getNextEvent(void);
};



#endif /* EVENT_BASE_H */
