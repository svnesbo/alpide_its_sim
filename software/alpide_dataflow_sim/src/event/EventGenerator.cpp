/**
 * @file   EventGenerator.cpp
 * @author Simon Voigt Nesbo
 * @date   December 22, 2016
 * @details A simple event generator for Alpide SystemC simulation model.
 */

#include "EventGenerator.hpp"
#include "../ITS/ITS_config.hpp"
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
  mSingleChipSimulation = settings->value("simulation/single_chip").toBool();
  mNumChips = 0;

  mITSConfig.layer[0].num_staves = settings->value("its/layer0_num_staves").toInt();
  mITSConfig.layer[1].num_staves = settings->value("its/layer1_num_staves").toInt();
  mITSConfig.layer[2].num_staves = settings->value("its/layer2_num_staves").toInt();
  mITSConfig.layer[3].num_staves = settings->value("its/layer3_num_staves").toInt();
  mITSConfig.layer[4].num_staves = settings->value("its/layer4_num_staves").toInt();
  mITSConfig.layer[5].num_staves = settings->value("its/layer5_num_staves").toInt();
  mITSConfig.layer[6].num_staves = settings->value("its/layer6_num_staves").toInt();

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

      mRandHitMultiplicityDiscrete = new discrete_distribution<>(mult_dist.begin(), mult_dist.end());
      std::cout << "Number of bins in distribution before scaling: ";
      std::cout << mult_dist.size() << std::endl;

      double multpl_dist_mean = normalizeDiscreteDistribution(mult_dist);

      if(mSingleChipSimulation) {
        mSingleChipHitDensity = settings->value("event/hit_density_layer0").toDouble();
        mSingleChipDetectorArea = CHIP_WIDTH_CM * CHIP_HEIGHT_CM;
        mSingleChipHitAverage = mSingleChipHitDensity * mSingleChipDetectorArea;
        mSingleChipMultiplicityScaleFactor = mSingleChipHitAverage / multpl_dist_mean;
        mNumChips = 1;

        std::cout << "Chip area [cm^2]: ";
        std::cout << mDetectorArea[0] << std::endl;

        std::cout << "Chip hit density [cm^-1]: ";
        std::cout << mHitDensities[0] << std::endl;

        std::cout << "Chip average number of hits per event: ";
        std::cout << mHitAverage[0] << std::endl;

        std::cout << "Chip multiplicity distr. scaling factor: ";
        std::cout << mMultiplicityScaleFactor[0] << std::endl;
      } else {
        mHitDensities[0] = settings->value("event/hit_density_layer0").toDouble();
        mHitDensities[1] = settings->value("event/hit_density_layer1").toDouble();
        mHitDensities[2] = settings->value("event/hit_density_layer2").toDouble();
        mHitDensities[3] = settings->value("event/hit_density_layer3").toDouble();
        mHitDensities[4] = settings->value("event/hit_density_layer4").toDouble();
        mHitDensities[5] = settings->value("event/hit_density_layer5").toDouble();
        mHitDensities[6] = settings->value("event/hit_density_layer6").toDouble();

        for(unsigned int layer = 0; layer < ITS::N_LAYERS; layer++) {
          mDetectorArea[layer] =
            ITS::STAVES_PER_LAYER[layer] *
            ITS::CHIPS_PER_STAVE_IN_LAYER[layer] *
            CHIP_WIDTH_CM *
            CHIP_HEIGHT_CM;

          mHitAverage[layer] = mHitDensities[layer] * mDetectorArea[layer];
          mMultiplicityScaleFactor[layer] = mHitAverage[layer] / multpl_dist_mean;

          // Number of chips to actually simulate
          mNumChips += mITSConfig.layer[layer].num_staves * ITS::CHIPS_PER_STAVE_IN_LAYER[layer];

          std::cout << "Num chips so far: " << mNumChips << std::endl;

          std::cout << "Layer " << layer << " area [cm^2]: ";
          std::cout << mDetectorArea[layer] << std::endl;

          std::cout << "Layer " << layer << " hit density [cm^-1]: ";
          std::cout << mHitDensities[layer] << std::endl;

          std::cout << "Layer " << layer << " average number of hits per event: ";
          std::cout << mHitAverage[layer] << std::endl;

          std::cout << "Layer " << layer << " multiplicity distr. scaling factor: ";
          std::cout << mMultiplicityScaleFactor[layer] << std::endl;
        }
      }

      // Gaussian distribution is not used in this case
      mRandHitMultiplicityGauss = nullptr;
    }
  } else {
    QString monte_carlo_event_path_str = settings->value("event/monte_carlo_path").toString();
    QDir monte_carlo_event_dir(monte_carlo_event_path_str);
    QStringList name_filters;

    name_filters << "*.xml";

    QStringList MC_xml_files = monte_carlo_event_dir.entryList(name_filters);

    if(MC_xml_files.empty()) {
      std::cout << "Error: No .xml files found in path \"";
      std::cout << monte_carlo_event_path_str.toStdString();
      std::cout << "\", or path does not exist." << std::endl;
      exit(-1);
    }

    mMCPhysicsEvents = new EventXML(mITSConfig);
    mMCPhysicsEvents->readEventXML(monte_carlo_event_path_str, MC_xml_files);

    mNumChips = 1;

    QString qed_noise_input = settings->value("event/qed_noise_input").toString();
    if(qed_noise_input == "true") {
      mQedNoiseGenEnable = true;
      mQedNoiseRate = settings->value("event/qed_noise_rate_ns").toUInt();

      if(mQedNoiseRate == 0) {
        std::cout << "Error: QED/Noise rate has to be larger than zero." << std::endl;
        exit(-1);
      }

      QString qed_noise_event_path_str = settings->value("event/qed_noise_path").toString();
      QDir qed_noise_event_dir(qed_noise_event_path_str);

      name_filters << "*.xml";

      QStringList QED_NOISE_xml_files = qed_noise_event_dir.entryList(name_filters);

      if(QED_NOISE_xml_files.empty()) {
        std::cout << "Error: No .xml files found in path \"";
        std::cout << qed_noise_event_path_str.toStdString();
        std::cout << "\", or path does not exist." << std::endl;
        exit(-1);
      }

      mMCQedNoiseEvents = new EventXML(mITSConfig);
      mMCQedNoiseEvents->readEventXML(qed_noise_event_path_str, QED_NOISE_xml_files);
    }

    // Discrete and gaussion hit distributions are not used in this case
    mRandHitMultiplicityDiscrete = nullptr;
    mRandHitMultiplicityGauss = nullptr;
  }

  mRandHitChipX = new uniform_int_distribution<int>(0, N_PIXEL_COLS-1);
  mRandHitChipY = new uniform_int_distribution<int>(0, N_PIXEL_ROWS-1);

  for(unsigned int layer = 0; layer < ITS::N_LAYERS; layer++) {
    // Modules are not used for IB layers..
    if(layer > 2)
      mRandModule[layer] =
        new uniform_int_distribution<int>(0, ITS::MODULES_PER_STAVE_IN_LAYER[layer]-1);
    else
      mRandModule[layer] == nullptr;

    mRandChipID[layer] =
      new uniform_int_distribution<int>(0, ITS::CHIPS_PER_MODULE_IN_LAYER[layer]-1);

    mRandStave[layer] =
      new uniform_int_distribution<int>(0, ITS::STAVES_PER_LAYER[layer]-1);
  }

  // Multiplied by BC rate so that the distribution is related to the clock cycles
  // Which is fine because physics events will be in sync with 40MHz BC clock, but
  // to get actual simulation time we must multiply the numbers obtained with BC rate.
  double lambda = 1.0/(mAverageEventRateNs/mBunchCrossingRateNs);
  mRandEventTime = new exponential_distribution<double>(lambda);

  initRandomNumGenerator();

  if(mCreateCSVFile) {
    std::string physics_events_csv_filename = mOutputPath + std::string("/physics_events_data.csv");
    mPhysicsEventsCSVFile.open(physics_events_csv_filename);
    mPhysicsEventsCSVFile << "delta_t;event_pixel_hit_multiplicity";

    if(mITSConfig.layer[0].num_staves > 0)
      mPhysicsEventsCSVFile << ";layer_0";
    if(mITSConfig.layer[1].num_staves > 0)
      mPhysicsEventsCSVFile << ";layer_1";
    if(mITSConfig.layer[2].num_staves > 0)
      mPhysicsEventsCSVFile << ";layer_2";
    if(mITSConfig.layer[3].num_staves > 0)
      mPhysicsEventsCSVFile << ";layer_3";
    if(mITSConfig.layer[4].num_staves > 0)
      mPhysicsEventsCSVFile << ";layer_4";
    if(mITSConfig.layer[5].num_staves > 0)
      mPhysicsEventsCSVFile << ";layer_5";
    if(mITSConfig.layer[6].num_staves > 0)
      mPhysicsEventsCSVFile << ";layer_6";

    for(unsigned int layer = 0; layer < ITS::N_LAYERS; layer++) {
      unsigned int chip_id = ITS::CUMULATIVE_CHIP_COUNT_AT_LAYER[layer];
      for(unsigned int stave = 0; stave < mITSConfig.layer[layer].num_staves; stave++) {
        for(unsigned int stave_chip = 0; stave_chip < ITS::CHIPS_PER_STAVE_IN_LAYER[layer]; stave_chip++) {
          mPhysicsEventsCSVFile << ";chip_" << chip_id;
          chip_id++;
        }
      }
    }

    mPhysicsEventsCSVFile << std::endl;
  }


  //////////////////////////////////////////////////////////////////////////////
  // SystemC declarations / connections / etc.
  //////////////////////////////////////////////////////////////////////////////
  SC_METHOD(physicsEventMethod);

  if(mQedNoiseGenEnable)
    SC_METHOD(qedNoiseEventMethod);
}


EventGenerator::~EventGenerator()
{
  for(unsigned int layer = 0; layer < ITS::N_LAYERS; layer++) {
    delete mRandChipID[layer];
    delete mRandStave[layer];
    delete mRandModule[layer];
  }

  delete mRandHitChipX;
  delete mRandHitChipY;
  delete mRandEventTime;

  // Note: safe to delete nullptr in C++
  delete mRandHitMultiplicityGauss;
  delete mRandHitMultiplicityDiscrete;

  if(mPhysicsEventsCSVFile.is_open())
    mPhysicsEventsCSVFile.close();
}


///@brief Get a reference to the next physics event
///@return Const reference to std::vector<Hit> that contains the hits in the latest event.
const std::vector<ITS::ITSPixelHit>& EventGenerator::getLatestPhysicsEvent(void) const
{
  return mEventHitVector;
}


///@brief Get a reference to the next QED/Noise event
///@return Const reference to std::vector<Hit> that contains the hits in the latest event.
const std::vector<ITS::ITSPixelHit>& EventGenerator::getLatestQedNoiseEvent(void) const
{
  return mQedNoiseHitVector;
}


///@brief Sets the bunch crossing rate, and recalculate the average crossing rate.
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


///@brief Scale the probabilities (y-values) of a discrete distribution,
///       to normalize it to 1.0 probability.
///@param[in,out] dist_vector Distribution to normalize. The original distribution in
///               this vector will be overwritten and replaced with the new, normalized, distribution.
///@throw runtime_error If dist_vector is empty, a runtime_error is thrown.
///@return Mean value in normalized distribution
double EventGenerator::normalizeDiscreteDistribution(std::vector<double> &dist_vector)
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
///@throw  runtime_error if the EventGenerator for some reason does not have
///                      a multiplicity distribution initialized.
unsigned int EventGenerator::getRandomMultiplicity(void)
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
uint64_t EventGenerator::generateNextPhysicsEvent(uint64_t time_now)
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

        ITS::detectorPosition pos = {0, 0, 0, 0};


        ///@todo Account for larger/bigger clusters here (when implemented)
        event_pixel_hit_count += 4;

        // std::cout << "@ " << time_now << " ns:";
        // std::cout << "Generated hit cluster around " << rand_x1 << ":" << rand_y1;
        // std::cout << " for " << pos << std::endl;

        ///@todo Create hits for all chips properly here!!
        mEventHitVector.emplace_back(pos, rand_x1, rand_y1, mLastPhysicsEventTimeNs,
                                mPixelDeadTime, mPixelActiveTime);
        mEventHitVector.emplace_back(pos, rand_x1, rand_y2, mLastPhysicsEventTimeNs,
                                mPixelDeadTime, mPixelActiveTime);
        mEventHitVector.emplace_back(pos, rand_x2, rand_y1, mLastPhysicsEventTimeNs,
                                mPixelDeadTime, mPixelActiveTime);
        mEventHitVector.emplace_back(pos, rand_x2, rand_y2, mLastPhysicsEventTimeNs,
                                mPixelDeadTime, mPixelActiveTime);
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


          unsigned int rand_module_id = 0;

          // Modules are not used for IB layers
          if(layer > 2)
            rand_module_id = (*mRandStave[layer])(mRandHitGen);

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
                                       rand_module_id,
                                       rand_chip_id};

          ///@todo Account for larger/bigger clusters here (when implemented)
          event_pixel_hit_count += 4;

          layer_hits[layer]++;
          chip_hits[detector_position_to_chip_id(pos)]++;

          // std::cout << "@ " << time_now << " ns:";
          // std::cout << "Generated hit cluster around " << rand_x1 << ":" << rand_y1;
          // std::cout << " for " << pos << std::endl;

          ///@todo Create hits for all chips properly here!!
          mEventHitVector.emplace_back(pos, rand_x1, rand_y1, mLastPhysicsEventTimeNs,
                                  mPixelDeadTime, mPixelActiveTime);
          mEventHitVector.emplace_back(pos, rand_x1, rand_y2, mLastPhysicsEventTimeNs,
                                  mPixelDeadTime, mPixelActiveTime);
          mEventHitVector.emplace_back(pos, rand_x2, rand_y1, mLastPhysicsEventTimeNs,
                                  mPixelDeadTime, mPixelActiveTime);
          mEventHitVector.emplace_back(pos, rand_x2, rand_y2, mLastPhysicsEventTimeNs,
                                  mPixelDeadTime, mPixelActiveTime);
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
      const int &chip_id = digit_it->first;
      const PixelData &pix = digit_it->second;

      ITS::detectorPosition pos = ITS::chip_id_to_detector_position(chip_id);

      // This takes a digit, which is just a coordinate, and places it in
      // the hit vector among with timing information for the hit
      // (ie. when it is over threshold, time over threshold)
      mEventHitVector.emplace_back(pos, pix.getCol(), pix.getRow(),
                              mLastPhysicsEventTimeNs,
                              mPixelDeadTime,
                              mPixelActiveTime);

      layer_hits[pos.layer_id]++;
      chip_hits[chip_id]++;

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
void EventGenerator::generateNextQedNoiseEvent(void)
{
  mQedNoiseHitVector.clear();

  const EventDigits* digits = mMCQedNoiseEvents->getNextEvent();

  if(digits == nullptr)
    throw std::runtime_error("EventDigits::getNextEvent() returned no new QED/noise MC event.");

  auto digit_it = digits->getDigitsIterator();
  auto digit_end_it = digits->getDigitsEndIterator();

  while(digit_it != digit_end_it) {
    const int &chip_id = digit_it->first;
    const PixelData &pix = digit_it->second;

    ITS::detectorPosition pos = ITS::chip_id_to_detector_position(chip_id);

    // This takes a digit, which is just a coordinate, and places it in
    // the hit vector among with timing information for the hit
    // (ie. when it is over threshold, time over threshold)
    mQedNoiseHitVector.emplace_back(pos, pix.getCol(), pix.getRow(),
                                    mLastPhysicsEventTimeNs,
                                    mPixelDeadTime,
                                    mPixelActiveTime);

    digit_it++;
  }
}


///@brief SystemC controlled method. Creates new physics events (hits)
void EventGenerator::physicsEventMethod(void)
{
  uint64_t time_now = sc_time_stamp().value();
  uint64_t t_delta = generateNextPhysicsEvent(time_now);
  E_physics_event.notify();
  next_trigger(t_delta, SC_NS);
}


///@brief SystemC controlled method. Creates new QED/Noise events (hits)
void EventGenerator::qedNoiseEventMethod(void)
{
  generateNextQedNoiseEvent();
  E_qed_noise_event.notify();
  next_trigger(mQedNoiseRate, SC_NS);
}
