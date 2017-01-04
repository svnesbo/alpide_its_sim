/**
 * @file   event_generator.cpp
 * @Author Simon Voigt Nesbo
 * @date   December 22, 2016
 * @brief  A simple event generator for Alpide SystemC simulation model.
 *         Physics events are generated at a rate that has an exponential distribution,
 *         with Lambda = 1 / average rate.
 *
 *         The number of hits generated (hit multiplicity) per event can be based on a gaussian distribution
 *         or a user-defined discrete distribution. The ROOT macro export_multiplicity_data.cxx found under
 *         process/Multiplicity_distribution can be used to generate a discrete distribution based on
 *         real multiplicity data from ALICE.
 * 
 *         The hits will currently be disributed randomly (with a flat/uniform distribution) among the
 *         different chips and over a chip's x/y coordinates.
 *         For each hit a fixed 2x2 pixel cluster is generated on the chip (this might be replaced with a
 *         more advanced random distribution in the future).
 *
 * @todo   Get rid off all the constructors. Lets only have one, and configure certain things (like what
 *         distribution for multiplicity to use) with functions. There is so much copy-paste between the
 *         3 constructors right now, which is hard to keep up to date everywhere.
 */

#include "event_generator.h"
#include "../alpide/alpide_constants.h"
#include <boost/current_function.hpp>
#include <stdexcept>
#include <cmath>

using boost::random::uniform_int_distribution;
using boost::random::normal_distribution;
using boost::random::exponential_distribution;
using boost::random::discrete_distribution;


#define print_function_timestamp() \
  std::cout << std::endl << "@ " << sc_time_stamp().value() << " ns\t";  \
  std::cout << BOOST_CURRENT_FUNCTION << ":" << std::endl; \
  std::cout << "-------------------------------------------"; \
  std::cout << "-------------------------------------------" << std::endl;



//@brief "Default" constructor for EventGenerator
//@param name    SystemC module name
SC_HAS_PROCESS(EventGenerator);
EventGenerator::EventGenerator(sc_core::sc_module_name name)
  : sc_core::sc_module(name)  
{
  mBunchCrossingRateNs = 25;  

  mAverageEventRateNs = 2500;

  mStrobeLengthNs = 5000;
  mPixelDeadTime = 500;
  mPixelActiveTime = 5000;
  
  mHitMultiplicityGaussAverage = 2000;
  mHitMultiplicityGaussDeviation = 350;
  
  mRandomSeed = 0;

  mRandHitChipID = new uniform_int_distribution<int>(0, N_CHIPS-1);
  mRandHitChipX = new uniform_int_distribution<int>(0, N_PIXEL_COLS-1);
  mRandHitChipY = new uniform_int_distribution<int>(0, N_PIXEL_ROWS-1);
  mRandHitMultiplicityGauss = new normal_distribution<double>(mHitMultiplicityGaussAverage,
                                                              mHitMultiplicityGaussDeviation);

  // Discrete distribution is not used in this case
  mRandHitMultiplicityDiscrete = nullptr;  

  // Multiplied by BC rate so that the distribution is related to the clock cycles
  // Which is fine because physics events will be in sync with 40MHz BC clock, but
  // to get actual simulation time we must multiply the numbers obtained with BC rate.
  double lambda = 1.0/(mAverageEventRateNs/mBunchCrossingRateNs);
  mRandEventTime = new exponential_distribution<double>(lambda);

  initRandomNumGenerator();

  mWriteRandomDataToFile = false;

  //////////////////////////////////////////////////////////////////////////////
  // SystemC declarations / connections / etc.
  //////////////////////////////////////////////////////////////////////////////
  SC_CTHREAD(physicsEventProcess, s_clk_in.pos());
  
  SC_METHOD(triggerEventProcess);
  sensitive << s_strobe_in;    
}

//@brief Constructor for EventGenerator with gaussian multiplicity distribution.
//@param name    SystemC module name
//@param BC_rate_ns Bunch crossing rate in nanoseconds.
//@param avg_event_rate_ns Average event rate in nanoseconds.
//@param strobe_length_ns Strobe length in nanoseconds
//@param hit_mult_avg Average hit multiplicity. Then number of hits generated for an event will be
//                    gaussian distributed, with this value as the mean, and hit_mult_dev as the
//                    standard deviation.
//@param hit_mult_dev Standard deviation for hit multiplicity.
//@param pixel_dead_time_ns Pixel shaping dead time in nanoseconds (ie. time before analog
//                          pulse goes above threshold).
//@param pixel_active_time_ns Pixel shaping active time in nanoseconds (ie. the amount of time
//                            the analog pulse is above the threshold).
//@param random_seed Random seed to use in random number generators.
//@param create_csv_file Create a CSV-file and store each event time and hit multiplicity value to
//                       this file
SC_HAS_PROCESS(EventGenerator);
EventGenerator::EventGenerator(sc_core::sc_module_name name,
                               int BC_rate_ns, int avg_event_rate_ns, int strobe_length_ns,
                               int hit_mult_avg, int hit_mult_dev,
                               int pixel_dead_time_ns, int pixel_active_time_ns,
                               int random_seed, bool create_csv_hit_file)
  : sc_core::sc_module(name)  
{
  mBunchCrossingRateNs = BC_rate_ns;

  mAverageEventRateNs = avg_event_rate_ns;
  mStrobeLengthNs = strobe_length_ns;
  mPixelDeadTime = pixel_dead_time_ns;
  mPixelActiveTime = pixel_active_time_ns;  
  mHitMultiplicityGaussAverage = hit_mult_avg;
  mHitMultiplicityGaussDeviation = hit_mult_dev;
  mRandomSeed = random_seed;

  mRandHitChipID = new uniform_int_distribution<int>(0, N_CHIPS-1);
  mRandHitChipX = new uniform_int_distribution<int>(0, N_PIXEL_COLS-1);
  mRandHitChipY = new uniform_int_distribution<int>(0, N_PIXEL_ROWS-1);
  mRandHitMultiplicityGauss = new normal_distribution<double>(mHitMultiplicityGaussAverage,
                                                                             mHitMultiplicityGaussDeviation);

  // Discrete distribution is not used in this case
  mRandHitMultiplicityDiscrete = nullptr;

  // Multiplied by BC rate so that the distribution is related to the clock cycles
  // Which is fine because physics events will be in sync with 40MHz BC clock, but
  // to get actual simulation time we must multiply the numbers obtained with BC rate.
  double lambda = 1.0/(mAverageEventRateNs/mBunchCrossingRateNs);
  mRandEventTime = new exponential_distribution<double>(lambda);  

  initRandomNumGenerator();

  mWriteRandomDataToFile = create_csv_hit_file;
  
  if(mWriteRandomDataToFile) {
    mRandDataFile.open("random_data.csv");
    mRandDataFile << "delta_t;hit_multiplicity" << std::endl;
  }

  
  //////////////////////////////////////////////////////////////////////////////
  // SystemC declarations / connections / etc.
  //////////////////////////////////////////////////////////////////////////////
  SC_CTHREAD(physicsEventProcess, s_clk_in.pos());
  
  SC_METHOD(triggerEventProcess);
  sensitive << s_strobe_in;  
}


//@brief Constructor for EventGenerator with discrete multiplicity distribution.
//@param name    SystemC module name
//@param BC_rate_ns Bunch crossing rate in nanoseconds.
//@param avg_event_rate_ns Average event rate in nanoseconds.
//@param strobe_length_ns Strobe length in nanoseconds
//@param mult_dist_filename Absolute or relative filename/path to discrete distribution file.
//                          The file format is rows with two values, X value (hit) and Y value (Probability),
//                          separated with whitespace.
//@param pixel_dead_time_ns Pixel shaping dead time in nanoseconds (ie. time before analog
//                          pulse goes above threshold).
//@param pixel_active_time_ns Pixel shaping active time in nanoseconds (ie. the amount of time
//                            the analog pulse is above the threshold).
//@param random_seed Random seed to use in random number generators.
//@param create_csv_file Create a CSV-file and store each event time and hit multiplicity value to this file
SC_HAS_PROCESS(EventGenerator);
EventGenerator::EventGenerator(sc_core::sc_module_name name,
                               int BC_rate_ns, int avg_event_rate_ns, int strobe_length_ns,
                               const char* mult_dist_filename, int random_seed,
                               int pixel_dead_time_ns, int pixel_active_time_ns,                               
                               bool create_csv_hit_file)
  : sc_core::sc_module(name)    
{
  mBunchCrossingRateNs = BC_rate_ns;

  mAverageEventRateNs = avg_event_rate_ns;
  mStrobeLengthNs = strobe_length_ns;
  mPixelDeadTime = pixel_dead_time_ns;
  mPixelActiveTime = pixel_active_time_ns;
  mRandomSeed = random_seed;

  mRandHitChipID = new uniform_int_distribution<int>(0, N_CHIPS-1);
  mRandHitChipX = new uniform_int_distribution<int>(0, N_PIXEL_COLS-1);
  mRandHitChipY = new uniform_int_distribution<int>(0, N_PIXEL_ROWS-1);

  // Read multiplicity distribution from file,
  // and initialize boost::random discrete distribution with data
  std::vector<double> mult_dist;
  readDiscreteDistributionFile(mult_dist_filename, mult_dist);
  mRandHitMultiplicityDiscrete = new discrete_distribution<>(mult_dist.begin(), mult_dist.end());

  // Gaussian distribution is not used in this case
  mRandHitMultiplicityGauss = nullptr;
  
  // Multiplied by BC rate so that the distribution is related to the clock cycles
  // Which is fine because physics events will be in sync with 40MHz BC clock, but
  // to get actual simulation time we must multiply the numbers obtained with BC rate.
  double lambda = 1.0/(mAverageEventRateNs/mBunchCrossingRateNs);
  mRandEventTime = new exponential_distribution<double>(lambda);

  std::cout << "mBunchCrossingRateNs = " << mBunchCrossingRateNs << std::endl;  
  std::cout << "mAverageEventRateNs = " << mAverageEventRateNs << std::endl;
  std::cout << "lambda = " << lambda << std::endl;

  initRandomNumGenerator();

  mWriteRandomDataToFile = create_csv_hit_file;
  
  if(mWriteRandomDataToFile) {
    mRandDataFile.open("random_data.csv");
    mRandDataFile << "delta_t;hit_multiplicity" << std::endl;
  }


  //////////////////////////////////////////////////////////////////////////////
  // SystemC declarations / connections / etc.
  //////////////////////////////////////////////////////////////////////////////
  SC_CTHREAD(physicsEventProcess, s_clk_in.pos());
  
  SC_METHOD(triggerEventProcess);
  sensitive << s_strobe_in;
}


EventGenerator::~EventGenerator()
{
  delete mRandHitChipID;
  delete mRandHitChipX;
  delete mRandHitChipY;
  delete mRandEventTime;

  // Note: safe to delete nullptr in C++
  delete mRandHitMultiplicityGauss;
  delete mRandHitMultiplicityDiscrete;    

  if(mRandDataFile.is_open())
    mRandDataFile.close();
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


//@todo Remove
//@brief Generate a number of events.
//@param n_events Number of events to generate.
// void EventGenerator::generateNextEvents(int n_events)
// {
//   for(int i = 0; i < n_events; i++)
//     generateNextEvent();
//}


//@brief Get a reference to the next event (if there is one)
//@return Reference to next event. If there are no events,
//        then a refernce to NoEvent (with event id = -1) is returned.
const TriggerEvent& EventGenerator::getNextTriggerEvent(void) const
{
  if(getEventsInMem() > 0) {
    TriggerEvent* oldest_event = mEventQueue.front();
    return *oldest_event;
  } else {
    return NoTriggerEvent;
  }
}


//@brief Sets the bunch crossing rate, and recalculates the average crossing rate.
void EventGenerator::setBunchCrossingRate(int rate_ns)
{
  mBunchCrossingRateNs = rate_ns;
}


//@brief Sets the random seed used by random number generators.
void EventGenerator::setRandomSeed(int seed)
{
  mRandomSeed = seed;
  //@todo More than one seed? What if seed is set after random number generators have been started?

  initRandomNumGenerator();
}


//@brief Initialize random number generators
void EventGenerator::initRandomNumGenerator(void)
{
  //@todo Do I need to have separate random number generators for each random distribution?
  //@todo Use different random seed for each generator?
  mRandHitGen.seed(mRandomSeed);
  mRandHitMultiplicityGen.seed(mRandomSeed);
  mRandEventTimeGen.seed(mRandomSeed);
}


//todo Remove?
//@brief Set the total number of events that can be in memory at a time. If the count goes above this threshold,
//       the oldest events will be deleted (and possibly written to disk if disk writing is enabled).
//@param n Number of events allowed in memory. If set to 0, infinite number (limited by memory) is allowed.
// void EventGenerator::setNumEventsInMemAllowed(int n)
// {
//   mNumEventsInMemoryAllowed = n;
// }


//@brief Remove the oldest event from the event queue
//       (if there are any events in the queue, otherwise do nothing). 
void EventGenerator::removeOldestEvent(void)
{
  if(getEventsInMem() > 0) {
    TriggerEvent *oldest_event = mEventQueue.front();
    mEventQueue.pop();      

    if(mWriteEventsToDisk)
      oldest_event->writeToFile(mDataPath);
      
    delete oldest_event;
  }
}


//@brief Read a discrete distribution from file and store it in a vector. The file format is a simple text file,
//       with the following format:
//       X0 Y0
//       X1 Y1
//       ...
//       Xn Yn
//
//       Where X-values correspond to the possible range of values for the random distribution, and the Y-values
//       correspond to probability for a given X-value. X and Y is separated by whitespace.
//       All X-values must be unsigned integers, and Y-values are assumed to be (positive) floating point.
//
//       The boost::random::discrete_distribution expects a list of probability values, where the index in the list
//       corresponds to the X-value. This function generates a vector to represent that list. Missing X-values
//       is allowed in the file, for example:
//       0 0.12
//       1 0.23
//       3 0.45
//
//       In the above example, an entry for the X-value of 2 with probability (Y) 0.0 will be inserted to the
//       vector by this function.
//
//@param filename Relative or absolute path and filename to open
//@param dist_vector Reference to vector to store the distribution in
//@throw domain_error If a negative x-value (hits) or y-value (probability) is encountered in the file
void EventGenerator::readDiscreteDistributionFile(const char* filename, std::vector<double> &dist_vector) const
{
  std::ifstream in_file(filename);

  if(in_file.is_open() == false) {
    std::cerr << "Error opening discrete distribution file." << std::endl;
    throw std::runtime_error("Error opening discrete distribution file.");
  }

  int i = 0;
  int x;
  double y;
  
  while((in_file >> x >> y)) {
    if(x < 0)
      throw std::domain_error("Negative x-value in discrete distribution file");

    if(y < 0.0)
      throw std::domain_error("Negative probability-value in discrete distribution file");
    
    // Some bins/x-values may be missing in the file.
    // Missing bins have zero probability, but need to be present in the
    // vector because discrete_distribution expects the full range
    while(i < x) {
      dist_vector.push_back(0);
      i++;
    }

    dist_vector.push_back(y);
    i++;
  }

  in_file.close();
}


//@brief Return a random number of hits (multiplicity) based on the chosen
//       distribution for multiplicity.
//@return Number of hits
//@throw  runtime_error if the EventGenerator for some reason does not have
//                      a multiplicity distribution initialized.
unsigned int EventGenerator::getRandomMultiplicity(void)
{
  if(mRandHitMultiplicityDiscrete != nullptr)
    return (*mRandHitMultiplicityDiscrete)(mRandHitMultiplicityGen);
  
  else if(mRandHitMultiplicityGauss != nullptr)
    return (*mRandHitMultiplicityGauss)(mRandHitMultiplicityGen);

  // Has no distribution for multiplicity been initialized? Throw an error..
  else
    throw std::runtime_error("No multiplicity distribution initialized.");
}


//@brief Generate the next physics event (in the future).
//       1) Generate time till the next physics event
//       2) Generate hits for the next event, and put them on the hit queue
//       3) Update counters etc.
//@return The number of clock cycles until this event will actually occur
int64_t EventGenerator::generateNextPhysicsEvent(void)
{
  int64_t t_delta, t_delta_cycles;

  // Generate random (exponential distributed) interval till next event/interaction
  // The exponential distribution only works with double float, that's why it is rounded
  // to nearest clock cycle. Which is okay, because events in LHC should be synchronous
  // with bunch crossing clock anyway?
  // Add 1 because otherwise we risk getting events with 0 t_delta, which obviously is not
  // physically possible, and also SystemC doesn't allow wait() for 0 clock cycles.
  t_delta_cycles = std::round((*mRandEventTime)(mRandEventTimeGen)) + 1;
  t_delta = t_delta_cycles * mBunchCrossingRateNs;

  print_function_timestamp();
  std::cout << "\tPhysics event number: " << mPhysicsEventCount << std::endl;
  std::cout << "\tt_delta: " << t_delta << std::endl;
  std::cout << "\tt_delta_cycles: " << t_delta_cycles << std::endl;
  std::cout << "\tmLastPhysicsEventTimeNs: " << mLastPhysicsEventTimeNs << std::endl;

  mLastPhysicsEventTimeNs += t_delta;
  mPhysicsEventCount++;

  // Generate a random number of hits for this event
  int n_hits = getRandomMultiplicity();

  // Write data to CSV file (if we are generating CSV).
  if(mWriteRandomDataToFile) {
    mRandDataFile << t_delta << ";" << n_hits << std::endl;
  }


  // Generate hits here
  for(int i = 0; i < n_hits; i++) {
    int rand_chip_id = (*mRandHitChipID)(mRandHitGen);
    int rand_x1 = (*mRandHitChipX)(mRandHitGen);
    int rand_y1 = (*mRandHitChipY)(mRandHitGen);
    int rand_x2, rand_y2;


    // Very simple and silly method for making 2x2 pixel cluster
    // Makes sure that we don't get pixels below row/col 0, and not above
    // row 511 or above column 1023
    if(rand_x1 < N_PIXEL_COLS/2) {
      rand_x2 = rand_x1+1;
    } else {
      rand_x2 = rand_x1-1;
    }

    if(rand_y1 < N_PIXEL_ROWS/2) {
      rand_y2 = rand_y1+1;
    } else {
      rand_y2 = rand_y1-1;
    }

    // Create hit objects directly at the back of the deque,
    // without a copy or move taking place.
    mHitQueue.emplace_back(rand_chip_id, rand_x1, rand_y1, mLastPhysicsEventTimeNs,
                           mPixelDeadTime, mPixelActiveTime);
    mHitQueue.emplace_back(rand_chip_id, rand_x1, rand_y2, mLastPhysicsEventTimeNs,
                           mPixelDeadTime, mPixelActiveTime);
    mHitQueue.emplace_back(rand_chip_id, rand_x2, rand_y1, mLastPhysicsEventTimeNs,
                           mPixelDeadTime, mPixelActiveTime);
    mHitQueue.emplace_back(rand_chip_id, rand_x2, rand_y2, mLastPhysicsEventTimeNs,
                           mPixelDeadTime, mPixelActiveTime);
  }

  //@todo Remove?
  //eventMemoryCountLimiter();

  return t_delta_cycles;
}


//@brief Create a new trigger event at the given start time. It checks if trigger event
//       should be filtered or not, and updates trigger ID count. 
//@param event_start Start time of trigger event (time when strobe signal went high).
TriggerEvent* EventGenerator::generateNextTriggerEvent(int64_t event_start)
{
  //@todo Should I check distance between start time of two triggers?
  //      Or the distance in time between the end of the first trigger and the
  //      start of the next trigger?
  int64_t time_since_last_trigger = event_start - mLastTriggerEventStartTimeNs;
  
  // If event/trigger filtering is enabled, and this event/trigger came
  // too close to the previous one we filter it out.
  bool filter_event = mTriggerFilteringEnabled ? (time_since_last_trigger < mTriggerFilterTimeNs) : false;

  // But don't filter the first trigger event
  if(mTriggerEventIdCount == 0)
    filter_event = false;

  TriggerEvent* e = new TriggerEvent(event_start, mTriggerEventIdCount, filter_event);

  mTriggerEventIdCount++;

  print_function_timestamp();
  std::cout << "\tTrigger event number: " << mTriggerEventIdCount << std::endl;
  std::cout << "\ttime_since_last_trigger: " << time_since_last_trigger << std::endl;
  std::cout << "\tevent_start: " << event_start << std::endl;
  std::cout << "\tmLastTriggerEventStartTimeNs: " << mLastTriggerEventStartTimeNs << std::endl;
  std::cout << "\tmTriggerFilterTimeNs: " << mTriggerFilterTimeNs << std::endl;
  std::cout << "\tFiltered: " << (filter_event ? "true" : "false") << std::endl;
  

  return e;
}


//@brief Remove old hits.
//       Start at the front of the hit queue, and pop (remove)
//       hits from the front while the hits are no longer active at current simulation time,
//       and older than the oldest trigger event (so we don't delete hits that may be
//       still be used in a trigger event that hasn't been processed yet).
void EventGenerator::removeInactiveHits(void)
{
  int64_t time_now = sc_time_stamp().value();
  bool done = false;

  print_function_timestamp();
  std::cout << "\tQueue size (before): " << mHitQueue.size() << std::endl;
  int i = 0;

  do {
    if(mHitQueue.size() > 0) {
      // Check if oldest hit should be removed..
      if(mHitQueue.front().isActive(time_now) == false &&
         mHitQueue.front().getActiveTimeEnd() < mLastTriggerEventEndTimeNs)
      {
        mHitQueue.pop_front();
        i++;
        done = false;
      } else {
        done = true; // Done if oldest hit is still active
      }
    } else {
      done = true; // Done if queue size is zero
    }
  } while(done == false);

  std::cout << "\tQueue size (now): " << mHitQueue.size() << "\t" << i << " hits removed" << std::endl;
}


//@brief SystemC controled method, should be sensitive to the positive edge
//       of the clock. Responsible for
//       1) Creating new physics events (hits)
//       2) Deleting old inactive hits
void EventGenerator::physicsEventProcess(void)
{
  int64_t t_delta_cycles;

  // Run until the simulation stops
  while(1) {
    // Generate next physics event. This event will be t_delta_cycles in the future.
    t_delta_cycles = generateNextPhysicsEvent();

    // Wait for t_delta_cycles number of clock cycles, which is when the next physics event will
    // actually happen.
    // Note: this type of wait() call only works with CTHREAD (normally one would specify time unit).
    wait(t_delta_cycles);

    //@todo Maybe do this only on strobe falling edge? Saves some CPU cycles that way?
    removeInactiveHits();
  }
}


//@brief SystemC controlled method. It should be sensitive to the strobe signal,
//       (both rising and falling edge) and is responsible for the following:
//       1) Create a new trigger class object on rising edge
//       2) Complete trigger object on falling edge, and add to queue
void EventGenerator::triggerEventProcess(void)
{
  int64_t time_now = sc_time_stamp().value();

  print_function_timestamp();
      
  // Rising edge
  if(s_strobe_in.read() == true) {
    mNextTriggerEvent = generateNextTriggerEvent(time_now);
  }

  // Falling edge. Also check that we've actually allocated something..
  else if(mNextTriggerEvent != nullptr) {
    mNextTriggerEvent->setTriggerEventEndTime(time_now);
    
    // Only add hits to the event if it is not being filtered
    if(mNextTriggerEvent->getEventFilteredFlag() == false) {
      addHitsToTriggerEvent(*mNextTriggerEvent);
      mLastTriggerEventStartTimeNs = mNextTriggerEvent->getEventStartTime();
      mLastTriggerEventEndTimeNs = mNextTriggerEvent->getEventEndTime();
      std::cout << "\tTrigger start time: " << mLastTriggerEventStartTimeNs << " ns. " << std::endl;
      std::cout << "\tEnd time: " << mLastTriggerEventEndTimeNs << " ns." << std::endl;
    }
    
    mEventQueue.push(mNextTriggerEvent);
    mNextTriggerEvent = nullptr;

    // Post an event notification that a new trigger event/frame is ready
    E_trigger_event_available->notify(SC_ZERO_TIME);
    
    std::cout << "\tTrigger event queue size: " << mEventQueue.size() << std::endl;
  }
}


//@brief Iterate through the EventGenerator's hit queue, and add active hits to
//       the event referenced by e.
//@param e Event to add hits to.
void EventGenerator::addHitsToTriggerEvent(TriggerEvent& e)
{
  for(auto it = mHitQueue.begin(); it != mHitQueue.end(); it++) {
    // All the hits are ordered in time in the hit queue.
    // If this hit is not active, it could be that:
    // 1) We haven't reached the newer hits which would be active for this event yet
    // 2) We have gone through the hits that are active for this event, and have now
    //    reached hits that are "too new" (event queue size is larger than 0 then)
    if(it->isActive(e.getEventStartTime(), e.getEventEndTime())) {
      e.addHit(*it);
    } else if (e.getEventSize() > 0) {
      // Case 2. There won't be any more hits now, so we can break.
      // @todo Is this check worth it performance wise, or is it better to just iterate
      //       through the whole list?
      break;
    }
  }
}
