/**
 * @file   EventBase.hpp
 * @author Simon Voigt Nesbo
 * @date   March 5, 2018
 * @brief  Class for handling events from AliRoot MC simulations, stored in an XML file
 */

#ifndef EVENT_BASE_H
#define EVENT_BASE_H

#include <map>
#include <QString>
#include <QStringList>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int_distribution.hpp>
#include "Alpide/Hit.hpp"
#include "../ITS/ITS_config.hpp"

class EventDigits {
  // Vector index: hit/digit number
  // Pair: <Chip ID, pixel hit coords>
  std::vector<std::pair<int, PixelData>> mHitDigits;

public:
  void addHit(int chip_id, int col, int row) {
    mHitDigits.push_back(std::pair<int, PixelData>(chip_id, PixelData(col, row)));
  }
//  std::vector<std::pair<int, PixelData>>::const_iterator getDigitsIterator(void) const {
  auto getDigitsIterator(void) const -> std::vector<std::pair<int, PixelData>>::const_iterator
  {
    return mHitDigits.begin();
  }
//  std::vector<std::pair<int, PixelData>>::const_iterator getDigitsEndIterator(void) const {
  auto getDigitsEndIterator(void) const -> std::vector<std::pair<int, PixelData>>::const_iterator
  {
    return mHitDigits.end();
  }

  size_t size(void) const {return mHitDigits.size();}

  void printEvent(void) const {
    for(auto it = mHitDigits.begin(); it != mHitDigits.end(); it++) {
      std::cout << "Chip  " << it->first << "  ";
      std::cout << it->second.getCol() << ":";
      std::cout << it->second.getRow() << std::endl;
    }
  }
};


class EventBase {
protected:
  // Maps a detector position to each unique chip id
  std::map<unsigned int, ITS::detectorPosition> mDetectorPositionList;

  std::vector<EventDigits*> mEvents;

  EventDigits* mSingleEvent = nullptr;

  QString mEventPath;
  QStringList mEventFileNames;

  bool mRandomEventOrder;
  int mRandomSeed;
  int mEventCount;
  int mPreviousEvent;

  /// Load all events to memory if true, read one at a time from file if false
  bool mLoadAllEvents;

  boost::random::mt19937 mRandEventIdGen;
  boost::random::uniform_int_distribution<int> *mRandEventIdDist;

  void createEventIdDistribution(void);

public:
  EventBase(ITS::detectorConfig config,
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
