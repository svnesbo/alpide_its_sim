/**
 * @file   PixelReadoutStats.hpp
 * @author Simon Voigt Nesbo
 * @date   August 13, 2018
 * @brief  Header file for PixelReadoutStats class. This class holds a map that stores counts of how
 *         many times a pixel hit was read out. The key in the map is how many times a pixel
 *         hit was read out, and value is for how many pixel hit this occured.
 *         Readout efficiency and pile up statistics can be calculated using this map.
 */


///@addtogroup event_generation
///@{
#ifndef PIXEL_READOUT_STATS_HPP
#define PIXEL_READOUT_STATS_HPP

#include <map>
#include <cstdint>
#include <fstream>
#include <iostream>

class PixelReadoutStats
{
private:
  ///@brief Readout stats for pixel hits.
  ///Key = number of times pixel hit was read out (or pile up value)
  ///Value = number of pixel hits that was read out this number of times
  ///e.g.
  /// mReadoutStats[0] == 100 --> 100 hits were never read out
  /// mReadoutStats[1] == 550 --> 550 hits were read out once
  /// mReadoutStats[2] == 300 --> 300 hits were read out twice
  ///
  /// The sum of values for mReadoutStats[1..N] equals the total number of hits that were read out
  std::map<unsigned int, std::uint64_t> mReadoutStats;

public:
  ///@brief Add readout count for a pixel hits.
  ///@param[in] count The number of times a particular pixel hit was read out
  inline void addReadoutCount(unsigned int count) {
    mReadoutStats[count]++;
  }

  ///@brief Get the number of pixel hits that were not read out
  ///@return Number of pixel hits that were not read out
  inline unsigned int getNotReadOutCount(void) {
    return mReadoutStats[0];
  }

  ///@brief Get the number of pixel hits that were actually read out
  ///@return Number of pixel hits that were read out
  inline unsigned int getReadOutCount(void) {
    unsigned int read_out_count = 0;

    for(auto stats_it = mReadoutStats.begin(); stats_it != mReadoutStats.end(); stats_it++) {
      if(stats_it->first != 0)
        read_out_count += stats_it->second;
    }
    return read_out_count;
  }

  inline void writeToFile(const std::string& filename) const {
    std::ofstream file(filename);

    if(file.is_open()) {
      std::cout << "Writing pixel readout stats to: \"" << filename << "\"" << std::endl;

      file << "Readout count/pileup:";

      for(auto stats_it = mReadoutStats.begin(); stats_it != mReadoutStats.end(); stats_it++) {
        file << ";" << stats_it->first;
      }

      file << std::endl << "Number of pixels:";

      for(auto stats_it = mReadoutStats.begin(); stats_it != mReadoutStats.end(); stats_it++) {
        file << ";" << stats_it->second;
      }
      file << std::endl;
      file.close();
    } else {
      std::cout << "Error opening file \"" << filename << "\"" << std::endl;
    }
  }
};


#endif
///@}
