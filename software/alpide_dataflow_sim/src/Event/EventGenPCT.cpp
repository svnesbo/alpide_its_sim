/**
 * @file   EventGenPCT.cpp
 * @author Simon Voigt Nesbo
 * @date   November 14, 2018
 * @brief  A simple event generator for PCT simulation with Alpide SystemC simulation model.
 */

#include "EventGenPCT.hpp"
#include "../utils.hpp"
#include <boost/random/random_device.hpp>
#include <stdexcept>
#include <cmath>
#include <map>
#include <QDir>


SC_HAS_PROCESS(EventGenPCT);
///@brief Constructor for EventGenPCT
///@param[in] name SystemC module name
///@param[in] settings QSettings object with simulation settings.
///@param[in] output_path Directory path to store simulation output data in
EventGenPCT::EventGenPCT(sc_core::sc_module_name name,
                               const QSettings* settings,
                               std::string output_path)
  : EventGenBase(settings, output_path)
{
  mRandomHitGeneration = settings->value("event/random_hit_generation").toBool();
  mCreateCSVFile = settings->value("data_output/write_event_csv").toBool();

  //////////////////////////////////////////////////////////////////////////////
  // SystemC declarations / connections / etc.
  //////////////////////////////////////////////////////////////////////////////
  SC_METHOD(eventMethod);
}


///@brief Destructor for EventGenPCT class
EventGenPCT::~EventGenPCT()
{
  if(mPhysicsEventsCSVFile.is_open())
    mPhysicsEventsCSVFile.close();
}


///@brief Write simulation stats/data to file
///@param[in] output_path Path to simulation output directory
void EventGenPCT::writeSimulationStats(const std::string output_path) const
{
  mPhysicsReadoutStats->writeToFile(output_path + std::string("/physics_readout_stats.csv"));
  mQedReadoutStats->writeToFile(output_path + std::string("/qed_readout_stats.csv"));
}


///@brief Get a reference to the next physics event
///@return Const reference to std::vector<Hit> that contains the hits in the latest event.
const std::vector<std::shared_ptr<PixelHit>>& EventGenPCT::getLatestPhysicsEvent(void) const
{
  return mEventHitVector;
}


///@brief Get a reference to the next QED/Noise event
///@return Const reference to std::vector<Hit> that contains the hits in the latest event.
const std::vector<std::shared_ptr<PixelHit>>& EventGenPCT::getLatestQedNoiseEvent(void) const
{
  return mQedNoiseHitVector;
}


///@brief Initialize random number generators
void EventGenPCT::initRandomNumGenerators(void)
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
void EventGenPCT::readDiscreteDistributionFile(const char* filename, std::vector<double> &dist_vector) const
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


///@brief Scale the probabilities (y-values) of a discrete distribution,
///       to normalize it to 1.0 probability.
///@param[in,out] dist_vector Distribution to normalize. The original distribution in
///               this vector will be overwritten and replaced with the new, normalized, distribution.
///@throw runtime_error If dist_vector is empty, a runtime_error is thrown.
///@return Mean value in normalized distribution
double EventGenPCT::normalizeDiscreteDistribution(std::vector<double> &dist_vector)
{
  double probability_sum = 0.0;
  double probability_sum_normalized = 0.0;
  double mean_value = 0.0;
  double mean_value_normalized = 0.0;
  double resulting_mean_value = 0.0;
  std::vector<double> new_dist_vector;
  std::map<double, double> scaled_dist_map;

  if(dist_vector.empty())
    throw std::runtime_error("Discrete distribution to scale is empty.");

  // First calculate mean value in current distribution
  for(unsigned int i = 0; i < dist_vector.size(); i++) {
    mean_value += i*dist_vector[i];
    probability_sum += dist_vector[i];
  }
  std::cout << "Mean value in original distribution: " << mean_value << std::endl << std::endl;
  std::cout << "Hit density in original distribution: " << mean_value/4.5 << std::endl << std::endl;
  std::cout << "Probability sum/integral in original distribution: " << probability_sum << std::endl;

  // Normalize the area of the probability curve to 1.0
  for(unsigned int i = 0; i < dist_vector.size(); i++) {
    dist_vector[i] /= probability_sum;
    mean_value_normalized += i*dist_vector[i];
    probability_sum_normalized += dist_vector[i];
  }

  std::cout << "Mean value in original distribution (normalized): " << mean_value_normalized << std::endl << std::endl;
  std::cout << "Hit density in original distribution (normalized): " << mean_value_normalized/4.5 << std::endl << std::endl;
  std::cout << "Probability sum/integral in original distribution (normalized): " << probability_sum_normalized << std::endl;

  return mean_value_normalized;
}


///@brief Return a random number of hits (multiplicity) based on the chosen
///       distribution for multiplicity.
///@return Number of hits
///@throw  runtime_error if the EventGenPCT for some reason does not have
///                      a multiplicity distribution initialized.
unsigned int EventGenPCT::getRandomMultiplicity(void)
{
  if(mRandHitMultiplicityDiscrete != nullptr) {
    unsigned int n_hits = (*mRandHitMultiplicityDiscrete)(mRandHitMultiplicityGen);
    return n_hits;
  }
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
uint64_t EventGenPCT::generateNextPhysicsEvent(uint64_t time_now)
{
  unsigned int event_pixel_hit_count = 0;
  int64_t t_delta, t_delta_cycles;
  unsigned int n_hits = 0;
  unsigned int n_hits_raw = 0;

  std::map<unsigned int, unsigned int> layer_hits;
  std::map<unsigned int, unsigned int> chip_hits;

  mLastPhysicsEventTimeNs = time_now;
  mPhysicsEventCount++;

  mEventHitVector.clear();

  // std::cout << std::endl;
  // std::cout << "@ " << time_now << " ns:";
  // std::cout << "Generating new physics event." << std::endl;
  // std::cout << "--------------------------------------------------";
  // std::cout << std::endl;

  if(mRandomHitGeneration == true) {
    // Generate an uncorrected random number of hits for this event
    n_hits_raw = getRandomMultiplicity();

    // Skip empty events??
    if(mSingleChipSimulation && n_hits_raw > 0) {
      n_hits = n_hits_raw * mSingleChipMultiplicityScaleFactor;

      for(unsigned int i = 0; i < n_hits; i++) {
        unsigned int rand_x1 = (*mRandHitChipX)(mRandHitGen);
        unsigned int rand_y1 = (*mRandHitChipY)(mRandHitGen);
        unsigned int rand_x2, rand_y2;

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

        ITS::detectorPosition pos = {0, 0, 0, 0, 0};


        ///@todo Account for larger/bigger clusters here (when implemented)
        event_pixel_hit_count += 4;

        // Create hit with timing information and pointer to readout stats object
        std::shared_ptr<PixelHit> pix1_shared = std::make_shared<PixelHit>(rand_x1, rand_y1, 0);
        std::shared_ptr<PixelHit> pix2_shared = std::make_shared<PixelHit>(rand_x1, rand_y2, 0);
        std::shared_ptr<PixelHit> pix3_shared = std::make_shared<PixelHit>(rand_x2, rand_y1, 0);
        std::shared_ptr<PixelHit> pix4_shared = std::make_shared<PixelHit>(rand_x2, rand_y2, 0);

        pix1_shared->setActiveTimeStart(mLastPhysicsEventTimeNs+mPixelDeadTime);
        pix2_shared->setActiveTimeStart(mLastPhysicsEventTimeNs+mPixelDeadTime);
        pix3_shared->setActiveTimeStart(mLastPhysicsEventTimeNs+mPixelDeadTime);
        pix4_shared->setActiveTimeStart(mLastPhysicsEventTimeNs+mPixelDeadTime);

        pix1_shared->setActiveTimeEnd(mLastPhysicsEventTimeNs+mPixelDeadTime+mPixelActiveTime);
        pix2_shared->setActiveTimeEnd(mLastPhysicsEventTimeNs+mPixelDeadTime+mPixelActiveTime);
        pix3_shared->setActiveTimeEnd(mLastPhysicsEventTimeNs+mPixelDeadTime+mPixelActiveTime);
        pix4_shared->setActiveTimeEnd(mLastPhysicsEventTimeNs+mPixelDeadTime+mPixelActiveTime);

        pix1_shared->setPixelReadoutStatsObj(mPhysicsReadoutStats);
        pix2_shared->setPixelReadoutStatsObj(mPhysicsReadoutStats);
        pix3_shared->setPixelReadoutStatsObj(mPhysicsReadoutStats);
        pix4_shared->setPixelReadoutStatsObj(mPhysicsReadoutStats);

        mEventHitVector.push_back(pix1_shared);
        mEventHitVector.push_back(pix2_shared);
        mEventHitVector.push_back(pix3_shared);
        mEventHitVector.push_back(pix4_shared);
      }
    } else if(n_hits_raw > 0) { // Generate hits for each layer in ITS detector simulation
      for(unsigned int layer = 0; layer < ITS::N_LAYERS; layer++) {
        //for(unsigned int layer = 0; layer < num_layers; layer++) {

        // Skip this layer if no staves are configured for this
        // layer in simulation settings file
        if(mITSConfig.layer[layer].num_staves == 0)
          continue;

        n_hits = n_hits_raw * mMultiplicityScaleFactor[layer];

        std::cout << "@ " << time_now << " ns: ";
        std::cout << "Generating " << n_hits;
        std::cout << " track hits for layer " << layer << "." << std::endl;

        // Generate hits here
        for(unsigned int i = 0; i < n_hits; i++) {
          unsigned int rand_stave_id = (*mRandStave[layer])(mRandHitGen);

          // Skip hits for staves in this layer other than the first
          // N staves that were defined in the simulation settings file
          if(rand_stave_id >= mITSConfig.layer[layer].num_staves)
            continue;

          unsigned int rand_sub_stave_id = 0;
          if(layer > 2) // Sub staves are not used for IB layers
            rand_sub_stave_id = (*mRandSubStave[layer])(mRandHitGen);

          unsigned int rand_module_id = 0;
          if(layer > 2) // Modules are not used for IB layers
            rand_module_id = (*mRandModule[layer])(mRandHitGen);

          unsigned int rand_chip_id = (*mRandChipID[layer])(mRandHitGen);


          unsigned int rand_x1 = (*mRandHitChipX)(mRandHitGen);
          unsigned int rand_y1 = (*mRandHitChipY)(mRandHitGen);
          unsigned int rand_x2, rand_y2;

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

          ITS::detectorPosition pos = {layer,
                                       rand_stave_id,
                                       rand_sub_stave_id,
                                       rand_module_id,
                                       rand_chip_id};

          unsigned int global_chip_id = ITS::detector_position_to_chip_id(pos);

#ifdef PIXEL_DEBUG
          std::cerr << "Created hit for: chip_id: " << global_chip_id;
          std::cerr << ", layer: " << layer;
          std::cerr << ", stave: " << rand_stave_id;
          std::cerr << ", sub stave: " << rand_sub_stave_id;
          std::cerr << ", module: " << rand_module_id;
          std::cerr << ", local chip id: " << rand_chip_id << std::endl;
#endif

          ///@todo Account for larger/bigger clusters here (when implemented)
          event_pixel_hit_count += 4;

          layer_hits[layer]++;
          chip_hits[global_chip_id]++;

          // Create hit with timing information and pointer to readout stats object
          std::shared_ptr<PixelHit> pix1_shared = std::make_shared<PixelHit>(rand_x1, rand_y1, global_chip_id);
          std::shared_ptr<PixelHit> pix2_shared = std::make_shared<PixelHit>(rand_x1, rand_y2, global_chip_id);
          std::shared_ptr<PixelHit> pix3_shared = std::make_shared<PixelHit>(rand_x2, rand_y1, global_chip_id);
          std::shared_ptr<PixelHit> pix4_shared = std::make_shared<PixelHit>(rand_x2, rand_y2, global_chip_id);

          pix1_shared->setActiveTimeStart(mLastPhysicsEventTimeNs+mPixelDeadTime);
          pix2_shared->setActiveTimeStart(mLastPhysicsEventTimeNs+mPixelDeadTime);
          pix3_shared->setActiveTimeStart(mLastPhysicsEventTimeNs+mPixelDeadTime);
          pix4_shared->setActiveTimeStart(mLastPhysicsEventTimeNs+mPixelDeadTime);

          pix1_shared->setActiveTimeEnd(mLastPhysicsEventTimeNs+mPixelDeadTime+mPixelActiveTime);
          pix2_shared->setActiveTimeEnd(mLastPhysicsEventTimeNs+mPixelDeadTime+mPixelActiveTime);
          pix3_shared->setActiveTimeEnd(mLastPhysicsEventTimeNs+mPixelDeadTime+mPixelActiveTime);
          pix4_shared->setActiveTimeEnd(mLastPhysicsEventTimeNs+mPixelDeadTime+mPixelActiveTime);

          pix1_shared->setPixelReadoutStatsObj(mPhysicsReadoutStats);
          pix2_shared->setPixelReadoutStatsObj(mPhysicsReadoutStats);
          pix3_shared->setPixelReadoutStatsObj(mPhysicsReadoutStats);
          pix4_shared->setPixelReadoutStatsObj(mPhysicsReadoutStats);

          mEventHitVector.push_back(pix1_shared);
          mEventHitVector.push_back(pix2_shared);
          mEventHitVector.push_back(pix3_shared);
          mEventHitVector.push_back(pix4_shared);
        } // Hit generation loop
      } // Layer loop
    } // Detector simulation

  } else { // No random hits, use MC generated events
    const EventDigits* digits = mMCPhysicsEvents->getNextEvent();

    if(digits == nullptr)
      throw std::runtime_error("EventDigits::getNextEvent() returned no new Monte Carlo event.");

    auto digit_it = digits->getDigitsIterator();
    auto digit_end_it = digits->getDigitsEndIterator();

    event_pixel_hit_count = digits->size();

    while(digit_it != digit_end_it) {
      const PixelHit &pix = *digit_it;

      // Recreate hit with timing information and pointer to readout stats object
      std::shared_ptr<PixelHit> pix_shared = std::make_shared<PixelHit>(pix);
      pix_shared->setActiveTimeStart(mLastPhysicsEventTimeNs+mPixelDeadTime);
      pix_shared->setActiveTimeEnd(mLastPhysicsEventTimeNs+mPixelDeadTime+mPixelActiveTime);
      pix_shared->setPixelReadoutStatsObj(mPhysicsReadoutStats);

      mQedNoiseHitVector.push_back(pix_shared);

      ITS::detectorPosition pos = ITS::chip_id_to_detector_position(pix.getChipId());

      layer_hits[pos.layer_id]++;
      chip_hits[pix.getChipId()]++;

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
    // Write time to next event, and multiplicity for the whole event
    mPhysicsEventsCSVFile << t_delta << ";" << event_pixel_hit_count;

    // Write multiplicity for whole layers of detectors (of included layers)
    for(unsigned int layer = 0; layer < ITS::N_LAYERS; layer++) {
      if(mITSConfig.layer[layer].num_staves > 0)
        mPhysicsEventsCSVFile << ";" << layer_hits[layer];
    }

    // Write multiplicity for the chips that were included in the simulation
    for(unsigned int layer = 0; layer < ITS::N_LAYERS; layer++) {
      unsigned int chip_id = ITS::CUMULATIVE_CHIP_COUNT_AT_LAYER[layer];
      for(unsigned int stave = 0; stave < mITSConfig.layer[layer].num_staves; stave++) {
        for(unsigned int stave_chip = 0; stave_chip < ITS::CHIPS_PER_STAVE_IN_LAYER[layer]; stave_chip++) {
          mPhysicsEventsCSVFile << ";" << chip_hits[chip_id];
          chip_id++;
        }
      }
    }

    mPhysicsEventsCSVFile << std::endl;
  }

  return t_delta;
}


///@brief Generate a QED/Noise event
void EventGenPCT::generateNextQedNoiseEvent(void)
{
  mQedNoiseHitVector.clear();

  const EventDigits* digits = mMCQedNoiseEvents->getNextEvent();

  if(digits == nullptr)
    throw std::runtime_error("EventDigits::getNextEvent() returned no new QED/noise MC event.");

  auto digit_it = digits->getDigitsIterator();
  auto digit_end_it = digits->getDigitsEndIterator();

  while(digit_it != digit_end_it) {
    const PixelHit &pix = *digit_it;

    // Recreate hit with timing information and pointer to readout stats object
    std::shared_ptr<PixelHit> pix_shared = std::make_shared<PixelHit>(pix);
    pix_shared->setActiveTimeStart(mLastPhysicsEventTimeNs+mPixelDeadTime);
    pix_shared->setActiveTimeEnd(mLastPhysicsEventTimeNs+mPixelDeadTime+mPixelActiveTime);
    pix_shared->setPixelReadoutStatsObj(mQedReadoutStats);

    mQedNoiseHitVector.push_back(pix_shared);

    digit_it++;
  }
}


///@brief SystemC controlled method. Creates new physics events (hits)
void EventGenPCT::physicsEventMethod(void)
{
  if(mStopEventGeneration == false) {
    uint64_t time_now = sc_time_stamp().value();
    uint64_t t_delta = generateNextPhysicsEvent(time_now);
    E_physics_event.notify();
    next_trigger(t_delta, SC_NS);
  }
}


///@brief SystemC controlled method. Creates new QED/Noise events (hits)
void EventGenPCT::qedNoiseEventMethod(void)
{
  if(mStopEventGeneration == false) {
    generateNextQedNoiseEvent();
    E_qed_noise_event.notify();
    next_trigger(mQedNoiseFeedRateNs, SC_NS);
  }
}


void EventGenPCT::stopEventGeneration(void)
{
  mStopEventGeneration = true;
  mEventHitVector.clear();
  mQedNoiseHitVector.clear();
}
