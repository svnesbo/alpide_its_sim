/**
 * @file   EventGenBase.cpp
 * @author Simon Voigt Nesbo
 * @date   November 14, 2018
 * @brief  Base class for event generator
 */


EventGenBase::EventGenBase(const QSettings* settings, std::string output_path)
{
  mOutputPath = output_path;
  mRandomSeed = settings->value("simulation/random_seed").toInt();
  mPixelDeadTime = settings->value("alpide/pixel_shaping_dead_time_ns").toInt();
  mPixelActiveTime = settings->value("alpide/pixel_shaping_active_time_ns").toInt();
  mSingleChipSimulation = settings->value("simulation/single_chip").toBool();

  mPhysicsReadoutStats = std::make_shared<PixelReadoutStats>();
  mQedReadoutStats = std::make_shared<PixelReadoutStats>();
}

EventGenBase::~EventGenBase()
{

}

virtual void EventGenBase::initRandomNumGenerators(void)
{

}

void EventGenBase::setRandomSeed(int seed)
{

}

void EventGenBase::stopEventGeneration(void)
{

}

void EventGenBase::writeSimulationStats(const std::string output_path) const
{
  mTriggeredReadoutStats->writeToFile(output_path + std::string("/triggered_readout_stats.csv"));
  mUntriggeredReadoutStats->writeToFile(output_path + std::string("/untriggered_readout_stats.csv"));
}
