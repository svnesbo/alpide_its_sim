/**
 * @file   EventBase.cpp
 * @author Simon Voigt Nesbo
 * @date   March 5, 2018
 * @brief  Base class for MC events read from files
 */

#include <iostream>
#include <boost/random/random_device.hpp>
#include "EventBase.hpp"

using boost::random::uniform_int_distribution;


///@brief Constructor for EventBase class
///@param config detectorConfig object which specifies which staves in ITS should
///              be included.
///@param random_event_order True to randomize which event is used, false to get events
///              in sequential order.
///@param random_seed Random seed for event sequence randomizer.
EventBase::EventBase(ITS::detectorConfig config, bool random_event_order, int random_seed)
  : mRandomEventOrder(random_event_order)
  , mRandomSeed(random_seed)
  , mEventCount(0)
  , mEventCountChanged(true)
{
  if(mRandomSeed == 0) {
    boost::random::random_device r;

    std::cout << "Boost random_device entropy: " << r.entropy() << std::endl;

    unsigned int random_seed = r();
    mRandEventIdGen.seed(random_seed);
    std::cout << "Random event ID generator random seed: " << random_seed << std::endl;
  } else {
    mRandEventIdGen.seed(mRandomSeed);
  }

  mRandEventIdDist = nullptr;


  // Construct a list of chips to read from the event files
  for(unsigned  layer = 0; layer < ITS::N_LAYERS; layer++)
  {
    for(unsigned int stave = 0; stave < config.layer[layer].num_staves; stave++)
    {
      for(unsigned int module = 0; module < ITS::MODULES_PER_STAVE_IN_LAYER[layer]; module++)
      {
        for(unsigned int chip = 0; chip < ITS::CHIPS_PER_MODULE_IN_LAYER[layer]; chip++)
        {
          ITS::detectorPosition pos = {layer, stave, module, chip};
          unsigned int global_chip_id = ITS::detector_position_to_chip_id(pos);
          mDetectorPositionList[global_chip_id] = pos;
        }
      }
    }
  }
}


EventBase::~EventBase()
{
  for(unsigned int i = 0; i < mEvents.size(); i++)
    delete mEvents[i];

  delete mRandEventIdDist;
}


///@brief Get the next event. If the class was constructed with random_event_order
///       set to true, then this will return a random event from the pool of events.
///       If not they will be in sequential order.
///@return Const pointer to EventDigits object for event.
const EventDigits* EventBase::getNextEvent(void)
{
  EventDigits* event = nullptr;
  int next_event_index;

  if(mEvents.empty() == false) {
    if(mRandomEventOrder) {
      if(mEventCountChanged)
        updateEventIdDistribution();

      // Generate random event here
      next_event_index = (*mRandEventIdDist)(mRandEventIdGen);
    } else { // Sequential event order if not random
      mPreviousEvent++;
      mPreviousEvent = mPreviousEvent % mEvents.size();
      next_event_index = mPreviousEvent;
    }

    event = mEvents[next_event_index];

    std::cout << "MC Event number: " << next_event_index << std::endl;
  }

  return event;
}


///@brief When number of events has changed, update the uniform random distribution used to
///       pick event ID, so that the new event IDs are included in the distribution.
void EventBase::updateEventIdDistribution(void)
{
  if(mRandEventIdDist != nullptr)
    delete mRandEventIdDist;

  mRandEventIdDist = new uniform_int_distribution<int>(0, mEvents.size()-1);

  mEventCountChanged = false;
}
