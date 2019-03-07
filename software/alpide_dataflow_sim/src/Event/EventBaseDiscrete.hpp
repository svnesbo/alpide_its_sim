/**
 * @file   EventBaseDiscrete.hpp
 * @author Simon Voigt Nesbo
 * @date   March 5, 2018
 * @brief  Base class for handling discrete events, such as collisions in LHC.
 *         This class assumes that each discrete is stored in its own file.
 */

#ifndef EVENT_BASE_DISCRETE_H
#define EVENT_BASE_DISCRETE_H

#include <map>
#include <memory>
#include <QString>
#include <QStringList>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int_distribution.hpp>
#include "Alpide/PixelHit.hpp"
#include "Alpide/PixelReadoutStats.hpp"
#include "Detector/Common/DetectorConfig.hpp"
#include "EventDigits.hpp"


class EventBaseDiscrete {
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
  EventBaseDiscrete(Detector::DetectorConfigBase config,
            Detector::t_global_chip_id_to_position_func global_chip_id_to_position_func,
            Detector::t_position_to_global_chip_id_func position_to_global_chip_id_func,
            const QString& path,
            const QStringList& event_filenames,
            bool random_event_order = true,
            int random_seed = 0,
            bool load_all = false);
  ~EventBaseDiscrete();
  virtual void readEventFiles() = 0;
  virtual EventDigits* readEventFile(const QString& event_filename) = 0;
  const EventDigits* getNextEvent(void);
};



#endif /* EVENT_BASE_DISCRETE_H */
