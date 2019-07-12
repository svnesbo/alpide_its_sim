/**
 * @file   EventGenITS.cpp
 * @author Simon Voigt Nesbo
 * @date   November 18, 2018
 * @details A simple event generator for ITS simulation with Alpide SystemC simulation model.
 */



#include <stdexcept>
#include <cmath>
#include <map>
#include <boost/random/random_device.hpp>
#include <QDir>
#include "Alpide/alpide_constants.hpp"
#include "../utils.hpp"
#include "EventGenITS.hpp"
#include "EventXMLITS.hpp"
#include "EventBinaryITS.hpp"

#ifdef ROOT_ENABLED
#include "EventRootFocal.hpp"
#endif


using boost::random::uniform_int_distribution;
using boost::random::normal_distribution;
using boost::random::exponential_distribution;
using boost::random::discrete_distribution;


SC_HAS_PROCESS(EventGenITS);
///@brief Constructor for EventGenITS
///@param[in] name SystemC module name
///@param[in] config Detector configuration (number of layers/staves etc.)
///@param[in] settings QSettings object with simulation settings.
///@param[in] output_path Directory path to store simulation output data in
EventGenITS::EventGenITS(sc_core::sc_module_name name,
                         Detector::DetectorConfigBase config,
                         const QSettings* settings,
                         std::string output_path)
  : EventGenBase(name, settings, output_path)
  , mDetectorConfig(config)
{
  mBunchCrossingRate_ns = settings->value("its/bunch_crossing_rate_ns").toInt();
  mAverageEventRate_ns = settings->value("event/average_event_rate_ns").toInt();

  if(mRandomHitGeneration) {
    initRandomHitGen(settings);
  } else {
    initMonteCarloHitGen(settings);
  }

  // Random number is always used for event time (follows exponential distribution)
  initRandomNumGen(settings);

  if(mCreateCSVFile)
    initCsvEventFileHeader(settings);


  //////////////////////////////////////////////////////////////////////////////
  // SystemC declarations / connections / etc.
  //////////////////////////////////////////////////////////////////////////////
  SC_METHOD(physicsEventMethod); // "triggered"

  if(mQedNoiseGenEnable)
    SC_METHOD(qedNoiseEventMethod); // "untriggered"
}


///@brief Destructor for EventGenITS class
EventGenITS::~EventGenITS()
{
  for(unsigned int layer = 0; layer < ITS::N_LAYERS; layer++) {
    delete mRandChipID[layer];
    delete mRandStave[layer];
    delete mRandSubStave[layer];
    delete mRandModule[layer];
  }

  delete mRandHitChipX;
  delete mRandHitChipY;
  delete mRandEventTime;

  delete mRandHitMultiplicity;

  if(mPhysicsEventsCSVFile.is_open())
    mPhysicsEventsCSVFile.close();
}


void EventGenITS::initRandomHitGen(const QSettings* settings)
{
  QString multipl_dist_file = settings->value("its/hit_multiplicity_distribution_file").toString();

  // Read multiplicity distribution from file,
  // and initialize boost::random discrete distribution with data
  std::vector<double> mult_dist;
  std::string dist_file = multipl_dist_file.toStdString();
  readDiscreteDistributionFile(dist_file.c_str(), mult_dist);

  mRandHitMultiplicity = new discrete_distribution<>(mult_dist.begin(), mult_dist.end());
  std::cout << "Number of bins in distribution before scaling: ";
  std::cout << mult_dist.size() << std::endl;

  double multpl_dist_mean = normalizeDiscreteDistribution(mult_dist);

  if(mSingleChipSimulation) {
    mSingleChipHitDensity = settings->value("its/hit_density_layer0").toDouble();
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
    mHitDensities[0] = settings->value("its/hit_density_layer0").toDouble();
    mHitDensities[1] = settings->value("its/hit_density_layer1").toDouble();
    mHitDensities[2] = settings->value("its/hit_density_layer2").toDouble();
    mHitDensities[3] = settings->value("its/hit_density_layer3").toDouble();
    mHitDensities[4] = settings->value("its/hit_density_layer4").toDouble();
    mHitDensities[5] = settings->value("its/hit_density_layer5").toDouble();
    mHitDensities[6] = settings->value("its/hit_density_layer6").toDouble();

    for(unsigned int layer = 0; layer < ITS::N_LAYERS; layer++) {
      mDetectorArea[layer] =
        ITS::STAVES_PER_LAYER[layer] *
        ITS::CHIPS_PER_STAVE_IN_LAYER[layer] *
        CHIP_WIDTH_CM *
        CHIP_HEIGHT_CM;

      mHitAverage[layer] = mHitDensities[layer] * mDetectorArea[layer];
      mMultiplicityScaleFactor[layer] = mHitAverage[layer] / multpl_dist_mean;

      // Number of chips to actually simulate
      mNumChips += mDetectorConfig.layer[layer].num_staves * ITS::CHIPS_PER_STAVE_IN_LAYER[layer];

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

  mRandHitChipX = new uniform_int_distribution<int>(0, N_PIXEL_COLS-1);
  mRandHitChipY = new uniform_int_distribution<int>(0, N_PIXEL_ROWS-1);

  for(unsigned int layer = 0; layer < ITS::N_LAYERS; layer++) {
    // Modules are not used for IB layers..
    if(layer > 2) {
      mRandSubStave[layer] =
        new uniform_int_distribution<int>(0, ITS::SUB_STAVES_PER_STAVE[layer]-1);
      mRandModule[layer] =
        new uniform_int_distribution<int>(0, ITS::MODULES_PER_SUB_STAVE_IN_LAYER[layer]-1);
    } else {
      mRandSubStave[layer] = nullptr;
      mRandModule[layer] = nullptr;
    }

    mRandChipID[layer] =
      new uniform_int_distribution<int>(0, ITS::CHIPS_PER_MODULE_IN_LAYER[layer]-1);

    mRandStave[layer] =
      new uniform_int_distribution<int>(0, ITS::STAVES_PER_LAYER[layer]-1);
  }
}


void EventGenITS::initMonteCarloHitGen(const QSettings* settings)
{
  QString monte_carlo_file_type = settings->value("event/monte_carlo_file_type").toString();
  QString monte_carlo_event_path_str = settings->value("its/monte_carlo_dir_path").toString();
  QString monte_carlo_focal_data_file_str = settings->value("pct/monte_carlo_file_path").toString();
  QDir monte_carlo_event_dir(monte_carlo_event_path_str);
  QStringList name_filters;

  if(monte_carlo_file_type == "xml" && mSimType == "its") {
    name_filters << "*.xml";
    QStringList MC_files = monte_carlo_event_dir.entryList(name_filters);

    if(MC_files.isEmpty()) {
      std::cerr << "Error: No .xml files found in MC event path";
      std::cerr << std::endl;
      exit(-1);
    }

    mMCPhysicsEvents = new EventXMLITS(mDetectorConfig,
                                       &ITS::ITS_global_chip_id_to_position,
                                       &ITS::ITS_position_to_global_chip_id,
                                       monte_carlo_event_path_str,
                                       MC_files,
                                       true,
                                       mRandomSeed);
  }
  else if(monte_carlo_file_type == "binary" && mSimType == "its") {
    name_filters << "*.dat";
    QStringList MC_files = monte_carlo_event_dir.entryList(name_filters);

    if(MC_files.isEmpty()) {
      std::cerr << "Error: No binary .dat files found in MC event path";
      std::cerr << std::endl;
      exit(-1);
    }

    mMCPhysicsEvents = new EventBinaryITS(mDetectorConfig,
                                          &ITS::ITS_global_chip_id_to_position,
                                          &ITS::ITS_position_to_global_chip_id,
                                          monte_carlo_event_path_str,
                                          MC_files,
                                          true,
                                          mRandomSeed);
  }
  else if(monte_carlo_file_type == "root" && mSimType == "focal") {
#ifdef ROOT_ENABLED
    unsigned int random_seed = mRandomSeed;

    if(random_seed == 0) {
      boost::random::random_device r;
      random_seed = r();
    }

    mFocalEvents = new EventRootFocal(mDetectorConfig,
                                      &PCT::PCT_global_chip_id_to_position,
                                      &PCT::PCT_position_to_global_chip_id,
                                      monte_carlo_focal_data_file_str,
                                      random_seed);
#else
    std::cerr << "Error: Simulation must be compiled with ROOT support for Focal simulation." << std::endl;
    exit(-1);
#endif
  }
  else if(mSimType == "focal") {
    std::cerr << "Error: Only monte carlo files in ROOT format supported for Focal simulation." << std::endl;
    exit(-1);
  }
  else if(monte_carlo_file_type == "root" && mSimType == "its") {
    std::cerr << "Error: MC files in ROOT format not supported for ITS simulation" << std::endl;
    exit(-1);
  }
  else {
    std::cerr << "Error: Unknown MC event format \"";
    std::cerr << monte_carlo_file_type.toStdString() << "\"";
    exit(-1);
  }

  mNumChips = 1;

  QString qed_noise_input = settings->value("event/qed_noise_input").toString();
  if(qed_noise_input == "true") {
    mQedNoiseGenEnable = true;
    mQedNoiseFeedRateNs = settings->value("event/qed_noise_feed_rate_ns").toUInt();
    mQedNoiseEventRateNs = settings->value("event/qed_noise_event_rate_ns").toUInt();

    // The QED events are generated by AliRoot with two fixed parameters:
    // - integration time (ie. qed_noise_feed_rate_ns)
    // - event rate (ie. qed_noise_event_rate_ns)
    // To use the QED events at a different event rate (average_event_rate_ns setting)
    // than what they were generated for, the feed rate needs to be scaled to the new
    // event rate.
    double scaling_factor = (double)mQedNoiseEventRateNs / (double)mAverageEventRate_ns;
    mQedNoiseFeedRateNs = mQedNoiseFeedRateNs/scaling_factor;

    if(mQedNoiseFeedRateNs == 0) {
      std::cout << "Error: QED/Noise rate has to be larger than zero." << std::endl;
      exit(-1);
    }

    QString qed_noise_event_path_str = settings->value("event/qed_noise_path").toString();
    QDir qed_noise_event_dir(qed_noise_event_path_str);

    if(mSimType == "focal") {
      std::cerr << "QED/noise not supported for Focal.";
      std::cerr << std::endl;
      exit(-1);
    }
    else if(monte_carlo_file_type == "xml") {
      name_filters << "*.xml";
      QStringList QED_noise_event_files = qed_noise_event_dir.entryList(name_filters);

      if(QED_noise_event_files.isEmpty()) {
        std::cerr << "Error: No .xml files found in QED/noise event path";
        std::cerr << std::endl;
        exit(-1);
      }

      mMCQedNoiseEvents = new EventXMLITS(mDetectorConfig,
                                          &ITS::ITS_global_chip_id_to_position,
                                          &ITS::ITS_position_to_global_chip_id,
                                          qed_noise_event_path_str,
                                          QED_noise_event_files,
                                          true,
                                          mRandomSeed);
    }
    else if(monte_carlo_file_type == "binary") {
      name_filters << "*.dat";
      QStringList QED_noise_event_files = qed_noise_event_dir.entryList(name_filters);

      if(QED_noise_event_files.isEmpty()) {
        std::cerr << "Error: No binary .dat files found in QED/noise event path";
        std::cerr << std::endl;
        exit(-1);
      }

      mMCQedNoiseEvents = new EventBinaryITS(mDetectorConfig,
                                             &ITS::ITS_global_chip_id_to_position,
                                             &ITS::ITS_position_to_global_chip_id,
                                             qed_noise_event_path_str,
                                             QED_noise_event_files,
                                             true,
                                             mRandomSeed);
    }
    else {
      std::cerr << "Error: Unknown MC event format \"";
      std::cerr << monte_carlo_file_type.toStdString() << "\"";
      exit(-1);
    }
  }
}


void EventGenITS::initCsvEventFileHeader(const QSettings* settings)
{
  std::string physics_events_csv_filename = mOutputPath + std::string("/physics_events_data.csv");
  mPhysicsEventsCSVFile.open(physics_events_csv_filename);
  mPhysicsEventsCSVFile << "delta_t;event_pixel_hit_multiplicity";

  if(mDetectorConfig.layer[0].num_staves > 0)
    mPhysicsEventsCSVFile << ";layer_0";
  if(mDetectorConfig.layer[1].num_staves > 0)
    mPhysicsEventsCSVFile << ";layer_1";
  if(mDetectorConfig.layer[2].num_staves > 0)
    mPhysicsEventsCSVFile << ";layer_2";
  if(mDetectorConfig.layer[3].num_staves > 0)
    mPhysicsEventsCSVFile << ";layer_3";
  if(mDetectorConfig.layer[4].num_staves > 0)
    mPhysicsEventsCSVFile << ";layer_4";
  if(mDetectorConfig.layer[5].num_staves > 0)
    mPhysicsEventsCSVFile << ";layer_5";
  if(mDetectorConfig.layer[6].num_staves > 0)
    mPhysicsEventsCSVFile << ";layer_6";

  for(unsigned int layer = 0; layer < ITS::N_LAYERS; layer++) {
    unsigned int chip_id = ITS::CUMULATIVE_CHIP_COUNT_AT_LAYER[layer];
    for(unsigned int stave = 0; stave < mDetectorConfig.layer[layer].num_staves; stave++) {
      // Safe to ignore OB sub staves here, since we only include full staves in simulation,
      // and this will include all chips from a full stave
      for(unsigned int stave_chip = 0; stave_chip < ITS::CHIPS_PER_STAVE_IN_LAYER[layer]; stave_chip++) {
        mPhysicsEventsCSVFile << ";chip_" << chip_id;
        chip_id++;
      }
    }
  }

  mPhysicsEventsCSVFile << std::endl;
}


void EventGenITS::addCsvEventLine(uint64_t t_delta,
                                  unsigned int event_pixel_hit_count,
                                  std::map<unsigned int, unsigned int> &chip_hits,
                                  std::map<unsigned int, unsigned int> &layer_hits)
{
  // Write time to next event, and multiplicity for the whole event
  mPhysicsEventsCSVFile << t_delta << ";" << event_pixel_hit_count;

  // Write multiplicity for whole layers of detectors (of included layers)
  for(unsigned int layer = 0; layer < ITS::N_LAYERS; layer++) {
    if(mDetectorConfig.layer[layer].num_staves > 0)
      mPhysicsEventsCSVFile << ";" << layer_hits[layer];
  }

  // Write multiplicity for the chips that were included in the simulation
  for(unsigned int layer = 0; layer < ITS::N_LAYERS; layer++) {
    unsigned int chip_id = ITS::CUMULATIVE_CHIP_COUNT_AT_LAYER[layer];
    for(unsigned int stave = 0; stave < mDetectorConfig.layer[layer].num_staves; stave++) {
      for(unsigned int stave_chip = 0; stave_chip < ITS::CHIPS_PER_STAVE_IN_LAYER[layer]; stave_chip++) {
        mPhysicsEventsCSVFile << ";" << chip_hits[chip_id];
        chip_id++;
      }
    }
  }

  mPhysicsEventsCSVFile << std::endl;
}


///@brief Get a reference to the next "triggered" event. In this event generator this is used
///       for collision events, which are discrete events that do not happen continuously,
///       and which are typically triggered on.
///@return Const reference to std::vector<std::shared_ptr<PixelHit>> that
///        contains the hits in the latest event.
const std::vector<std::shared_ptr<PixelHit>>& EventGenITS::getTriggeredEvent(void) const
{
  return mEventHitVector;
}


///@brief Get a reference to the next "untriggered" event. In this event generator this is
///       used for QED and noise events, processes that happens continuously.
///@return Const reference to std::vector<Hit> that contains the hits in the latest event.
const std::vector<std::shared_ptr<PixelHit>>& EventGenITS::getUntriggeredEvent(void) const
{
  return mQedNoiseHitVector;
}


///@brief Initialize random number generators used with random distributions in this class.
///       There is a generator for event time which is always used.
///       And if random event generation is enabled (no monte carlo input), then the generators
///       for random multiplicity and random hit coords are used.
void EventGenITS::initRandomNumGen(const QSettings* settings)
{
  // Multiplied by BC rate so that the distribution is related to the clock cycles
  // Which is fine because physics events will be in sync with 40MHz BC clock, but
  // to get actual simulation time we must multiply the numbers obtained with BC rate.
  double lambda = 1.0/(mAverageEventRate_ns/mBunchCrossingRate_ns);
  mRandEventTime = new exponential_distribution<double>(lambda);

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
void EventGenITS::readDiscreteDistributionFile(const char* filename, std::vector<double> &dist_vector) const
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
double EventGenITS::normalizeDiscreteDistribution(std::vector<double> &dist_vector)
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
///@throw  runtime_error if the EventGenITS for some reason does not have
///                      a multiplicity distribution initialized.
unsigned int EventGenITS::getRandomMultiplicity(void)
{
    unsigned int n_hits = (*mRandHitMultiplicity)(mRandHitMultiplicityGen);
    return n_hits;
}


///@brief Generate a random event, and put it in the hit vector.
///@param[out] event_time_ns Time when event occured
///@param[out] event_pixel_hit_count Total number of pixel hits for this event,
///            for all layers/chips, including chips/layers that are excluded from the simulation
///@param[out] chip_hits Map with number of pixel hits for this event per chip ID
///@param[out] layer_hits Map with number of pixel hits for this event per layer
void EventGenITS::generateRandomEventData(uint64_t event_time_ns,
                                          unsigned int &event_pixel_hit_count,
                                          std::map<unsigned int, unsigned int> &chip_hits,
                                          std::map<unsigned int, unsigned int> &layer_hits)
{
  // Random number of hits directly from discrete multiplicity distribution
  unsigned int n_particle_hits_unscaled = 0;

  // Random number of hits, scaled to hit density for whatever layer
  unsigned int n_particle_hits_scaled = 0;

  // Clear old hit data
  mEventHitVector.clear();

  // Generate an uncorrected random number of particle hits for this event
  n_particle_hits_unscaled = getRandomMultiplicity();

  if(mSingleChipSimulation && n_particle_hits_unscaled > 0) {
    n_particle_hits_scaled = n_particle_hits_unscaled * mSingleChipMultiplicityScaleFactor;

    for(unsigned int i = 0; i < n_particle_hits_scaled; i++) {
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

      Detector::DetectorPosition pos = {0, 0, 0, 0, 0};


      ///@todo USE createCluster method in EventGenBase here....
      event_pixel_hit_count += 4;

      // Create hit with timing information and pointer to readout stats object
      std::shared_ptr<PixelHit> pix1_shared = std::make_shared<PixelHit>(rand_x1, rand_y1, 0);
      std::shared_ptr<PixelHit> pix2_shared = std::make_shared<PixelHit>(rand_x1, rand_y2, 0);
      std::shared_ptr<PixelHit> pix3_shared = std::make_shared<PixelHit>(rand_x2, rand_y1, 0);
      std::shared_ptr<PixelHit> pix4_shared = std::make_shared<PixelHit>(rand_x2, rand_y2, 0);

      pix1_shared->setActiveTimeStart(event_time_ns+mPixelDeadTime);
      pix2_shared->setActiveTimeStart(event_time_ns+mPixelDeadTime);
      pix3_shared->setActiveTimeStart(event_time_ns+mPixelDeadTime);
      pix4_shared->setActiveTimeStart(event_time_ns+mPixelDeadTime);

      pix1_shared->setActiveTimeEnd(event_time_ns+mPixelDeadTime+mPixelActiveTime);
      pix2_shared->setActiveTimeEnd(event_time_ns+mPixelDeadTime+mPixelActiveTime);
      pix3_shared->setActiveTimeEnd(event_time_ns+mPixelDeadTime+mPixelActiveTime);
      pix4_shared->setActiveTimeEnd(event_time_ns+mPixelDeadTime+mPixelActiveTime);

      pix1_shared->setPixelReadoutStatsObj(mTriggeredReadoutStats);
      pix2_shared->setPixelReadoutStatsObj(mTriggeredReadoutStats);
      pix3_shared->setPixelReadoutStatsObj(mTriggeredReadoutStats);
      pix4_shared->setPixelReadoutStatsObj(mTriggeredReadoutStats);

      mEventHitVector.push_back(pix1_shared);
      mEventHitVector.push_back(pix2_shared);
      mEventHitVector.push_back(pix3_shared);
      mEventHitVector.push_back(pix4_shared);
    }
  } else if(n_particle_hits_unscaled > 0) { // Generate hits for each layer in ITS detector simulation
    for(unsigned int layer = 0; layer < ITS::N_LAYERS; layer++) {
      //for(unsigned int layer = 0; layer < num_layers; layer++) {

      // Skip this layer if no staves are configured for this
      // layer in simulation settings file
      if(mDetectorConfig.layer[layer].num_staves == 0)
        continue;

      n_particle_hits_scaled = n_particle_hits_unscaled * mMultiplicityScaleFactor[layer];

#ifdef PIXEL_DEBUG
      std::cout << "@ " << event_time_ns << " ns: ";
      std::cout << "Generating " << n_particle_hits_scaled;
      std::cout << " track hits for layer " << layer << "." << std::endl;
#endif

      // Generate hits here
      for(unsigned int i = 0; i < n_particle_hits_scaled; i++) {
        unsigned int rand_stave_id = (*mRandStave[layer])(mRandHitGen);

        // Skip hits for staves in this layer other than the first
        // N staves that were defined in the simulation settings file
        if(rand_stave_id >= mDetectorConfig.layer[layer].num_staves)
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

        Detector::DetectorPosition pos = {layer,
                                          rand_stave_id,
                                          rand_sub_stave_id,
                                          rand_module_id,
                                          rand_chip_id};

        unsigned int global_chip_id = ITS::ITS_position_to_global_chip_id(pos);

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

        pix1_shared->setActiveTimeStart(event_time_ns+mPixelDeadTime);
        pix2_shared->setActiveTimeStart(event_time_ns+mPixelDeadTime);
        pix3_shared->setActiveTimeStart(event_time_ns+mPixelDeadTime);
        pix4_shared->setActiveTimeStart(event_time_ns+mPixelDeadTime);

        pix1_shared->setActiveTimeEnd(event_time_ns+mPixelDeadTime+mPixelActiveTime);
        pix2_shared->setActiveTimeEnd(event_time_ns+mPixelDeadTime+mPixelActiveTime);
        pix3_shared->setActiveTimeEnd(event_time_ns+mPixelDeadTime+mPixelActiveTime);
        pix4_shared->setActiveTimeEnd(event_time_ns+mPixelDeadTime+mPixelActiveTime);

        pix1_shared->setPixelReadoutStatsObj(mTriggeredReadoutStats);
        pix2_shared->setPixelReadoutStatsObj(mTriggeredReadoutStats);
        pix3_shared->setPixelReadoutStatsObj(mTriggeredReadoutStats);
        pix4_shared->setPixelReadoutStatsObj(mTriggeredReadoutStats);

        mEventHitVector.push_back(pix1_shared);
        mEventHitVector.push_back(pix2_shared);
        mEventHitVector.push_back(pix3_shared);
        mEventHitVector.push_back(pix4_shared);
      } // Hit generation loop
    } // Layer loop
  } // Detector simulation
}


///@brief Generate a monte carlo event (ie. read it from file), and put it in the hit vector.
///@param[out] event_time_ns Time when event occured
///@param[out] event_pixel_hit_count Total number of pixel hits for this event,
///            for all layers/chips, including chips/layers that are excluded from the simulation
///@param[out] chip_hits Map with number of pixel hits for this event per chip ID
///@param[out] layer_hits Map with number of pixel hits for this event per layer
void EventGenITS::generateMonteCarloEventData(uint64_t event_time_ns,
                                              unsigned int &event_pixel_hit_count,
                                              std::map<unsigned int, unsigned int> &chip_hits,
                                              std::map<unsigned int, unsigned int> &layer_hits)
{
  // Clear old hit data
  mEventHitVector.clear();

  const EventDigits* digits;

  if(mSimType == "its")
    digits = mMCPhysicsEvents->getNextEvent();
  else if(mSimType == "focal") {
#ifdef ROOT_ENABLED
    digits = mFocalEvents->getNextEvent();
#else
    throw std::runtime_error("EventGenITS::generateMonteCarloEventData(): Compile with ROOT for focal sim.");
#endif
  } else
    throw std::runtime_error("EventGenITS::generateMonteCarloEventData(): Invalid sim type.");

  if(digits == nullptr)
    throw std::runtime_error("EventDigits::getNextEvent() returned no new Monte Carlo event.");

  auto digit_it = digits->getDigitsIterator();
  auto digit_end_it = digits->getDigitsEndIterator();

  event_pixel_hit_count = digits->size();

  while(digit_it != digit_end_it) {
    const PixelHit &pix = *digit_it;

    // Recreate hit with timing information and pointer to readout stats object
    std::shared_ptr<PixelHit> pix_shared = std::make_shared<PixelHit>(pix);
    pix_shared->setActiveTimeStart(event_time_ns+mPixelDeadTime);
    pix_shared->setActiveTimeEnd(event_time_ns+mPixelDeadTime+mPixelActiveTime);
    pix_shared->setPixelReadoutStatsObj(mTriggeredReadoutStats);

    mEventHitVector.push_back(pix_shared);

    Detector::DetectorPosition pos;

    if(mSimType == "its")
      pos = ITS::ITS_global_chip_id_to_position(pix.getChipId());
    else // Focal
      pos = PCT::PCT_global_chip_id_to_position(pix.getChipId());

    layer_hits[pos.layer_id]++;
    chip_hits[pix.getChipId()]++;

    digit_it++;
  }
}


///@brief Generate the next physics event (in the future).
///       1) Generate time till the next physics event
///       2) Generate hits for the next event, and put them on the hit queue
///       3) Update counters etc.
///@return The number of clock cycles until this event will actually occur
uint64_t EventGenITS::generateNextPhysicsEvent(void)
{
  uint64_t time_now = sc_time_stamp().value();
  unsigned int event_pixel_hit_count = 0;
  uint64_t t_delta, t_delta_cycles;

  std::map<unsigned int, unsigned int> layer_hits;
  std::map<unsigned int, unsigned int> chip_hits;

  mTriggeredEventCount++;

  if(mRandomHitGeneration == true) {
    generateRandomEventData(time_now, event_pixel_hit_count, chip_hits, layer_hits);
  } else {
    generateMonteCarloEventData(time_now, event_pixel_hit_count, chip_hits, layer_hits);
  }

  // Generate random (exponential distributed) interval till next event/interaction
  // The exponential distribution only works with double float, that's why it is rounded
  // to nearest clock cycle. Which is okay, because events in LHC should be synchronous
  // with bunch crossing clock anyway?
  // Add +1 because otherwise we risk getting events with 0 t_delta, which obviously is not
  // physically possible, and also SystemC doesn't allow wait() for 0 clock cycles.
  t_delta_cycles = std::round((*mRandEventTime)(mRandEventTimeGen)) + 1;
  t_delta = t_delta_cycles * mBunchCrossingRate_ns;

  // Write event rate and multiplicity numbers to CSV file
  if(mCreateCSVFile)
    addCsvEventLine(t_delta, event_pixel_hit_count, chip_hits, layer_hits);

  if(mTriggeredEventCount % 100 == 0) {
    std::cout << "@ " << time_now << " ns: ";
    std::cout << "\tPhysics event number: " << mTriggeredEventCount;
    std::cout << "\tt_delta: " << t_delta;
    std::cout << "\tt_delta_cycles: " << t_delta_cycles;
    //std::cout << "\tmLastPhysicsEventTimeNs: " << mLastPhysicsEventTimeNs << std::endl;
  }

  return t_delta;
}


///@brief Generate a QED/Noise event
void EventGenITS::generateNextQedNoiseEvent(uint64_t event_time_ns)
{
  mUntriggeredEventCount++;

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
    pix_shared->setActiveTimeStart(event_time_ns+mPixelDeadTime);
    pix_shared->setActiveTimeEnd(event_time_ns+mPixelDeadTime+mPixelActiveTime);
    pix_shared->setPixelReadoutStatsObj(mUntriggeredReadoutStats);

    mQedNoiseHitVector.push_back(pix_shared);

    digit_it++;
  }
}


///@brief SystemC controlled method. Creates new physics events (hits)
void EventGenITS::physicsEventMethod(void)
{
  if(mStopEventGeneration == false) {
    uint64_t t_delta = generateNextPhysicsEvent();
    E_triggered_event.notify();
    next_trigger(t_delta, SC_NS);
  }
}


///@brief SystemC controlled method. Creates new QED/Noise events (hits)
void EventGenITS::qedNoiseEventMethod(void)
{
  if(mStopEventGeneration == false) {
    uint64_t time_now = sc_time_stamp().value();

    generateNextQedNoiseEvent(time_now);
    E_untriggered_event.notify();
    next_trigger(mQedNoiseFeedRateNs, SC_NS);
  }
}


void EventGenITS::stopEventGeneration(void)
{
  mStopEventGeneration = true;
  mEventHitVector.clear();
  mQedNoiseHitVector.clear();
}
