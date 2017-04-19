/**
 * @file   event_generator.cpp
 * @author Simon Voigt Nesbo
 * @date   December 22, 2016
 * @details A simple event generator for Alpide SystemC simulation model.
 */

#include "event_generator.h"
#include "../alpide/alpide_constants.h"
#include <boost/current_function.hpp>
#include <boost/random/random_device.hpp>
#include <stdexcept>
#include <cmath>
#include <map>


using boost::random::uniform_int_distribution;
using boost::random::normal_distribution;
using boost::random::exponential_distribution;
using boost::random::discrete_distribution;


#define print_function_timestamp() \
  std::cout << std::endl << "@ " << sc_time_stamp().value() << " ns\t";  \
  std::cout << BOOST_CURRENT_FUNCTION << ":" << std::endl; \
  std::cout << "-------------------------------------------"; \
  std::cout << "-------------------------------------------" << std::endl;


SC_HAS_PROCESS(EventGenerator);
///@brief Constructor for EventGenerator with gaussian multiplicity distribution.
///@param name SystemC module name
///@param settings QSettings object with simulation settings.
///@param output_path Directory path to store simulation output data in
EventGenerator::EventGenerator(sc_core::sc_module_name name,
                               const QSettings* settings,
                               std::string output_path)
  : sc_core::sc_module(name)  
{
  mOutputPath = output_path;
  mBunchCrossingRateNs = settings->value("event/bunch_crossing_rate_ns").toInt();
  mAverageEventRateNs = settings->value("event/average_event_rate_ns").toInt();  
  mRandomSeed = settings->value("simulation/random_seed").toInt();
  mCreateCSVFile = settings->value("data_output/write_event_csv").toBool();
  mPixelDeadTime = settings->value("alpide/pixel_shaping_dead_time_ns").toInt();
  mPixelActiveTime = settings->value("alpide/pixel_shaping_active_time_ns").toInt();
  mNumChips = settings->value("simulation/n_chips").toInt();

  mContinuousMode = settings->value("simulation/continuous_mode").toBool();
  mTriggerFilterTimeNs = settings->value("event/trigger_filter_time_ns").toInt();  

  // Trigger filtering only allowed in triggered mode
  if(mContinuousMode == false) {
    mTriggerFilteringEnabled = settings->value("event/trigger_filter_enable").toBool();
  } else {
    mTriggerFilteringEnabled = false;
  }

  mEventQueue.resize(mNumChips);
  mHitQueue.resize(mNumChips);
  
  // Instantiate event generator object with the desired hit multiplicity distribution
  QString multipl_dist_type = settings->value("event/hit_multiplicity_distribution_type").toString();  
  if(multipl_dist_type == "gauss") {
    mHitMultiplicityGaussAverage = settings->value("event/hit_multiplicity_gauss_avg").toInt();  
    mHitMultiplicityGaussDeviation = settings->value("event/hit_multiplicity_gauss_stddev").toInt();

    mRandHitMultiplicityGauss = new normal_distribution<double>(mHitMultiplicityGaussAverage,
                                                                             mHitMultiplicityGaussDeviation);

    // Discrete distribution is not used in this case
    mRandHitMultiplicityDiscrete = nullptr;
  }
  else if(multipl_dist_type == "discrete") {
    QString multipl_dist_file = settings->value("event/hit_multiplicity_distribution_file").toString();

    // Read multiplicity distribution from file,
    // and initialize boost::random discrete distribution with data
    std::vector<double> mult_dist;
    readDiscreteDistributionFile(multipl_dist_file.toStdString().c_str(), mult_dist);

    // Calculate the average number of hits in an event,
    // assuming that all chips here are on the same layer.
    double hits_per_cm2 = settings->value("event/hit_density_min_bias_per_cm2").toDouble();
    double alpide_chip_area = CHIP_WIDTH_CM*CHIP_HEIGHT_CM;
    double its_layer_area = mNumChips * alpide_chip_area;
    double avg_hits_per_event =  hits_per_cm2 * its_layer_area;

    std::cout << "hits_per_cm2: " << hits_per_cm2;
    std::cout << "\talpide_chip_area: " << alpide_chip_area;
    std::cout << "\tits_layer_area: " << its_layer_area;
    std::cout << "\tavg_hits_per_event: " << avg_hits_per_event << std::endl;
    std::cout << "Number of bins in distribution before scaling: " << mult_dist.size() << std::endl;
    scaleDiscreteDistribution(mult_dist, avg_hits_per_event);
    std::cout << "Number of bins in distribution after scaling: " << mult_dist.size() << std::endl;
    
    mRandHitMultiplicityDiscrete = new discrete_distribution<>(mult_dist.begin(), mult_dist.end());

    // Gaussian distribution is not used in this case
    mRandHitMultiplicityGauss = nullptr;    
  }

  mRandHitChipID = new uniform_int_distribution<int>(0, mNumChips-1);
  mRandHitChipX = new uniform_int_distribution<int>(0, N_PIXEL_COLS-1);
  mRandHitChipY = new uniform_int_distribution<int>(0, N_PIXEL_ROWS-1);

  
  // Multiplied by BC rate so that the distribution is related to the clock cycles
  // Which is fine because physics events will be in sync with 40MHz BC clock, but
  // to get actual simulation time we must multiply the numbers obtained with BC rate.
  double lambda = 1.0/(mAverageEventRateNs/mBunchCrossingRateNs);
  mRandEventTime = new exponential_distribution<double>(lambda);  

  initRandomNumGenerator();

  if(mCreateCSVFile) {
    std::string physics_events_csv_filename = mOutputPath + std::string("/physics_events_data.csv");
    mPhysicsEventsCSVFile.open(physics_events_csv_filename);
    mPhysicsEventsCSVFile << "delta_t;hit_multiplicity";
    for(int i = 0; i < mNumChips; i++)
      mPhysicsEventsCSVFile << ";chip_" << i << "_trace_hits";
    for(int i = 0; i < mNumChips; i++)
      mPhysicsEventsCSVFile << ";chip_" << i << "_pixel_hits";
    mPhysicsEventsCSVFile << std::endl;

    std::string trigger_events_csv_filename = mOutputPath + std::string("/trigger_events_data.csv");    
    mTriggerEventsCSVFile.open(trigger_events_csv_filename);
    mTriggerEventsCSVFile << "time";
    mTriggerEventsCSVFile << ";filtered";
    for(int i = 0; i < mNumChips; i++)
      mTriggerEventsCSVFile << ";chip_" << i << "_pixel_hits";
    mTriggerEventsCSVFile << std::endl;
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

  if(mPhysicsEventsCSVFile.is_open())
    mPhysicsEventsCSVFile.close();
  if(mTriggerEventsCSVFile.is_open())
    mTriggerEventsCSVFile.close();  
}


///@brief Limit the number of events stored in memory, as specified by mNumEventsInMemoryAllowed.
///       The oldest events will be removed to bring the count below the threshold. If mWriteEventsToDisk is
///       true, then the events that are removed will be written to disk.
void EventGenerator::eventMemoryCountLimiter(void)
{
  // If mNumEventsInMemoryAllowed == 0, then an infinite (limited by memory) amount of events are allowed.
  if((getEventsInMem() > mNumEventsInMemoryAllowed) &&
     (mNumEventsInMemoryAllowed > 0))
  {
    removeOldestEvent();
  }  
}


///@brief Get a reference to the next event (if there is one). Note: this function
///       will keep returning the same event until it has been removed by removeOldestEvent().
///@return Reference to next event. If there are no events,
///        then a reference to NoTriggerEvent (with event id = -1) is returned.
const TriggerEvent& EventGenerator::getNextTriggerEvent(void)
{
  // Start where we left off
  int chip_id = mNextTriggerEventChipId;

  // Find first available TriggerEvent and return it
  while(chip_id < mNumChips) {
    if(mEventQueue[chip_id].size() > 0) {
      return *(mEventQueue[chip_id].front());
    }

    chip_id++;
    mNextTriggerEventChipId = chip_id;
  }

  return NoTriggerEvent;  
}


///@brief Sets the bunch crossing rate, and recalculates the average crossing rate.
void EventGenerator::setBunchCrossingRate(int rate_ns)
{
  mBunchCrossingRateNs = rate_ns;
}


///@brief Sets the random seed used by random number generators.
void EventGenerator::setRandomSeed(int seed)
{
  mRandomSeed = seed;
  ///@todo More than one seed? What if seed is set after random number generators have been started?

  initRandomNumGenerator();
}


///@brief Initialize random number generators
void EventGenerator::initRandomNumGenerator(void)
{  
  // If seed was set to 0 in settings file, initialize with a non-deterministic
  // higher entropy random number using boost::random::random_device
  // Based on this: http://stackoverflow.com/a/13004555
  if(mRandomSeed == 0) {
    boost::random::random_device r;
    
    std::cout << "Boost random_device entropy: " << r.entropy() << std::endl;
    
    unsigned int random_seed = r();
    mRandHitGen.seed(random_seed);
    std::cout << "Hit coordinates generator random seed: " << random_seed << std::endl;

    random_seed = r();
    mRandHitMultiplicityGen.seed(random_seed);
    std::cout << "Hit multiplicity generator random seed: " << random_seed << std::endl;
    
    random_seed = r();
    mRandEventTimeGen.seed(random_seed);
    std::cout << "Event rate generator random seed: " << random_seed << std::endl;
  } else {
    mRandHitGen.seed(mRandomSeed);
    mRandHitMultiplicityGen.seed(mRandomSeed);
    mRandEventTimeGen.seed(mRandomSeed);    
  }
}


///@brief Remove the oldest event from the event queue
///       (if there are any events in the queue, otherwise do nothing). 
void EventGenerator::removeOldestEvent(void)
{ 
  // Start where we left off
  int chip_id = mNextTriggerEventChipId;

  // chip_id in range?
  if(chip_id < mNumChips) {
    // Event left to remove for this chip?
    if(mEventQueue[chip_id].size() > 0) {
      TriggerEvent *oldest_event = mEventQueue[chip_id].front();
      mEventQueue[chip_id].pop();      

      if(mWriteEventsToDisk)
        oldest_event->writeToFile(mDataPath);
      
      delete oldest_event;      
    }
  }
}


///@brief Read a discrete distribution from file and store it in a vector. The file format is a simple text file,
///       with the following format:
///       X0 Y0
///       X1 Y1
///       ...
///       Xn Yn
///
///       Where X-values correspond to the possible range of values for the random distribution, and the Y-values
///       correspond to probability for a given X-value. X and Y is separated by whitespace.
///       All X-values must be unsigned integers, and Y-values are assumed to be (positive) floating point.
///
///       The boost::random::discrete_distribution expects a list of probability values, where the index in the list
///       corresponds to the X-value. This function generates a vector to represent that list. Missing X-values
///       is allowed in the file, for example:
///       0 0.12
///       1 0.23
///       3 0.45
///
///       In the above example, an entry for the X-value of 2 with probability (Y) 0.0 will be inserted to the
///       vector by this function.
///
///@param filename Relative or absolute path and filename to open
///@param dist_vector Reference to vector to store the distribution in
///@throw runtime_error If the file can not be opened
///@throw domain_error If a negative x-value (hits) or y-value (probability) is encountered in the file
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


///@brief Scale the x axis of a discrete distribution,
///       so that the distribution gets a new mean value.
///@param dist_vector Distribution to scale. The original distribution in
///                   this vector will be overwritten and replaced with
///                   the new, scaled, distribution.
///@param new_mean_value The desired mean value of the new distribution.
///@throw runtime_error If dist_vector is empty, a runtime_error is thrown.
void EventGenerator::scaleDiscreteDistribution(std::vector<double> &dist_vector, double new_mean_value)
{
  double old_probability_sum = 0.0;
  double old_probability_sum_fixed = 0.0;
  double old_mean_value = 0.0;
  double old_mean_value_fixed = 0.0;  
  double resulting_mean_value = 0.0;
  std::vector<double> new_dist_vector;
  std::map<double, double> scaled_dist_map;

  if(dist_vector.empty())
    throw std::runtime_error("Discrete distribution to scale is empty.");
  
  // First calculate mean value in current distribution
  for(unsigned int i = 0; i < dist_vector.size(); i++) {
    old_mean_value += i*dist_vector[i];
    old_probability_sum += dist_vector[i];
  }
  std::cout << "Mean value in original distribution: " << old_mean_value << std::endl << std::endl;

  std::cout << "Probability sum/integral in original distribution: " << old_probability_sum << std::endl;

  // Normalize the area of the probability curve to 1.0
  for(unsigned int i = 0; i < dist_vector.size(); i++) {
    dist_vector[i] /= old_probability_sum;
    old_mean_value_fixed += i*dist_vector[i];    
    old_probability_sum_fixed += dist_vector[i];
  }

  std::cout << "Mean value in original distribution (fixed): " << old_mean_value_fixed << std::endl << std::endl;

  std::cout << "Probability sum/integral in original distribution (fixed): " << old_probability_sum_fixed << std::endl;  

  std::cout << "scaled_dist_map: " << std::endl;

  double scale_factor = new_mean_value / old_mean_value_fixed;

  // Size the new vector correctly
  // Calculate a position in the old distribution, as a floating point value. Since we only have discrete
  // values, use the decimal part to pick a certain amount from the previous index and the next index.
  new_dist_vector.resize(dist_vector.size()*scale_factor);
  double sum = 0.0;
  for(unsigned int x_new = 0; x_new < new_dist_vector.size(); x_new++) {
    // x_new: X value in new distribution (scaled)
    // x_old: X value in old distribution (before scaling)    
    double x_old_decimal = x_new/scale_factor;
    unsigned int x_old_int = (unsigned int)(x_old_decimal);
    double x_old_remainder = x_old_decimal-x_old_int;
    double y_new;

    // Use the first and last entry directly
    if(x_new == 0)
      y_new = dist_vector.front();
    else if(x_new == new_dist_vector.size()-1)
      y_new = dist_vector.back();
    else
      // If the new value essentially lies between two values in the old distribution
      // (after dividing by scaling factor), we calculate the this position would have if we
      // drew a straight line between the value before and after.
      y_new = dist_vector[x_old_int] + x_old_remainder*(dist_vector[x_old_int+1] - dist_vector[x_old_int]);

    new_dist_vector[x_new] = y_new;
    
    // Don't scale bin 0, because the probability of 0 hits should not change
    // just because the distribution is scaled to a higher mean value.
    if(x_new > 0)
      new_dist_vector[x_new] /= scale_factor;

    // Integrate over distribution vector as we go.
    // All bins have width 1, and ideally the probability should sum up to be 1.0.
    sum += new_dist_vector[x_new];
  }

  std::cout << "New distribution integral/sum: " << sum << std::endl;

  /// @todo This changes the mean value slightly.. and the sum isn't that far
  ///       off 1.0 before this anyway...
  /*
  double new_sum = 0.0;
  // Normalize the area of the probability curve to 1.0, because after scaling
  // the sum might have changed slightly. 
  for(unsigned int i = 0; i < new_dist_vector.size(); i++) {
    new_dist_vector[i] /= sum;
    new_sum += new_dist_vector[i];
  }
  std::cout << "New distribution normalized sum: " << new_sum << std::endl;
  */

  // Calculate mean value in new distribution
  for(unsigned int i = 0; i < new_dist_vector.size(); i++) {
    resulting_mean_value += i*new_dist_vector[i];
  }
  std::cout << "Mean value in new distribution: " << resulting_mean_value << std::endl << std::endl;  
  
  dist_vector = new_dist_vector;
}


///@brief Return a random number of hits (multiplicity) based on the chosen
///       distribution for multiplicity.
///@return Number of hits
///@throw  runtime_error if the EventGenerator for some reason does not have
///                      a multiplicity distribution initialized.
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


///@brief Generate the next physics event (in the future).
///       1) Generate time till the next physics event
///       2) Generate hits for the next event, and put them on the hit queue
///       3) Update counters etc.
///@return The number of clock cycles until this event will actually occur
int64_t EventGenerator::generateNextPhysicsEvent(void)
{
  int64_t t_delta, t_delta_cycles;

  // Initialize array to 0. http://stackoverflow.com/a/2204380/6444574
  int *chip_trace_hit_counts = new int[mNumChips]();
  int *chip_pixel_hit_counts = new int[mNumChips]();

  // Generate random (exponential distributed) interval till next event/interaction
  // The exponential distribution only works with double float, that's why it is rounded
  // to nearest clock cycle. Which is okay, because events in LHC should be synchronous
  // with bunch crossing clock anyway?
  // Add 1 because otherwise we risk getting events with 0 t_delta, which obviously is not
  // physically possible, and also SystemC doesn't allow wait() for 0 clock cycles.
  t_delta_cycles = std::round((*mRandEventTime)(mRandEventTimeGen)) + 1;
  t_delta = t_delta_cycles * mBunchCrossingRateNs;

  if((mPhysicsEventCount % 100) == 0) {
    //print_function_timestamp();
    int64_t time_now = sc_time_stamp().value();
    std::cout << "@ " << time_now << " ns: ";
    std::cout << "\tPhysics event number: " << mPhysicsEventCount;
    std::cout << "\tt_delta: " << t_delta;
    std::cout << "\tt_delta_cycles: " << t_delta_cycles;
    std::cout << "\tmLastPhysicsEventTimeNs: " << mLastPhysicsEventTimeNs << std::endl;
  }

  mLastPhysicsEventTimeNs += t_delta;
  mPhysicsEventCount++;

  // Generate a random number of hits for this event
  int n_hits = getRandomMultiplicity();

  
  // Generate hits here
  for(int i = 0; i < n_hits; i++) {
    int rand_chip_id = (*mRandHitChipID)(mRandHitGen);
    int rand_x1 = (*mRandHitChipX)(mRandHitGen);
    int rand_y1 = (*mRandHitChipY)(mRandHitGen);
    int rand_x2, rand_y2;

    chip_trace_hit_counts[rand_chip_id]++;

    ///@todo Account larger/bigger clusters here (when implemented)
    chip_pixel_hit_counts[rand_chip_id] += 4;

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
    mHitQueue[rand_chip_id].emplace_back(rand_x1, rand_y1, mLastPhysicsEventTimeNs,
                                         mPixelDeadTime, mPixelActiveTime);
    mHitQueue[rand_chip_id].emplace_back(rand_x1, rand_y2, mLastPhysicsEventTimeNs,
                                         mPixelDeadTime, mPixelActiveTime);
    mHitQueue[rand_chip_id].emplace_back(rand_x2, rand_y1, mLastPhysicsEventTimeNs,
                                         mPixelDeadTime, mPixelActiveTime);
    mHitQueue[rand_chip_id].emplace_back(rand_x2, rand_y2, mLastPhysicsEventTimeNs,
                                         mPixelDeadTime, mPixelActiveTime);
  }

  // Write event rate and multiplicity numbers to CSV file
  if(mCreateCSVFile) {
    mPhysicsEventsCSVFile << t_delta << ";" << n_hits;

    for(int i = 0; i < mNumChips; i++)
      mPhysicsEventsCSVFile << ";" << chip_trace_hit_counts[i];    
    for(int i = 0; i < mNumChips; i++)
      mPhysicsEventsCSVFile << ";" << chip_pixel_hit_counts[i];
    
    mPhysicsEventsCSVFile << std::endl;
  }
  
  delete chip_trace_hit_counts;
  delete chip_pixel_hit_counts;

  ///@todo Remove?
  //eventMemoryCountLimiter();

  return t_delta_cycles;
}


///@brief Remove old hits.
///       Start at the front of the hit queue, and pop (remove)
///       hits from the front while the hits are no longer active at current simulation time,
///       and older than the oldest trigger event (so we don't delete hits that may be
///       still be used in a trigger event that hasn't been processed yet).
void EventGenerator::removeInactiveHits(void)
{
  int64_t time_now = sc_time_stamp().value();
  bool done = false;
  int i = 0;
  
  #ifdef DEBUG_OUTPUT
  print_function_timestamp();
  std::cout << "\tQueue size (before): " << mHitQueue.size() << std::endl;
  #endif

  for(int chip_id = 0; chip_id < mNumChips; chip_id++) {
    do {
      if(mHitQueue[chip_id].size() > 0) {
        // Check if oldest hit should be removed..
        if(mHitQueue[chip_id].front().isActive(time_now) == false &&
           mHitQueue[chip_id].front().getActiveTimeEnd() < mLastTriggerEventEndTimeNs)
        {
          mHitQueue[chip_id].pop_front();
          i++;
          done = false;
        } else {
          done = true; // Done if oldest hit is still active
        }
      } else {
        done = true; // Done if queue size is zero
      }
    } while(done == false);
  }


  #ifdef DEBUG_OUTPUT
  ///@todo Fix this..
  std::cout << /*"\tQueue size (now): " << mHitQueue[chip_id].size() <<*/ "\t" << i << " hits removed" << std::endl;
  #endif
}


///@brief Create a new trigger event at the given start time. It checks if trigger event
///       should be filtered or not, and updates trigger ID count. 
///@param event_start Start time of trigger event (time when strobe signal went high).
///@param event_end End time of trigger event (time when strobe signal went low again).
///@param chip_id Chip ID to generate event for
///@return Pointer to new TriggerEvent object that was allocated on the stack.
///        Caller must remember to delete it when done in order to free memory.
TriggerEvent* EventGenerator::generateNextTriggerEvent(int64_t event_start, int64_t event_end, int chip_id)
{
  ///@todo Should I check distance between start time of two triggers?
  ///      Or the distance in time between the end of the first trigger and the
  ///      start of the next trigger?
  int64_t time_since_last_trigger = event_start - mLastTriggerEventStartTimeNs;
  
  // If event/trigger filtering is enabled, and this event/trigger came
  // too close to the previous one we filter it out.
  bool filter_event = mTriggerFilteringEnabled ? (time_since_last_trigger < mTriggerFilterTimeNs) : false;

  // But don't filter the first trigger event
  if(mTriggerEventIdCount == 0)
    filter_event = false;

  int event_id = mTriggerEventIdCount;
  TriggerEvent* e = new TriggerEvent(event_start, event_end, chip_id, event_id, filter_event);

  // Only add hits to the event if it is not being filtered
  if(e->getEventFilteredFlag() == false) {
    addHitsToTriggerEvent(*e);
  }  

  #ifdef DEBUG_OUTPUT
  print_function_timestamp();
  std::cout << "\tTrigger event number: " << mTriggerEventIdCount << std::endl;
  std::cout << "\ttime_since_last_trigger: " << time_since_last_trigger << std::endl;
  std::cout << "\tevent_start: " << event_start << std::endl;
  std::cout << "\tmLastTriggerEventStartTimeNs: " << mLastTriggerEventStartTimeNs << std::endl;
  std::cout << "\tmTriggerFilterTimeNs: " << mTriggerFilterTimeNs << std::endl;
  std::cout << "\tFiltered: " << (filter_event ? "true" : "false") << std::endl;
  #endif
  

  return e;
}


///@brief Iterate through the hit queue corresponding to the chip_id associated with
///       the event referenced by e, and add the active hits to it.
///@param e Event to add hits to.
void EventGenerator::addHitsToTriggerEvent(TriggerEvent& e)
{
  int chip_id = e.getChipId();
  
  for(auto it = mHitQueue[chip_id].begin(); it != mHitQueue[chip_id].end(); it++) {
    // All the hits are ordered in time in the hit queue.
    // If this hit is not active, it could be that:
    // 1) We haven't reached the newer hits which would be active for this event yet
    // 2) We have gone through the hits that are active for this event, and have now
    //    reached hits that are "too new" (event queue size is larger than 0 then)
    if(it->isActive(e.getEventStartTime(), e.getEventEndTime())) {
      e.addHit(*it);
    } else if (e.getEventSize() > 0) {
      // Case 2. There won't be any more hits now, so we can break.
      /// @todo Is this check worth it performance wise, or is it better to just iterate
      ///       through the whole list?
      break;
    }
  }
}


///@brief SystemC controled method, should be sensitive to the positive edge
///       of the clock. Responsible for
///       1) Creating new physics events (hits)
///       2) Deleting old inactive hits
void EventGenerator::physicsEventProcess(void)
{
  int64_t t_delta_cycles;

  // Run until the simulation stops
  while(1) {
    // Generate next physics event. This event will be t_delta_cycles in the future.
    t_delta_cycles = generateNextPhysicsEvent();

    // Indicate the event with a 1 clock cycle pulse on this signal
    s_physics_event_out.write(1);
    wait(1);
    s_physics_event_out.write(0);

    if(t_delta_cycles > 1) {
      // Wait for t_delta_cycles number of clock cycles, which is when the next physics event
      // will actually happen, taking into account the clock cycle we already waited.
      // Note: this type of wait() call only works with CTHREAD (normally one would specify time unit).
      wait(t_delta_cycles-1);
    }

    ///@todo Maybe do this only on strobe falling edge? Saves some CPU cycles that way?
    removeInactiveHits();
  }
}


///@brief SystemC controlled method. It should be sensitive to the strobe signal,
///       (both rising and falling edge) and is responsible for creating the
///       triggerEvent objects after a STROBE pulse.
void EventGenerator::triggerEventProcess(void)
{
  int64_t time_now = sc_time_stamp().value();
  bool triggers_filtered = false;

  #ifdef DEBUG_OUTPUT
  print_function_timestamp();
  #endif
      
  // Falling edge - active low strobe
  if(s_strobe_in.read() == false) {
    // Save the current simulation time when the strobe was asserted.
    // We will create the triggerEvent object when it is deasserted, and need to
    // remember the start time.
    mNextTriggerEventStartTimeNs = time_now;

    // Make sure this process doesn't trigger the first time on the wrong strobe edge..
    mStrobeActive = true;
  } else if(mStrobeActive) { // Rising edge.
    mStrobeActive = false;

    for(int chip_id = 0; chip_id < mNumChips; chip_id++) {
      TriggerEvent* next_trigger_event = generateNextTriggerEvent(mNextTriggerEventStartTimeNs,
                                                                  time_now,
                                                                  chip_id);
      
      mEventQueue[chip_id].push(next_trigger_event);

      // Post an event notification that a new trigger event/frame is ready
      E_trigger_event_available->notify(SC_ZERO_TIME);

      // Write number of pixel hits in event to CSV file
      if(mCreateCSVFile) {
        // Write time column and filtered column only one time
        if(chip_id == 0) {
          mTriggerEventsCSVFile << time_now << ";";
          mTriggerEventsCSVFile << (next_trigger_event->getEventFilteredFlag() ? "true" : "false");
        }
        
        mTriggerEventsCSVFile << ";" << next_trigger_event->getEventSize();

        if(chip_id == mNumChips-1)
          mTriggerEventsCSVFile << std::endl;
      }

      // Doesn't matter if this is set every time during the loop, all trigger events
      // with the same ID will have the trigger filtered flag set to the same value
      triggers_filtered = next_trigger_event->getEventFilteredFlag();

      #ifdef DEBUG_OUTPUT
      std::cout << "\tTrigger event queue size: " << mEventQueue.size() << std::endl;
      #endif
    }
    
    // Don't update last event time if the trigger event(s) were filtered out
    if(triggers_filtered == false) {
      mLastTriggerEventStartTimeNs = mNextTriggerEventStartTimeNs;
      mLastTriggerEventEndTimeNs = time_now;
    }
    mTriggerEventIdCount++;    
    mNextTriggerEventChipId = 0;

    #ifdef DEBUG_OUTPUT
    std::cout << "\tTrigger start time: " << mLastTriggerEventStartTimeNs << " ns. " << std::endl;
    std::cout << "\tEnd time: " << mLastTriggerEventEndTimeNs << " ns." << std::endl;
    #endif
  }
}

