/**
 * @file   ReadoutUnitStats.hpp
 * @author Simon Voigt Nesbo
 * @date   November 20, 2017
 * @brief  Object for analyzing a RU_<layer>_<stave>_Trigger_stats.csv file
 *
 */

//#include <cstdint>
#include <stdint.h>
#include <vector>
#include <map>

//using std::uint8_t;

const uint8_t TRIGGER_SENT = 0;
const uint8_t TRIGGER_NOT_SENT_BUSY = 1;
const uint8_t TRIGGER_FILTERED = 2;

class ReadoutUnitStats {
  // Indexing: [trigger_id][link_id]
  std::vector<std::vector<uint8_t>> mTriggerActions;

  // Index: trigger id
  std::vector<double> mTriggerCoverage;

  // Trigger IDs where there was a mismatch in the
  // trigger filter status. Either all or no links should
  // have the TRIGGER_FILTERED state, if there are trigger
  // IDs where that was not the case, then they are put in this
  // vector.
  std::vector<uint64_t> mTriggerMismatch;

  uint64_t mNumTriggers;
  unsigned int mNumLinks;
  unsigned int mLayer;
  unsigned int mStave;

public:
  ReadoutUnitStats(unsigned int layer, unsigned int stave, const char* path);
  void readFile(std::string filename);
};
