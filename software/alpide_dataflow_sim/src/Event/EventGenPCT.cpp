/**
 * @file   EventGenPCT.cpp
 * @author Simon Voigt Nesbo
 * @date   November 14, 2018
 * @brief  A simple event generator for PCT simulation with Alpide SystemC simulation model.
 */

#include "EventGenPCT.hpp"
#include "Alpide/alpide_constants.hpp"
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
  : EventGenBase(name, settings, output_path)
{
  mNumLayers = settings->value("pct/num_layers").toUInt();
  mNumStavesPerLayer = settings->value("pct/num_staves_per_layer").toUInt();
  mBeamStartCoordX_mm = settings->value("pct/beam_start_coord_x_mm").toDouble();
  mBeamStartCoordY_mm = settings->value("pct/beam_start_coord_y_mm").toDouble();

  mBeamEndCoordX_mm = settings->value("pct/beam_end_coord_x_mm").toDouble();
  mBeamEndCoordY_mm = settings->value("pct/beam_end_coord_y_mm").toDouble();

  mBeamCenterCoordX_mm = mBeamStartCoordX_mm;
  mBeamCenterCoordY_mm = mBeamStartCoordY_mm;

  mBeamSpeedX_mm_per_us = settings->value("pct/beam_speed_x_mm_per_us").toDouble();
  mBeamStepY_mm = settings->value("pct/beam_step_y_mm").toDouble();

  mEventTimeFrameLength_ns = settings->value("pct/time_frame_length_ns").toDouble();

  if(mRandomHitGeneration) {
    initRandomHitGen(settings);
  } else {
    initMonteCarloHitGen(settings);
  }

  if(mCreateCSVFile)
    initCsvEventFileHeader(settings);

  //////////////////////////////////////////////////////////////////////////////
  // SystemC declarations / connections / etc.
  //////////////////////////////////////////////////////////////////////////////
  SC_METHOD(physicsEventMethod);
}


void EventGenPCT::initCsvEventFileHeader(const QSettings* settings)
{
  // TODO
  /*
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
      // Safe to ignore OB sub staves here, since we only include full staves in simulation,
      // and this will include all chips from a full stave
      for(unsigned int stave_chip = 0; stave_chip < ITS::CHIPS_PER_STAVE_IN_LAYER[layer]; stave_chip++) {
        mPhysicsEventsCSVFile << ";chip_" << chip_id;
        chip_id++;
      }
    }
  }

  mPhysicsEventsCSVFile << std::endl;
  */
}


void EventGenPCT::addCsvEventLine(uint64_t t_delta,
                                  unsigned int event_pixel_hit_count,
                                  std::map<unsigned int, unsigned int> &chip_hits,
                                  std::map<unsigned int, unsigned int> &layer_hits)
{
  // TODO
  /*
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
  */
}


void EventGenPCT::initRandomHitGen(const QSettings* settings)
{
  unsigned int particles_per_second = settings->value("pct/random_particles_per_s").toDouble();
  unsigned int beam_std_dev_mm = settings->value("pct/random_beam_stddev_mm").toDouble();

  // Initialize random number distributions
  mRandHitXDist = new boost::random::normal_distribution<double>(0, beam_std_dev_mm);
  mRandHitYDist = new boost::random::normal_distribution<double>(0, beam_std_dev_mm);

  double particles_per_timeframe_mean = (mEventTimeFrameLength_ns/1E9) * particles_per_second;
  mRandParticlesPerEventFrameDist = new boost::random::poisson_distribution<uint32_t,double>(particles_per_timeframe_mean);

  // Initialize random number generators
  // If seed was set to 0 in settings file, initialize with a non-deterministic
  // higher entropy random number using boost::random::random_device
  // Based on this: http://stackoverflow.com/a/13004555
  if(mRandomSeed == 0) {
    boost::random::random_device r;

    std::cout << "Boost random_device entropy: " << r.entropy() << std::endl;

    unsigned int random_seed = r();
    mRandParticleCountGen.seed(random_seed);
    mRandHitCoordsXGen.seed(random_seed);
    mRandHitCoordsYGen.seed(random_seed);
    std::cout << "EventGenPCT random seed: " << random_seed << std::endl;
  } else {
    mRandParticleCountGen.seed(mRandomSeed);
    mRandHitCoordsXGen.seed(mRandomSeed);
    mRandHitCoordsYGen.seed(mRandomSeed);
  }
}


void EventGenPCT::initMonteCarloHitGen(const QSettings* settings)
{
  QString monte_carlo_file_type = settings->value("event/monte_carlo_file_type").toString();
  QString monte_carlo_data_file_str = settings->value("pct/monte_carlo_file_path").toString();


  /* if(monte_carlo_file_type == "xml") { */
  /*   name_filters << "*.xml"; */
  /*   QStringList MC_files = monte_carlo_event_dir.entryList(name_filters); */

  /*   if(MC_files.isEmpty()) { */
  /*     std::cerr << "Error: No .xml files found in MC event path"; */
  /*     std::cerr << std::endl; */
  /*     exit(-1); */
  /*   } */
  /*   // TODO: Finish this */
  /*   //mMCPhysicsEvents = new EventXML(mITSConfig, */
  /*   //                                monte_carlo_data_file_str, */
  /*   //                                /\* TODO: Add more params here... *\/ */
  /*   //                                mRandomSeed); */
  /* } */
  /* else { */
  /*   throw std::runtime_error("Only XML type Monte Carlo files supported for PCT simulation."); */
  /* } */

}


///@brief Destructor for EventGenPCT class
EventGenPCT::~EventGenPCT()
{
  if(mPhysicsEventsCSVFile.is_open())
    mPhysicsEventsCSVFile.close();
}


///@brief Get a reference to the next "triggered" event. Not used in this event generator
///@return Const reference to an empty std::vector<std::shared_ptr<PixelHit>>
const std::vector<std::shared_ptr<PixelHit>>& EventGenPCT::getTriggeredEvent(void) const
{
  static std::vector<std::shared_ptr<PixelHit>> empty_vec;

  return empty_vec;
}


///@brief Get a reference to the next "untriggered" event. In this event generator this is
///       used for the particle hits for the pencil beam, since they are continuously
///       hitting the detector, and you don't trigger on any kind of collision/event.
///@return Const reference to std::vector<Hit> that contains the hits in the latest event.
const std::vector<std::shared_ptr<PixelHit>>& EventGenPCT::getUntriggeredEvent(void) const
{
  return mEventHitVector;
}


///@brief Generate a random event, and put it in the hit vector.
///@param[out] event_pixel_hit_count Total number of pixel hits for this event,
///            for all layers/chips, including chips/layers that are excluded from the simulation
///@param[out] chip_hits Map with number of pixel hits for this event per chip ID
///@param[out] layer_hits Map with number of pixel hits for this event per layer
void EventGenPCT::generateRandomEventData(unsigned int &event_pixel_hit_count,
                                          std::map<unsigned int, unsigned int> &chip_hits,
                                          std::map<unsigned int, unsigned int> &layer_hits)
{
  uint64_t time_now = sc_time_stamp().value();

  // Clear old hit data
  mEventHitVector.clear();

  unsigned int num_particles_total = (*mRandParticlesPerEventFrameDist)(mRandParticleCountGen);

  std::cout << "EventGenPCT: generating " << num_particles_total << " particles" << std::endl;

  for(unsigned int particle_num = 0; particle_num < num_particles_total; particle_num++) {
    double rand_x_mm = (*mRandHitXDist)(mRandHitCoordsXGen) + mBeamCenterCoordX_mm;
    double rand_y_mm = (*mRandHitYDist)(mRandHitCoordsYGen) + mBeamCenterCoordY_mm;

    // Skip pixels that fall outside the detector plane
    if(rand_x_mm < 0)
      continue;
    if(rand_y_mm < 0)
      continue;
    if(rand_x_mm > ITS::CHIPS_PER_IB_STAVE * (CHIP_WIDTH_CM*10))
      continue;
    if(rand_y_mm > mNumStavesPerLayer * (CHIP_HEIGHT_CM*10))
      continue;

    unsigned int chip_num = rand_x_mm / (CHIP_WIDTH_CM*10);
    unsigned int stave_num = rand_y_mm / (CHIP_HEIGHT_CM*10);

    // Position of particle relative to the chip it will hit
    double chip_x_mm = rand_x_mm - (chip_num*(CHIP_WIDTH_CM*10));
    double chip_y_mm = rand_y_mm - (stave_num*(CHIP_HEIGHT_CM*10));

    unsigned int chip_x_coord = round(chip_x_mm*(N_PIXEL_COLS/(CHIP_WIDTH_CM*10)));
    unsigned int chip_y_coord = round(chip_y_mm*(N_PIXEL_ROWS/(CHIP_HEIGHT_CM*10)));

    PixelHit pixel(chip_x_coord, chip_y_coord);

    if(mRandomClusterGeneration) {
      std::vector<std::shared_ptr<PixelHit>> pix_cluster = createCluster(pixel,
                                                                         time_now,
                                                                         mPixelDeadTime,
                                                                         mPixelActiveTime,
                                                                         mUntriggeredReadoutStats);

      for(auto pix_it = mEventHitVector.begin(); pix_it != mEventHitVector.end(); pix_it++)
        chip_hits[(*pix_it)->getChipId()]++;

      mEventHitVector.insert(mEventHitVector.end(), pix_cluster.begin(), pix_cluster.end());
    } else {
      mEventHitVector.emplace_back(std::make_shared<PixelHit>(pixel));

      // Do this after inserting (copy) of pixel, to avoid double registering of
      // readout stats when pixel is destructed
      mEventHitVector.back()->setPixelReadoutStatsObj(mUntriggeredReadoutStats);

      chip_hits[pixel.getChipId()]++;
    }
  }
}


///@brief Generate a monte carlo event (ie. read it from file), and put it in the hit vector.
///@param[out] event_pixel_hit_count Total number of pixel hits for this event,
///            for all layers/chips, including chips/layers that are excluded from the simulation
///@param[out] chip_hits Map with number of pixel hits for this event per chip ID
///@param[out] layer_hits Map with number of pixel hits for this event per layer
void EventGenPCT::generateMonteCarloEventData(unsigned int &event_pixel_hit_count,
                                              std::map<unsigned int, unsigned int> &chip_hits,
                                              std::map<unsigned int, unsigned int> &layer_hits)
{
  /*
  uint64_t time_now = sc_time_stamp().value();

  // Clear old hit data
  mEventHitVector.clear();

  const EventDigits* digits = mMCPhysicsEvents->getNextEvent(time_now, mEventtimeframelength);

  if(digits == nullptr)
    throw std::runtime_error("EventDigits::getNextEvent() returned no new Monte Carlo event.");

  auto digit_it = digits->getDigitsIterator();
  auto digit_end_it = digits->getDigitsEndIterator();

  event_pixel_hit_count = digits->size();

  while(digit_it != digit_end_it) {
    const PixelHit &pix = *digit_it;

    std::vector<std::shared_ptr<PixelHit>> pix_cluster = createCluster(pix,
                                                                       time_now,
                                                                       mPixelDeadTime,
                                                                       mPixelActiveTime,
                                                                       mUntriggeredReadoutStats);

    mEventHitVector.insert(mEventHitVector.end(),
                           pix_cluster.begin(),
                           pix_cluster.end());

    //layer_hits[pos.layer_id]++;
    chip_hits[pix.getChipId()]++;

    digit_it++;
  }
  */
}


void EventGenPCT::generateEvent(void)
{
  uint64_t time_now = sc_time_stamp().value();
  unsigned int event_pixel_hit_count = 0;


  std::map<unsigned int, unsigned int> layer_hits;
  std::map<unsigned int, unsigned int> chip_hits;

  mUntriggeredEventCount++;

  if(mRandomHitGeneration == true) {
    generateRandomEventData(event_pixel_hit_count, chip_hits, layer_hits);
  } else {
    generateMonteCarloEventData(event_pixel_hit_count, chip_hits, layer_hits);
  }

  // Write event rate and multiplicity numbers to CSV file
  if(mCreateCSVFile)
    addCsvEventLine(mEventTimeFrameLength_ns, event_pixel_hit_count, chip_hits, layer_hits);

  std::cout << "@ " << time_now << " ns: ";
  std::cout << "\tEvent number: " << mUntriggeredEventCount;
}


void EventGenPCT::updateBeamPosition(void)
{
  if(mBeamCenterCoordX_mm >= mBeamEndCoordX_mm && mBeamCenterCoordY_mm >= mBeamEndCoordY_mm)
    mBeamEndCoordsReached = true;

  // Beam direction is initialized to start moving to the right in the class
  if(mBeamDirectionRight) {
    // Move beam down (y-direction) if we've reached the right end point,
    // and start moving the beam back towards the left
    if(mBeamCenterCoordX_mm >= mBeamEndCoordX_mm) {
      mBeamCenterCoordY_mm += mBeamStepY_mm;
      mBeamDirectionRight = false;
    } else {
      mBeamCenterCoordX_mm += (mBeamSpeedX_mm_per_us/1000) * mEventTimeFrameLength_ns;
    }
  } else {
    // Move beam down (y-direction) if we've reached the left end point,
    // and start moving the beam back towards the left
    if(mBeamCenterCoordX_mm <= mBeamStartCoordX_mm) {
      mBeamCenterCoordY_mm += mBeamStepY_mm;
      mBeamDirectionRight = true;
    } else {
      mBeamCenterCoordX_mm -= (mBeamSpeedX_mm_per_us/1000) * mEventTimeFrameLength_ns;
    }
  }
}


///@brief SystemC controlled method. Creates new physics events (hits)
void EventGenPCT::physicsEventMethod(void)
{
  if(mStopEventGeneration == false && mBeamEndCoordsReached == false) {
    generateEvent();
    updateBeamPosition();

    E_untriggered_event.notify();
    next_trigger(mEventTimeFrameLength_ns, SC_NS);
  }
}


void EventGenPCT::stopEventGeneration(void)
{
  mStopEventGeneration = true;
  mEventHitVector.clear();
}
