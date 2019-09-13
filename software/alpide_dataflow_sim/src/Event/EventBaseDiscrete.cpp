/**
 * @file   EventBaseDiscrete.cpp
 * @author Simon Voigt Nesbo
 * @date   March 5, 2018
 * @brief  Base class for handling discrete events, such as collisions in LHC.
 *         This class assumes that each discrete is stored in its own file.
 */

#include <iostream>
#include <boost/random/random_device.hpp>
#include "EventBaseDiscrete.hpp"

using boost::random::uniform_int_distribution;


///@brief Constructor for EventBaseDiscrete class
///@param config detectorConfig object which specifies which staves in ITS should
///              be included.
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
EventBaseDiscrete::EventBaseDiscrete(Detector::DetectorConfigBase config,
                                     Detector::t_global_chip_id_to_position_func global_chip_id_to_position_func,
                                     Detector::t_position_to_global_chip_id_func position_to_global_chip_id_func,
                                     const QString& path,
                                     const QStringList& event_filenames,
                                     bool random_event_order,
                                     int random_seed,
                                     bool load_all)
  : mConfig(config)
  , mGlobalChipIdToPositionFunc(global_chip_id_to_position_func)
  , mPositionToGlobalChipIdFunc(position_to_global_chip_id_func)
  , mEventPath(path)
  , mEventFileNames(event_filenames)
  , mRandomEventOrder(random_event_order)
  , mRandomSeed(random_seed)
  , mEventCount(0)
  , mNextEvent(0)
  , mLoadAllEvents(load_all)
{
  if(random_seed == 0) {
    boost::random::random_device r;

    std::cout << "Boost random_device entropy: " << r.entropy() << std::endl;

    unsigned int random_seed2 = r();
    mRandEventIdGen.seed(random_seed2);
    std::cout << "Random event ID generator random seed: " << random_seed << std::endl;
  } else {
    mRandEventIdGen.seed(random_seed);
  }

  mRandEventIdDist = nullptr;


  // Construct a list of chips to read from the event files
  for(unsigned  layer = 0; layer < config.num_layers; layer++)
  {
    for(unsigned int stave = 0; stave < config.layer[layer].num_staves; stave++)
    {
      for(unsigned int sub_stave = 0;
          sub_stave < config.layer[layer].num_sub_staves_per_full_stave;
          sub_stave++)
      {
        for(unsigned int module = 0;
            module < config.layer[layer].num_modules_per_sub_stave;
            module++)
        {
          for(unsigned int chip = 0;
              chip < config.layer[layer].num_chips_per_module;
              chip++)
          {
            Detector::DetectorPosition pos = {layer, stave, sub_stave, module, chip};
            unsigned int global_chip_id = (*mPositionToGlobalChipIdFunc)(pos);
            mDetectorPositionList[global_chip_id] = pos;
          }
        }
      }
    }
  }

  createEventIdDistribution();
}


EventBaseDiscrete::~EventBaseDiscrete()
{
  for(unsigned int i = 0; i < mEvents.size(); i++)
    delete mEvents[i];

  if(mSingleEvent != nullptr)
    delete mSingleEvent;

  delete mRandEventIdDist;
}


///@brief Get the next event. If the class was constructed with random_event_order
///       set to true, then this will return a random event from the pool of events.
///       If not they will be in sequential order.
///@return Const pointer to EventDigits object for event.
const EventDigits* EventBaseDiscrete::getNextEvent(void)
{
  EventDigits* event = nullptr;
  int current_event_index;


  if(mRandomEventOrder) {
    // Generate random event here
    current_event_index = (*mRandEventIdDist)(mRandEventIdGen);
  } else { // Sequential event order if not random
    current_event_index = mNextEvent;
    mNextEvent++;
    mNextEvent = mNextEvent % mEventFileNames.size();
  }

  if(mLoadAllEvents) {
    if(mEvents.empty() == false) {
      event = mEvents[current_event_index];
    } else {
      std::cout << "Error: No MC events loaded into memory." << std::endl;
      exit(-1);
    }
  } else {
    if(mSingleEvent != nullptr)
      delete mSingleEvent;

    mSingleEvent = readEventFile(mEventPath + QString("/") + mEventFileNames.at(current_event_index));
    event = mSingleEvent;
  }

  std::cout << "MC Event number: " << current_event_index << std::endl;

  return event;
}


///@brief Create a uniform random distribution used to pick event ID,
///       with a range that matches the number of available events.
void EventBaseDiscrete::createEventIdDistribution(void)
{
  if(mRandEventIdDist != nullptr)
    delete mRandEventIdDist;

  mRandEventIdDist = new uniform_int_distribution<int>(0, mEventFileNames.size()-1);
}
