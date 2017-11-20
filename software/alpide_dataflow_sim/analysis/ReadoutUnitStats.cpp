/**
 * @file   ReadoutUnitStats.cpp
 * @author Simon Voigt Nesbo
 * @date   November 20, 2017
 * @brief  ReadoutUnitStats member functions for analyzing
 *         a RU_<layer>_<stave>_Trigger_stats.csv file
 *
 */


#include "ReadoutUnitStats.hpp"
#include <sstream>
#include <fstream>
#include <iostream>


ReadoutUnitStats::ReadoutUnitStats(unsigned int layer, unsigned int stave, const char* path)
  : mLayer(layer)
  , mStave(stave)
{
  std::stringstream ss;
  ss << path << "/" << "RU_" << layer << "_" << stave << "_Trigger_stats.dat";

  readFile(ss.str());
}


void ReadoutUnitStats::readFile(std::string filename)
{
  std::cout << "Opening file: " << filename << std::endl;
  std::ifstream ru_stats_file(filename, std::ios_base::in | std::ios_base::binary);

  if(!ru_stats_file.is_open()) {
    std::cerr << "Error opening file " << filename << std::endl;
    exit(-1);
  }

  uint64_t num_triggers;
  uint8_t num_links;

  std::cout << "sizeof(uint8_t): " << sizeof(uint8_t) << std::endl;

  ru_stats_file.read((char*)&num_triggers, sizeof(num_triggers));
  ru_stats_file.read((char*)&num_links, sizeof(num_links));

  mNumTriggers = num_triggers;
  mNumLinks = num_links;

  std::cout << "Num triggers: " << num_triggers << std::endl;
  std::cout << "Num links: " << mNumLinks << std::endl;

  mTriggerCoverage.resize(num_triggers);
  mTriggerActions.resize(num_triggers);

  uint64_t trigger_id = 0;
  uint8_t trig_action = 0;
  uint64_t unknown_trig_action_count = 0;

  // Read trigger action byte, one per link, for each trigger ID,
  // and calculate coverage etc.
  while(trigger_id < num_triggers && ru_stats_file.good()) {
    uint8_t coverage = 0;
    uint8_t links_included = 0;
    uint8_t links_filtered = 0;

    for(unsigned int link_id = 0; link_id < num_links; link_id++) {
      ru_stats_file.read((char*)&trig_action, sizeof(uint8_t));
      //mTriggerActions[trigger_id][link_id] = trig_action;
      mTriggerActions[trigger_id].push_back(trig_action);

      std::cout << "Trigger " << trigger_id;
      std::cout << ": RU" << mLayer << "." << mStave << ": ";

      switch(trig_action) {
      case TRIGGER_SENT:
        std::cout << "TRIGGER_SENT" << std::endl;
        coverage++;
        links_included++;
        break;

      case TRIGGER_NOT_SENT_BUSY:
        std::cout << "TRIGGER_NOT_SENT_BUSY" << std::endl;
        links_included++;
        break;

      case TRIGGER_FILTERED:
        std::cout << "TRIGGER_FILTERED" << std::endl;
        links_filtered++;
        break;
      default:
        std::cout << "UNKNOWN" << std::endl;
        unknown_trig_action_count++;
        break;
      }
    }

    mTriggerCoverage[trigger_id] = (double)coverage/links_included;

    std::cout << "Trigger " << trigger_id;
    std::cout << " coverage: " << mTriggerCoverage[trigger_id] << std::endl;

    if(links_filtered != 0 && links_filtered != num_links)
      mTriggerMismatch.push_back(trigger_id);

    trigger_id++;
  }

  if(trigger_id != num_triggers) {
    std::cerr << "Error reading " << num_triggers << ", got only " << trigger_id << std::endl;
    exit(-1);
  }

  std::cout << "Number of unknown trigger actions: " << unknown_trig_action_count << std::endl;

  std::cout << "Links with filter mismatch: ";
  for(auto it = mTriggerMismatch.begin(); it != mTriggerMismatch.end(); it++) {
    if(it == mTriggerMismatch.begin())
       std::cout << *it;
    else
      std::cout << ", " << *it;
  }
  std::cout << std::endl;



}
