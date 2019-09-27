/**
 * @file   EventGenPCT.cpp
 * @author Simon Voigt Nesbo
 * @date   November 14, 2018
 * @brief  A simple event generator for PCT simulation with Alpide SystemC simulation model.
 */

#include "EventGenPCT.hpp"
#include "Alpide/alpide_constants.hpp"
#include "Detector/PCT/PCT_constants.hpp"
#include "../utils.hpp"
#include <boost/random/random_device.hpp>
#include <stdexcept>
#include <cmath>
#include <map>
#include <QDir>

static bool comparePixelHitActiveTime(const std::shared_ptr<PixelHit>& p1, const std::shared_ptr<PixelHit>& p2);


SC_HAS_PROCESS(EventGenPCT);
///@brief Constructor for EventGenPCT
///@param[in] name SystemC module name
///@param[in] settings QSettings object with simulation settings.
///@param[in] config Detector configuration for PCT
///@param[in] output_path Directory path to store simulation output data in
EventGenPCT::EventGenPCT(sc_core::sc_module_name name,
                         const QSettings* settings,
                         const PCT::PCTDetectorConfig& config,
                         std::string output_path)
  : EventGenBase(name, settings, output_path)
  , mConfig(config)
{
  mNumStavesPerLayer = settings->value("pct/num_staves_per_layer").toUInt();
  mBeamStartCoordX_mm = settings->value("pct/beam_start_coord_x_mm").toDouble();
  mBeamStartCoordY_mm = settings->value("pct/beam_start_coord_y_mm").toDouble();

  mBeamEndCoordX_mm = settings->value("pct/beam_end_coord_x_mm").toDouble();
  mBeamEndCoordY_mm = settings->value("pct/beam_end_coord_y_mm").toDouble();

  mBeamCenterCoordX_mm = mBeamStartCoordX_mm;
  mBeamCenterCoordY_mm = mBeamStartCoordY_mm;

  mBeamStep_mm = settings->value("pct/beam_step_mm").toDouble();
  mBeamTimePerStep_us = settings->value("pct/beam_time_per_step_us").toDouble();

  mEventTimeFrameLength_ns = settings->value("pct/time_frame_length_ns").toDouble();

  if(mEventTimeFrameLength_ns > (mBeamTimePerStep_us*1000)/10) {
    std::string error_msg = "Event time frame length should be much smaller than beam time per step (1/10 minimum).";
    throw std::runtime_error(error_msg);
  }

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
  std::string physics_events_csv_filename = mOutputPath + std::string("/pct_events_data.csv");
  mPCTEventsCSVFile.open(physics_events_csv_filename);
  mPCTEventsCSVFile << "time_ns;beam_x_mm;beam_y_mm;";
  mPCTEventsCSVFile << "particle_count_total;pixel_hit_count_total";

  if(mSingleChipSimulation == false) {
    for(unsigned int layer_id = 0; layer_id < PCT::N_LAYERS; layer_id++) {
      if(mConfig.layer[layer_id].num_staves > 0) {
        std::string layer_str = std::string(";layer_") + std::to_string(layer_id);
        mPCTEventsCSVFile << ";layer_0";
      }
    }

    for(unsigned int layer_id = 0; layer_id < PCT::N_LAYERS; layer_id++) {
      for(unsigned int stave_id = 0; stave_id < mConfig.layer[layer_id].num_staves; stave_id++) {
        for(unsigned int stave_chip = 0; stave_chip < PCT::CHIPS_PER_STAVE; stave_chip++) {
          Detector::DetectorPosition pos = {layer_id, stave_id, 0, 0, stave_chip};
          mPCTEventsCSVFile << ";chip_" << PCT::PCT_position_to_global_chip_id(pos);;
        }
      }
    }
  } else { // Single chip simulation
    mPCTEventsCSVFile << ";chip_0";
  }

  mPCTEventsCSVFile << std::endl;
}


void EventGenPCT::addCsvEventLine(uint64_t time_ns,
                                  uint64_t particle_count_total,
                                  uint64_t pixel_hit_count_total,
                                  std::map<unsigned int, unsigned int> &chip_pixel_hits,
                                  std::map<unsigned int, unsigned int> &layer_pixel_hits)
{
  // Write time of event frame and particle count
  mPCTEventsCSVFile << time_ns << ";" << mBeamCenterCoordX_mm << ";" << mBeamCenterCoordY_mm;
  mPCTEventsCSVFile << ";" << particle_count_total << ";" << pixel_hit_count_total;

  if(mSingleChipSimulation == false) {
    // Write number of pixel hits for whole layers of detectors (of included layers)
    for(unsigned int layer = 0; layer < PCT::N_LAYERS; layer++) {
      if(mConfig.layer[layer].num_staves > 0)
        mPCTEventsCSVFile << ";" << layer_pixel_hits[layer];
    }

    // Write number of pixel hits for individual chips
    for(unsigned int layer_id = 0; layer_id < PCT::N_LAYERS; layer_id++) {
      for(unsigned int stave_id = 0; stave_id < mConfig.layer[layer_id].num_staves; stave_id++) {
        for(unsigned int stave_chip = 0; stave_chip < PCT::CHIPS_PER_STAVE; stave_chip++) {
          Detector::DetectorPosition pos = {layer_id, stave_id, 0, 0, stave_chip};
          mPCTEventsCSVFile << ";" << chip_pixel_hits[PCT::PCT_position_to_global_chip_id(pos)];
        }
      }
    }
  } else { // Single chip simulation
    mPCTEventsCSVFile << ";" << chip_pixel_hits[0];
  }

  mPCTEventsCSVFile << std::endl;
}


void EventGenPCT::initRandomHitGen(const QSettings* settings)
{
  double particles_per_second_mean = settings->value("pct/random_particles_per_s_mean").toDouble();
  double particles_per_second_stddev = settings->value("pct/random_particles_per_s_stddev").toDouble();
  unsigned int beam_std_dev_mm = settings->value("pct/random_beam_stddev_mm").toDouble();

  // Initialize random number distributions
  mRandHitXDist = new boost::random::normal_distribution<double>(0, beam_std_dev_mm);
  mRandHitYDist = new boost::random::normal_distribution<double>(0, beam_std_dev_mm);

  double particles_per_timeframe_mean = (mEventTimeFrameLength_ns/1E9) * particles_per_second_mean;
  double particles_per_timeframe_stddev = (mEventTimeFrameLength_ns/1E9) * particles_per_second_stddev;
  mRandParticlesPerEventFrameDist = new boost::random::normal_distribution<double>(particles_per_timeframe_mean,
                                                                                   particles_per_timeframe_stddev);

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

  if(monte_carlo_file_type == "xml") {
    std::cerr << "Error: MC files in XML format not supported for PCT simulation" << std::endl;
    exit(-1);
  }
  if(monte_carlo_file_type == "binary") {
    std::cerr << "Error: MC files in binary format not supported for PCT simulation" << std::endl;
    exit(-1);
  }
  else if(monte_carlo_file_type == "root") {
#ifdef ROOT_ENABLED
    mMCEvents = new EventRootPCT(mConfig,
                                 &PCT::PCT_global_chip_id_to_position,
                                 &PCT::PCT_position_to_global_chip_id,
                                 monte_carlo_data_file_str,
                                 mEventTimeFrameLength_ns);
#else
    std::cerr << "Error: Simulation must be compiled with ROOT support to use MC events for pCT simulation." << std::endl;
    exit(-1);
#endif
  }
  else {
    std::cerr << "Error: Unknown MC event format \"";
    std::cerr << monte_carlo_file_type.toStdString() << "\"";
    exit(-1);
  }

  // Create distribution used to "spread out" hits in time over the timeframe
  // In the ROOT event files the hits have timestamps at certain intervals
  // (10 us in the files used till now), but it is undesirable to have all the hits
  // come in big chunks at these specific intervals.
  // So we spread them out over the timeframe with a uniform distribution.
  int timeframe_length_ns = settings->value("pct/time_frame_length_ns").toInt();
  mRandHitTime = new boost::random::uniform_int_distribution<int>(0, timeframe_length_ns);
}


///@brief Destructor for EventGenPCT class
EventGenPCT::~EventGenPCT()
{
  if(mPCTEventsCSVFile.is_open())
    mPCTEventsCSVFile.close();
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


///@brief Compare the active time of two PixelHit objects. Return if p1 is active before p2.
static bool comparePixelHitActiveTime(const std::shared_ptr<PixelHit>& p1, const std::shared_ptr<PixelHit>& p2)
{
  return (p1->getActiveTimeStart() < p2->getActiveTimeStart());
}


///@brief Generate a random event, and put it in the hit vector.
///@param[out] particle_count_out Total number of particles for this event frame, excluding
///                               particles that fall outside the detector plane
///@param[out] pixel_hit_count Number of pixel hits for this event frame
///@param[out] chip_pixel_hits Map with number of pixel hits for this event per chip ID
///@param[out] layer_pixel_hits Map with number of pixel hits for this event per layer
void EventGenPCT::generateRandomEventData(unsigned int &particle_count_out,
                                          unsigned int &pixel_hit_count_out,
                                          std::map<unsigned int, unsigned int> &chip_pixel_hits,
                                          std::map<unsigned int, unsigned int> &layer_pixel_hits)
{
  uint64_t time_now = sc_time_stamp().value();

  particle_count_out = 0;
  pixel_hit_count_out = 0;

  // Clear old hit data
  mEventHitVector.clear();

  double rand_particle_count = (*mRandParticlesPerEventFrameDist)(mRandParticleCountGen);

  if(rand_particle_count < 0)
    rand_particle_count = 0;

  unsigned int num_particles_total = (unsigned int)rand_particle_count;

  std::cout << "EventGenPCT: generating " << num_particles_total << " particles" << std::endl;

  for(unsigned int particle_num = 0; particle_num < num_particles_total; particle_num++) {
    // Todo: loop over the layers?
    unsigned int layer = 0;

    double rand_x_mm = (*mRandHitXDist)(mRandHitCoordsXGen) + mBeamCenterCoordX_mm;
    double rand_y_mm = (*mRandHitYDist)(mRandHitCoordsYGen) + mBeamCenterCoordY_mm;

    // Skip pixels that fall outside the detector plane
    if(rand_x_mm < 0)
      continue;
    if(rand_y_mm < 0)
      continue;
    if(rand_x_mm > PCT::CHIPS_PER_STAVE * (CHIP_WIDTH_CM*10))
      continue;
    if(rand_y_mm > mNumStavesPerLayer * (CHIP_HEIGHT_CM*10))
      continue;

    particle_count_out++;

    unsigned int stave_chip_id =  rand_x_mm / (CHIP_WIDTH_CM*10);
    unsigned int stave_id = rand_y_mm / (CHIP_HEIGHT_CM*10);
    unsigned int global_chip_id = (layer*PCT::CHIPS_PER_LAYER)
      + (stave_id*PCT::CHIPS_PER_STAVE)
      + stave_chip_id;

    // Position of particle relative to the chip it will hit
    double chip_x_mm = rand_x_mm - (stave_chip_id*(CHIP_WIDTH_CM*10));
    double chip_y_mm = rand_y_mm - (stave_id*(CHIP_HEIGHT_CM*10));

    unsigned int chip_x_coord = round(chip_x_mm*(N_PIXEL_COLS/(CHIP_WIDTH_CM*10)));
    unsigned int chip_y_coord = round(chip_y_mm*(N_PIXEL_ROWS/(CHIP_HEIGHT_CM*10)));

    PixelHit pixel(chip_x_coord, chip_y_coord, global_chip_id);


#ifdef PIXEL_DEBUG
    std::cerr << "EventGenPCT: generated pixel X,Y(mm): " << rand_x_mm << "," << rand_y_mm;
    std::cerr << "  stave: " << stave_id << " stave chip id: " << stave_chip_id;
    std::cerr << " global chip id: " << global_chip_id;
    std::cerr << " chip X,Y: " << chip_x_coord << " " << chip_y_coord << std::endl;
#endif

    if(mRandomClusterGeneration) {
      std::vector<std::shared_ptr<PixelHit>> pix_cluster = createCluster(pixel,
                                                                         time_now,
                                                                         mPixelDeadTime,
                                                                         mPixelActiveTime,
                                                                         mUntriggeredReadoutStats);

      // Update hit counters. createCluster() only generates hits for _one_ chip,
      // if pixels are outside matrix boundaries then they are ignored. Hence it is sufficient
      // to add the number of pixels in the cluster, we don't have to check that they all
      // belong to the same chip.
      chip_pixel_hits[global_chip_id] += pix_cluster.size();
      pixel_hit_count_out += pix_cluster.size();

      // Copy pixels from cluster over to the event hit vector
      mEventHitVector.insert(mEventHitVector.end(), pix_cluster.begin(), pix_cluster.end());
    } else {
      mEventHitVector.emplace_back(std::make_shared<PixelHit>(pixel));

      // Do this after inserting (copy) of pixel, to avoid double registering of
      // readout stats when pixel is destructed
      mEventHitVector.back()->setPixelReadoutStatsObj(mUntriggeredReadoutStats);
      mEventHitVector.back()->setActiveTimeStart(time_now+mPixelDeadTime);
      mEventHitVector.back()->setActiveTimeEnd(time_now+mPixelDeadTime+mPixelActiveTime);

      chip_pixel_hits[pixel.getChipId()]++;
      pixel_hit_count_out++;
    }
  }
}


///@brief Generate a monte carlo event (ie. read it from file), and put it in the hit vector.
///@param[out] event_pixel_hit_count Total number of pixel hits for this event,
///            for all layers/chips, including chips/layers that are excluded from the simulation
///@param[out] chip_pixel_hits Map with number of pixel hits for this event per chip ID
///@param[out] layer_pixel_hits Map with number of pixel hits for this event per layer
///@return True if this is the last event, false if not
bool EventGenPCT::generateMonteCarloEventData(unsigned int &particle_count_out,
                                              unsigned int &pixel_hit_count_out,
                                              std::map<unsigned int, unsigned int> &chip_pixel_hits,
                                              std::map<unsigned int, unsigned int> &layer_pixel_hits)
{
#ifdef ROOT_ENABLED
  int x_prev = 0;
  int y_prev = 0;
  unsigned int chip_id_prev = 0;

  uint64_t time_now = sc_time_stamp().value();
  uint64_t hit_time = time_now;

  // Clear old hit data
  mEventHitVector.clear();

  std::shared_ptr<EventDigits> digits = mMCEvents->getNextEvent();

  auto digit_it = digits->getDigitsIterator();
  auto digit_end_it = digits->getDigitsEndIterator();

  while(digit_it != digit_end_it) {
    const PixelHit &pixel = *digit_it;

    if(mRandomClusterGeneration) {
      hit_time = time_now + (*mRandHitTime)(mRandHitTimeGen);

      std::vector<std::shared_ptr<PixelHit>> pix_cluster = createCluster(pixel,
                                                                         hit_time,
                                                                         mPixelDeadTime,
                                                                         mPixelActiveTime,
                                                                         mUntriggeredReadoutStats);

      // Update hit counters. createCluster() only generates hits for _one_ chip,
      // if pixels are outside matrix boundaries then they are ignored. Hence it is sufficient
      // to add the number of pixels in the cluster, we don't have to check that they all
      // belong to the same chip.
      chip_pixel_hits[pixel.getChipId()] += pix_cluster.size();
      pixel_hit_count_out += pix_cluster.size();

      // Copy pixels from cluster over to the event hit vector
      mEventHitVector.insert(mEventHitVector.end(), pix_cluster.begin(), pix_cluster.end());
    } else {
      mEventHitVector.emplace_back(std::make_shared<PixelHit>(pixel));

      // Very rudimentary algorithm for determining if pixels are in a cluster
      // Pixel hits are assumed to be in a cluster if chip id matches and the difference
      // in x/y between two hits is less than 10.
      // It is also assumed that neighboring pixels (in a cluster) appear
      // in a sequence in the ROOT data file.
      //
      // We spread out the hits over the 10 us readout frame in the ROOT files,
      // but we want all the pixel hits in a cluster to have the same timestamp,
      // which is what this is used for.
      if(pixel.getChipId() != chip_id_prev ||
         abs(pixel.getCol() - x_prev) > 10 ||
         abs(pixel.getRow() - y_prev) > 10)
      {
        hit_time = time_now + (*mRandHitTime)(mRandHitTimeGen);
      }

      // Do this after inserting (copy) of pixel, to avoid double registering of
      // readout stats when pixel is destructed
      mEventHitVector.back()->setPixelReadoutStatsObj(mUntriggeredReadoutStats);
      mEventHitVector.back()->setActiveTimeStart(hit_time+mPixelDeadTime);
      mEventHitVector.back()->setActiveTimeEnd(hit_time+mPixelDeadTime+mPixelActiveTime);

      unsigned int layer_id = PCT::PCT_global_chip_id_to_position(pixel.getChipId()).layer_id;

      // Increase pixel hit counters
      layer_pixel_hits[layer_id]++;
      chip_pixel_hits[pixel.getChipId()]++;
      pixel_hit_count_out++;

      // Remember hit position, to determine if next pixel hit
      // is in a new cluster or not
      x_prev = pixel.getCol();
      y_prev = pixel.getRow();
      chip_id_prev = pixel.getChipId();
    }

    digit_it++;
  }

  // Sort the hits in the frame, because the Alpide front end code
  // assumes that chips are inputted in the order that they become active
  std::sort(mEventHitVector.begin(), mEventHitVector.end(), comparePixelHitActiveTime);

  return !mMCEvents->getMoreEventsLeft();

#else
  std::cerr << "Error: generateMonteCarloEventData() called, but simulation not compiled with ROOT support." << std::endl;
  exit(-1);
#endif
}


///@brief Generate an event, either random or using data for MC files
///@return For MC events: True when last event was generated. For random events, always false.
bool EventGenPCT::generateEvent(void)
{
  uint64_t time_now = sc_time_stamp().value();
  unsigned int event_particle_count = 0;
  unsigned int event_pixel_hit_count = 0;

  bool last_event = false;


  std::map<unsigned int, unsigned int> layer_pixel_hits;
  std::map<unsigned int, unsigned int> chip_pixel_hits;

  mUntriggeredEventCount++;

  if(mRandomHitGeneration == true) {
    generateRandomEventData(event_particle_count, event_pixel_hit_count,
                            chip_pixel_hits, layer_pixel_hits);
  } else {
    last_event = generateMonteCarloEventData(event_particle_count, event_pixel_hit_count,
                                             chip_pixel_hits, layer_pixel_hits);
  }

  // Write event rate and multiplicity numbers to CSV file
  if(mCreateCSVFile)
    addCsvEventLine(time_now,
                    event_particle_count,
                    event_pixel_hit_count,
                    chip_pixel_hits,
                    layer_pixel_hits);

  std::cout << "@ " << time_now << " ns: ";
  std::cout << "\tEvent number: " << mUntriggeredEventCount;

  return last_event;
}


void EventGenPCT::updateBeamPosition(void)
{
  uint64_t time_now_ns = sc_time_stamp().value();
  uint64_t beam_step_num = (time_now_ns/1000)/mBeamTimePerStep_us;

  if(beam_step_num > mBeamStepCounter) {
    mBeamStepCounter++;

    if(mBeamCenterCoordX_mm >= mBeamEndCoordX_mm && mBeamCenterCoordY_mm >= mBeamEndCoordY_mm)
      mBeamEndCoordsReached = true;

    // Beam direction is initialized to start moving to the right in the class
    if(mBeamDirectionRight) {
      // Move beam down (y-direction) if we've reached the right end point,
      // and start moving the beam back towards the left
      if(mBeamCenterCoordX_mm >= mBeamEndCoordX_mm) {
        mBeamCenterCoordY_mm += mBeamStep_mm;
        mBeamDirectionRight = false;
      } else {
        mBeamCenterCoordX_mm += mBeamStep_mm;
      }
    } else {
      // Move beam down (y-direction) if we've reached the left end point,
      // and start moving the beam back towards the left
      if(mBeamCenterCoordX_mm <= mBeamStartCoordX_mm) {
        mBeamCenterCoordY_mm += mBeamStep_mm;
        mBeamDirectionRight = true;
      } else {
        mBeamCenterCoordX_mm -= mBeamStep_mm;
      }
    }
  }
}


///@brief SystemC controlled method. Creates new physics events (hits)
void EventGenPCT::physicsEventMethod(void)
{
  if(mStopEventGeneration == false && mBeamEndCoordsReached == false) {
    bool last_mc_event = generateEvent();

    if(mRandomHitGeneration)
      updateBeamPosition();
    else
      // Beam position is included in MC events, stop at last event
      mBeamEndCoordsReached = last_mc_event;

    E_untriggered_event.notify();
    next_trigger(mEventTimeFrameLength_ns, SC_NS);
  }
}


void EventGenPCT::stopEventGeneration(void)
{
  mStopEventGeneration = true;
  mEventHitVector.clear();
}
