#include "event_generator.h"


//@brief Default constructor.
EventGenerator::EventGenerator()
{
  mBunchCrossingRateNs = 25;  
  mGapFactor = 0.0; 

  calculateAverageCrossingRate();
  
  mHitMultiplicityAverage = 1000;
  mHitMultiplicityDeviation = 300;
  mRandomSeed = 0;  
}

//@brief Constructor. 
//@param BC_rate_ns Bunch crossing rate in nanoseconds.
//@param gap_factor Specifies the ratio of unfilled bunches / total number of possible bunches.
//@param hit_mult_avg Average hit multiplicity. Then number of hits generated for an event will be gaussian distributed,
//                    with this value as the mean, and hit_mult_dev as the standard deviation.
//@param hit_mult_dev Standard deviation for hit multiplicity.
//@param random_seed Random seed to use in random number generators.
EventGenerator::EventGenerator(int BC_rate_ns, double gap_factor, int hit_mult_avg, int hit_mult_dev, int random_seed)
{
  mBunchCrossingRateNs = BC_rate_ns;
  mGapFactor = gap_factor;

  calculateAverageCrossingRate();
  
  mHitMultiplicityAverage = hit_mult_avg;
  mHitMultiplicityDeviation = hit_mult_dev;
  mRandomSeed = random_seed;
}

//@brief Calculate the actual average bunch crossing rate, when empty bunches are taken into account
void EventGenerator::calculateAverageCrossingRate(void)
{
  mAverageCrossingRateNs = mBunchCrossingRateNs * (1.0 - mGapFactor);
}


//@brief Limit the number of events stored in memory, as specified by mNumEventsInMemoryAllowed.
//       The oldest events will be removed to bring the count below the threshold. If mWriteEventsToDisk is
//       true, then the events that are removed will be written to disk.
void EventGenerator::eventMemoryCountLimiter(void)
{
  // If mNumEventsInMemoryAllowed == 0, then an infinite (limited by memory) amount of events are allowed.
  if((getEventsInMem() > mNumEventsInMemoryAllowed) &&
     (mNumEventsInMemoryAllowed > 0))
  {
    removeOldestEvent();
  }  
}


void EventGenerator::generateNextEvent()
{
  double t_delta;

  // Generate random (exponential distributed) interval till next event/interaction
  t_delta = 100*mRandEventTimeGen.Exp(mAverageCrossingRateNs);

  mLastEventTimeNs += t_delta;

  Event *next_event = new Event(mLastEventTimeNs, mEventCount);

  if(mEventCount > 0) {
    Event *previous_event = mEventQueue.back();
    next_event->eventCarryOver(*previous_event);
  }
  mEventQueue.push(next_event);

  // Generate hits for this event
  int n_hits = mRandHitMultiplicityGen.Gaus(mHitMultiplicityAverage, mHitMultiplicityDeviation);

  for(int i = 0; i < n_hits; i++) {
    // Generate hits here..
    int rand_chip_id = mRandHitGen.Integer(25000);
    int rand_x = mRandHitGen.Integer(1024);
    int rand_y = mRandHitGen.Integer(512);
    Hit h1(rand_chip_id, rand_x, rand_y);

    // Generate second hits.
    // @todo Implement random number of pixel digits per hit. 2.2 hits on average,
    //       gaussian distribution?
    rand_x = mRandHitGen.Integer(1024);
    rand_y = mRandHitGen.Integer(512);
    Hit h2(rand_chip_id, rand_x, rand_y);      

    next_event->addHit(h1);
    next_event->addHit(h2);      
  }

  mEventCount++;
  eventMemoryCountLimiter();
}


//@brief Generate a number of events.
//@param n_events Number of events to generate.
void EventGenerator::generateNextEvents(int n_events)
{
  for(int i = 0; i < n_events; i++)
    generateNextEvent();
}


const Event& EventGenerator::getNextEvent(void) const
{
  if(getEventsInMem() > 0) {
    Event* oldest_event = mEventQueue.front();
    return *oldest_event;
  } else {
    return NoEvent;
  }
}


//@brief Sets the bunch crossing rate, and recalculates the average crossing rate.
void EventGenerator::setBunchCrossingRate(int rate_ns)
{
  mBunchCrossingRateNs = rate_ns;
  calculateAverageCrossingRate();
}


//@brief Sets the empty bunch factor, and recalculates the average crossing rate.
//@todo For more accuracy, maybe this factor and average crossing rate should be replaced
//      with an actual filling pattern?
void EventGenerator::setBunchGapFactor(double factor)
{
  mGapFactor = factor;
  calculateAverageCrossingRate();
}


//@brief Sets the random seed used by random number generators.
void EventGenerator::setRandomSeed(int seed)
{
  mRandomSeed = seed;
  //@todo More than one seed? What if seed is set after random number generators have been started?
}


//@brief Initialize random number generators
void EventGenerator::initRandomNumGenerator(void)
{
  //@todo Do I need to have separate random number generators for each random distribution?
  //@todo Use different random seed for each generator?
  mRandHitGen.SetSeed(mRandomSeed);
  mRandHitMultiplicityGen.SetSeed(mRandomSeed);
  mRandEventTimeGen.SetSeed(mRandomSeed);
}


//@brief Set the total number of events that can be in memory at a time. If the count goes above this threshold,
//       the oldest events will be deleted (and possibly written to disk if disk writing is enabled).
//@param n Number of events allowed in memory. If set to 0, infinite number (limited by memory) is allowed.
void EventGenerator::setNumEventsInMemAllowed(int n)
{
  mNumEventsInMemoryAllowed = n;
}


//@brief Remove the oldest event from the event queue
//       (if there are any events in the queue, otherwise do nothing). 
void EventGenerator::removeOldestEvent(void)
{
  if(getEventsInMem() > 0) {
    Event *oldest_event = mEventQueue.front();
    mEventQueue.pop();      

    if(mWriteEventsToDisk)
      oldest_event->writeToFile(mDataPath);
      
    delete oldest_event;
  }
}
