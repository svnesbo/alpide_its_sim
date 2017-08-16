/**
 * @file   EventGenerator.cpp
 * @author Simon Voigt Nesbo
 * @date   December 22, 2016
 * @details A simple event generator for Alpide SystemC simulation model.
 */

#include "EventGenerator.hpp"
#include "Alpide/alpide_constants.hpp"
#include <boost/current_function.hpp>
#include <boost/random/random_device.hpp>
#include <stdexcept>
#include <cmath>
#include <map>
#include <QDir>


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
///@brief Constructor for EventGenerator
///@param[in] name SystemC module name
///@param[in] settings QSettings object with simulation settings.
///@param[in] output_path Directory path to store simulation output data in
EventGenerator::EventGenerator(sc_core::sc_module_name name,
                               const QSettings* settings,
                               std::string output_path)
  : sc_core::sc_module(name)
{
  mOutputPath = output_path;
  mRandomHitGeneration = settings->value("event/random_hit_generation").toBool();
  mBunchCrossingRateNs = settings->value("event/bunch_crossing_rate_ns").toInt();
  mAverageEventRateNs = settings->value("event/average_event_rate_ns").toInt();
  mRandomSeed = settings->value("simulation/random_seed").toInt();
  mCreateCSVFile = settings->value("data_output/write_event_csv").toBool();
  mPixelDeadTime = settings->value("alpide/pixel_shaping_dead_time_ns").toInt();
  mPixelActiveTime = settings->value("alpide/pixel_shaping_active_time_ns").toInt();
  mNumChips = settings->value("simulation/n_chips").toInt();

  mContinuousMode = settings->value("simulation/continuous_mode").toBool();

  if(mRandomHitGeneration) {
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
      std::string dist_file = multipl_dist_file.toStdString();
      readDiscreteDistributionFile(dist_file.c_str(), mult_dist);

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
  } else {
    QDir monte_carlo_event_dir("config/monte_carlo_events/PbPb/");
    QStringList name_filters;

    name_filters << "*.xml";

    QStringList MC_xml_files = monte_carlo_event_dir.entryList(name_filters);

    mMonteCarloEvents.readEventXML("config/monte_carlo_events/PbPb/", MC_xml_files);

    // Discrete and gaussion hit distributions are not used in this case
    mRandHitMultiplicityDiscrete = nullptr;
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

    std::string event_frames_csv_filename = mOutputPath + std::string("/event_frames_data.csv");
    mEventFramesCSVFile.open(event_frames_csv_filename);
    mEventFramesCSVFile << "time";
    mEventFramesCSVFile << ";filtered";
    for(int i = 0; i < mNumChips; i++)
      mEventFramesCSVFile << ";chip_" << i << "_pixel_hits";
    mEventFramesCSVFile << std::endl;
  }


  //////////////////////////////////////////////////////////////////////////////
  // SystemC declarations / connections / etc.
  //////////////////////////////////////////////////////////////////////////////
  SC_METHOD(physicsEventMethod);
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
  if(mEventFramesCSVFile.is_open())
    mEventFramesCSVFile.close();
}


///@brief Get a reference to the next event (if there is one). Note: this function
///       will keep returning the same event until it has been removed by removeOldestEvent().
///@return Const reference to std::vector<Hit> that contains the hits in the latest event.
const std::vector<ITS::ITSPixelHit>& EventGenerator::getLatestPhysicsEvent(void) const
{
  return mHitVector;
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
///@param[in] filename Relative or absolute path and filename to open
///@param[out] dist_vector Reference to vector to store the distribution in
///@throw runtime_error If the file can not be opened
///@throw domain_error If a negative x-value (hits) or y-value (probability) is encountered in the file
void EventGenerator::readDiscreteDistributionFile(const char* filename, std::vector<double> &dist_vector) const
{
  std::ifstream in_file(filename);

  if(in_file.is_open() == false) {
    std::cerr << "Error opening discrete distribution file: " << filename << std::endl;
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
///@param[in,out] dist_vector Distribution to scale. The original distribution in
///               this vector will be overwritten and replaced with the new, scaled, distribution.
///@param[in] new_mean_value The desired mean value of the new distribution.
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
uint64_t EventGenerator::generateNextPhysicsEvent(uint64_t time_now)
{
  int64_t t_delta, t_delta_cycles;
  int n_hits = 0;

  // Initialize array to 0. http://stackoverflow.com/a/2204380/6444574
  int *chip_trace_hit_counts = new int[mNumChips]();
  int *chip_pixel_hit_counts = new int[mNumChips]();

  mLastPhysicsEventTimeNs = time_now;
  mPhysicsEventCount++;

  mHitVector.clear();

  if(mRandomHitGeneration == true) {
    // Generate a random number of hits for this event
    n_hits = getRandomMultiplicity();


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


      ITS::detectorPosition pos = {0, 0, 0, 0};

      ///@todo Create hits for all chips properly here!!
      mHitVector.emplace_back(pos, rand_x1, rand_y1, mLastPhysicsEventTimeNs,
                              mPixelDeadTime, mPixelActiveTime);
      mHitVector.emplace_back(pos, rand_x1, rand_y2, mLastPhysicsEventTimeNs,
                              mPixelDeadTime, mPixelActiveTime);
      mHitVector.emplace_back(pos, rand_x2, rand_y1, mLastPhysicsEventTimeNs,
                              mPixelDeadTime, mPixelActiveTime);
      mHitVector.emplace_back(pos, rand_x2, rand_y2, mLastPhysicsEventTimeNs,
                              mPixelDeadTime, mPixelActiveTime);
    }
  } else { // No random hits, use MC generated events
    const EventDigits* digits = mMonteCarloEvents.getNextEvent();

    if(digits == nullptr)
      throw std::runtime_error("EventDigits::getNextEvent() returned no new Monte Carlo event.");

    n_hits += digits->size();

    auto digit_it = digits->getDigitsIterator();
    auto digit_end_it = digits->getDigitsEndIterator();

    while(digit_it != digit_end_it) {
      const int &chip_id = digit_it->first;
      const PixelData &pix = digit_it->second;

      if(chip_id == 5) {
        // mHitVector[chip_id].emplace_back(pix.getCol(), pix.getRow(),
        //                                 mLastPhysicsEventTimeNs,
        //                                 mPixelDeadTime,
        //                                 mPixelActiveTime);

        ITS::detectorPosition pos = {0, 0, 0, 0};

        mHitVector.emplace_back(pos, pix.getCol(), pix.getRow(),
                                mLastPhysicsEventTimeNs,
                                mPixelDeadTime,
                                mPixelActiveTime);

        // Update statistics. We only get pixel digits from MC events,
        // no traces, so only this counter is used.
        chip_pixel_hit_counts[chip_id]++;
      }
      digit_it++;
    }
  }


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
    std::cout << "@ " << time_now << " ns: ";
    std::cout << "\tPhysics event number: " << mPhysicsEventCount;
    std::cout << "\tt_delta: " << t_delta;
    std::cout << "\tt_delta_cycles: " << t_delta_cycles;
    std::cout << "\tmLastPhysicsEventTimeNs: " << mLastPhysicsEventTimeNs << std::endl;
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

  return t_delta;
}


///@brief SystemC controlled method
///       1) Creating new physics events (hits)
///       2) Deleting old inactive hits
void EventGenerator::physicsEventMethod(void)
{
  uint64_t time_now = sc_time_stamp().value();
  uint64_t t_delta = generateNextPhysicsEvent(time_now);
  E_physics_event.notify();
  next_trigger(t_delta, SC_NS);
}
