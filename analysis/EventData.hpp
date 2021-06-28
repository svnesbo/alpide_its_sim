/**
 * @file   EventData.hpp
 * @author Simon Voigt Nesbo
 * @date   September 11, 2019
 * @brief  Time between events and multiplicity data
 *
 */

#ifndef EVENT_DATA_HPP
#define EVENT_DATA_HPP

#include <stdint.h>
#include <vector>
#include <map>


struct EventData {
  // Title/name for the set of data in each inner vector in multipl_data
  std::vector<std::string> multipl_entry_names;

  // Indexes in outer vector correspond to indexes in multipl_entry_names
  // Indexes in inner vector correspond to indexes in event_time_vec (ie. event number)
  std::vector<std::vector<unsigned int>> multipl_data;

  // Indexes correspond to event number
  std::vector<uint64_t> event_time_vec;
};


#endif
