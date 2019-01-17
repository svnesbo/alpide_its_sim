/**
 * @file   EventGenBase.cpp
 * @author Simon Voigt Nesbo
 * @date   November 14, 2018
 * @brief  Base class for event generator
 */
#include "EventGenBase.hpp"
#include <boost/random/random_device.hpp>

EventGenBase::EventGenBase(sc_core::sc_module_name name,
                           const QSettings* settings,
                           std::string output_path)
  : sc_core::sc_module(name)
{
  mOutputPath = output_path;
  mCreateCSVFile = settings->value("data_output/write_event_csv").toBool();
  mRandomHitGeneration = settings->value("event/random_hit_generation").toBool();
  mRandomClusterGeneration = settings->value("event/random_cluster_generation").toBool();
  mRandomSeed = settings->value("simulation/random_seed").toInt();
  mPixelDeadTime = settings->value("alpide/pixel_shaping_dead_time_ns").toInt();
  mPixelActiveTime = settings->value("alpide/pixel_shaping_active_time_ns").toInt();
  mSingleChipSimulation = settings->value("simulation/single_chip").toBool();

  mTriggeredReadoutStats = std::make_shared<PixelReadoutStats>();
  mUntriggeredReadoutStats = std::make_shared<PixelReadoutStats>();

  if(mRandomClusterGeneration)
    initRandomClusterGen(settings);
}

EventGenBase::~EventGenBase()
{

}

std::vector<std::shared_ptr<PixelHit>>
EventGenBase::createCluster(const PixelHit& pix,
                            const uint64_t& start_time_ns,
                            const uint64_t& dead_time_ns,
                            const uint64_t& active_time_ns,
                            const std::shared_ptr<PixelReadoutStats> &readout_stats)
{
  bool pixel_already_in_cluster;

  // Cluster distribution is initialized with mean-1,
  // to account for there always being 1 pixel in a cluster
  int cluster_size = round((*mRandClusterSizeDist)(mRandClusterSizeGen)) + 1;

  // Don't allow smaller clusters than 1, since gaussian distribution may return
  // negative numbers. Obviously we can't allocate negative amount of space
  // in vector below, and also the cluster should always contain a pixel at
  // the base coordinates.
  if(cluster_size < 1)
    cluster_size = 1;

  std::cout << "Cluster size: " << cluster_size << std::endl;

  std::vector<std::shared_ptr<PixelHit>> pixel_cluster(cluster_size);

  // Always add the "source" hit
  pixel_cluster[0] = std::make_shared<PixelHit>(pix);
  pixel_cluster[0]->setPixelReadoutStatsObj(readout_stats);

  PixelHit new_cluster_pixel;
  new_cluster_pixel.setChipId(pix.getChipId());

  // Skip first pixel, as it has the base/source coordinates of the hit
  // and is already added to the vector
  for(int i = 1; i < cluster_size; i++) {
    do {
      pixel_already_in_cluster = false;

      double rand_x = (*mRandClusterXDist)(mRandClusterXGen);
      double rand_y = (*mRandClusterYDist)(mRandClusterYGen);

      rand_x  = round(rand_x);
      rand_y  = round(rand_y);

      // Create random cluster pixels around base coordinate
      //new_cluster_pixel.setCol(pix.getCol() + round((*mRandClusterXDist)(mRandClusterXGen)));
      //new_cluster_pixel.setRow(pix.getRow() + round((*mRandClusterYDist)(mRandClusterYGen)));

      new_cluster_pixel.setCol(pix.getCol() + rand_x);
      new_cluster_pixel.setRow(pix.getRow() + rand_y);

      // Check if pixel with same coords was already generated
      for(int j = 0; j < i; j++) {
        if(new_cluster_pixel == *pixel_cluster[j]) {
          pixel_already_in_cluster = true;
        }
      }
    } while(pixel_already_in_cluster == true);
    pixel_cluster[i] = std::make_shared<PixelHit>(new_cluster_pixel);
    pixel_cluster[i]->setPixelReadoutStatsObj(readout_stats);
  }

  return pixel_cluster;
}

void EventGenBase::initRandomClusterGen(const QSettings* settings)
{
  double cluster_size_mean = settings->value("event/random_cluster_size_mean").toDouble();
  double cluster_size_stddev = settings->value("event/random_cluster_size_stddev").toDouble();

  // Initialize random number distributions. Set to mean of distribution to desired
  // mean value minus one, to account for the base pixel that is always in the cluster
  mRandClusterSizeDist = new boost::random::normal_distribution<double>(cluster_size_mean-1,
                                                                        cluster_size_stddev);
  mRandClusterXDist = new boost::random::normal_distribution<double>(0, sqrt(cluster_size_mean));
  mRandClusterYDist = new boost::random::normal_distribution<double>(0, sqrt(cluster_size_mean));

  // Initialize random number generators
  // If seed was set to 0 in settings file, initialize with a non-deterministic
  // higher entropy random number using boost::random::random_device
  // Based on this: http://stackoverflow.com/a/13004555
  if(mRandomSeed == 0) {
    boost::random::random_device r;

    std::cout << "Boost random_device entropy: " << r.entropy() << std::endl;

    unsigned int random_seed = r();
    mRandClusterSizeGen.seed(random_seed);
    mRandClusterXGen.seed(random_seed);
    mRandClusterYGen.seed(random_seed);
  } else {
    mRandClusterSizeGen.seed(mRandomSeed);
    mRandClusterXGen.seed(mRandomSeed);
    mRandClusterYGen.seed(mRandomSeed);
  }
}

void EventGenBase::writeSimulationStats(const std::string output_path) const
{
  mTriggeredReadoutStats->writeToFile(output_path + std::string("/triggered_readout_stats.csv"));
  mUntriggeredReadoutStats->writeToFile(output_path + std::string("/untriggered_readout_stats.csv"));
}
