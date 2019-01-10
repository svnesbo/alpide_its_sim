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
  : EventGenBase(name, settings, output_path)
{
  mRandomHitGeneration = settings->value("event/random_hit_generation").toBool();
  mCreateCSVFile = settings->value("data_output/write_event_csv").toBool();

  mBeamCenterCoordX_mm = settings->value("pct/beam_center_coord_x_start_mm").toDouble();
  mBeamCenterCoordY_mm = settings->value("pct/beam_center_coord_y_start_mm").toDouble();

  mBeamSpeedX_mm_per_us = settings->value("pct/beam_speed_x_mm_per_us").toDouble();
  mBeamSpeedY_mm_per_us = settings->value("pct/beam_speed_y_mm_per_us").toDouble();

  mEventTimeFrameLength_ns = settings->value("pct/time_frame_length_ns").toDouble();

  if(mRandomHitGeneration) {
    initRandomHitGen(settings);
  } else {
    initMonteCarloHitGen(settings);
  }

  initRandomNumGenerators();

  if(mCreateCSVFile)
    initCsvEventFileHeader();

  //////////////////////////////////////////////////////////////////////////////
  // SystemC declarations / connections / etc.
  //////////////////////////////////////////////////////////////////////////////
  SC_METHOD(eventMethod);
}


void EventGenPCT::initRandomHitGen(const QSettings* settings)
{
  mRandomFluxMean_per_second = settings->value("pct/random_flux_mean_per_s").toDouble();
  mRandomFluxStdDev_per_second = settings->value("pct/random_flux_stddev_per_s").toDouble();
  mRandomBeamDiameterMean_mm = settings->value("pct/random_beam_diameter_mean_mm").toDouble();
  mRandomBeamDiameterStdDev_mm = settings->value("pct/random_beam_diameter_stddev_mm").toDouble();

  ///@todo Initialize random number generators here
}


void EventGenPCT::initMonteCarloHitGen(const QSettings* settings)
{
  QString monte_carlo_file_type = settings->value("event/monte_carlo_file_type").toString();
  QString monte_carlo_data_file_str = settings->value("pct/monte_carlo_file_path").toString();

  if(monte_carlo_file_type == "xml") {
    name_filters << "*.xml";
    QStringList MC_files = monte_carlo_event_dir.entryList(name_filters);

    if(MC_files.isEmpty()) {
      std::cerr << "Error: No .xml files found in MC event path";
      std::cerr << std::endl;
      exit(-1);
    }

    mMCPhysicsEvents = new EventXML(mITSConfig,
                                    monte_carlo_data_file_str,
                                    /* TODO: Add more params here... */
                                    mRandomSeed);
  }
  else {
    throw std::runtime_error("Only XML type Monte Carlo files supported for PCT simulation.");
  }
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
  EventBase::initRandomNumGenerators();
}


///@brief Generate a random event, and put it in the hit vector.
///@param[out] event_pixel_hit_count Total number of pixel hits for this event,
///            for all layers/chips, including chips/layers that are excluded from the simulation
///@param[out] chip_hits Map with number of pixel hits for this event per chip ID
///@param[out] layer_hits Map with number of pixel hits for this event per layer
void EventGenITS::generateRandomEventData(unsigned int &event_pixel_hit_count,
                                          std::map<unsigned int, unsigned int> &chip_hits,
                                          std::map<unsigned int, unsigned int> &layer_hits)
{
  uint64_t time_now = sc_time_stamp().value();

  // Clear old hit data
  mEventHitVector.clear();

  unsigned int num_hits_total = rand(...); // Use gaussian here..

  auto digit_it = digits->getDigitsIterator();
  auto digit_end_it = digits->getDigitsEndIterator();

  event_pixel_hit_count = digits->size();

  for(unsigned int hit_num = 0; hit_num < num_hits_total; hit_num++) {
    unsigned int rand_x1 = (*mRandHitChipX)(mRandHitGen);
    unsigned int rand_y1 = (*mRandHitChipY)(mRandHitGen);

    std::shared_ptr<PixelHit> pixel = std::make_shared<PixelHit>(rand_x1, rand_y1, 0);

    std::vector<std::shared_ptr<PixelHit>> pix_cluster = createCluster(pixel,
                                                                       time_now,
                                                                       mPixelDeadTime,
                                                                       mPixelActiveTime,
                                                                       mUntriggeredReadoutStats);

    mEventHitVector.insert(mEventHitVector.end(),
                           pix_cluster.begin(),
                           pix_cluster.end());

    //layer_hits[pos.layer_id]++;
    chip_hits[pix.getChipId()]++;
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
  uint64_t time_now = sc_time_stamp().value();

  // Clear old hit data
  mEventHitVector.clear();

  const EventDigits* digits = mMCPhysicsEvents->getNextEvent();

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
}


void EventGenPCT::generateEvent(void)
{
  uint64_t time_now = sc_time_stamp().value();
  unsigned int event_pixel_hit_count = 0;


  std::map<unsigned int, unsigned int> layer_hits;
  std::map<unsigned int, unsigned int> chip_hits;

  mLastPhysicsEventTimeNs = time_now;
  mPhysicsEventCount++;

  if(mRandomHitGeneration == true) {
    generateRandomEventData(event_pixel_hit_count, chip_hits, layer_hits);
  } else {
    generateMonteCarloEventData(event_pixel_hit_count, chip_hits, layer_hits);
  }

  // Write event rate and multiplicity numbers to CSV file
  if(mCreateCSVFile)
    addCsvEventLine(mEventTimeFrameLength_ns, event_pixel_hit_count, chip_hits, layer_hits);

  std::cout << "@ " << time_now << " ns: ";
  std::cout << "\tPhysics event number: " << mPhysicsEventCount;
}


///@brief SystemC controlled method. Creates new physics events (hits)
void EventGenPCT::physicsEventMethod(void)
{
  if(mStopEventGeneration == false) {
    generateNextPhysicsEvent();
    E_physics_event.notify();
    next_trigger(mEventTimeFrameLength_ns, SC_NS);
  }
}


void EventGenPCT::stopEventGeneration(void)
{
  mStopEventGeneration = true;
  mEventHitVector.clear();
}
