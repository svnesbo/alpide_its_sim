/**
 * @file   EventGenBase.cpp
 * @author Simon Voigt Nesbo
 * @date   November 14, 2018
 * @brief  Base class for event generator
 */
#include "EventGenBase.hpp"

EventGenBase::EventGenBase(sc_core::sc_module_name name,
                           const QSettings* settings,
                           std::string output_path)
  : sc_core::sc_module(name)
{
  mOutputPath = output_path;
  mRandomHitGeneration = settings->value("event/random_hit_generation").toBool();
  mRandomSeed = settings->value("simulation/random_seed").toInt();
  mPixelDeadTime = settings->value("alpide/pixel_shaping_dead_time_ns").toInt();
  mPixelActiveTime = settings->value("alpide/pixel_shaping_active_time_ns").toInt();
  mSingleChipSimulation = settings->value("simulation/single_chip").toBool();

  mTriggeredReadoutStats = std::make_shared<PixelReadoutStats>();
  mUntriggeredReadoutStats = std::make_shared<PixelReadoutStats>();
}

EventGenBase::~EventGenBase()
{

}

std::vector<std::shared_ptr<PixelHit>> createCluster(const PixelHit& pix,
                                                     const uint64_t& start_time_ns,
                                                     const uint64_t& dead_time_ns,
                                                     const uint64_t& active_time_ns,
                                                     const std::shared_ptr<PixelReadoutStats> &readout_stats)
{
  /*
  int cluster_size = round(rand(...))+1; // TODO: Get cluster size from gaussian distribution.
                                         // Must be at least one..
                                         // Make mean of distribution 1 less than desired, and
                                         // then always add 1?
  */
  int cluster_size = 1;

  std::vector<std::shared_ptr<PixelHit>> pixel_cluster(cluster_size);

  // Always add the "source" hit
  pixel_cluster[0] = std::make_shared<PixelHit>(pix);
  pixel_cluster[0]->setPixelReadoutStatsObj(readout_stats);

  PixelHit new_cluster_pixel;
  new_cluster_pixel.setChipId(pix.getChipId());

  // Add additional pixels to cluster, starting at index 1
  for(int i = 1; i < cluster_size; i++) {
    bool pixel_already_in_cluster = false;
    do {
      /*
      new_cluster_pixel.setCol(rand(x)); // TODO: Use guassian distribution to generate x
      new_cluster_pixel.setRow(rand(y)); // TODO: Use guassian distribution to generate x
      */

      // Check if pixel with same coords was already generated
      for(int j = 0; j < i; j++) {
        if(new_cluster_pixel == *pixel_cluster[j]) {
          pixel_already_in_cluster = true;
        }
      }
    } while(pixel_already_in_cluster != false);
    pixel_cluster[i] = std::make_shared<PixelHit>(new_cluster_pixel);
    pixel_cluster[i]->setPixelReadoutStatsObj(readout_stats);
  }

  return pixel_cluster;
}

void EventGenBase::initRandomNumGenerators(void)
{
  // Init random number generators for clustering here
}

void EventGenBase::writeSimulationStats(const std::string output_path) const
{
  mTriggeredReadoutStats->writeToFile(output_path + std::string("/triggered_readout_stats.csv"));
  mUntriggeredReadoutStats->writeToFile(output_path + std::string("/untriggered_readout_stats.csv"));
}
