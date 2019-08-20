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
  std::map<unsigned int, std::map<unsigned int, std::uint64_t>> mReadoutStats;

public:
  ///@brief Add readout count for a pixel hits.
  ///@param[in] count The number of times a particular pixel hit was read out
  ///@param[in] chip_id The chip that this pixel belonged to
  inline void addReadoutCount(unsigned int count, unsigned int chip_id) {
    mReadoutStats[chip_id][count]++;
  }

  ///@brief Get the number of pixel hits that were not read out
  ///@param[in] chip_id Chip to get this count for
  ///@return Number of pixel hits that were not read out
  inline unsigned int getNotReadOutCount(unsigned int chip_id) {
    return mReadoutStats[chip_id][0];
  }

  ///@brief Get the number of pixel hits that were actually read out
  //////@param[in] chip_id Chip to get this count for
  ///@return Number of pixel hits that were read out
  inline unsigned int getReadOutCount(unsigned int chip_id) {
    unsigned int read_out_count = 0;

    for(auto stats_it = mReadoutStats[chip_id].begin(); stats_it != mReadoutStats[chip_id].end(); stats_it++) {
      if(stats_it->first != 0)
        read_out_count += stats_it->second;
    }
    return read_out_count;
  }

  ///@brief Create a CSV file with pixel readout stats versus chip ID
  inline void writeToFile(const std::string& filename) const {
    std::ofstream file(filename);

    if(file.is_open()) {
      unsigned int highest_readout_count = 0;

      std::cout << "Writing pixel readout stats to: \"" << filename << "\"" << std::endl;

      // The key in innermost map has the number of times a specific pixel was read out
      // We're looking for the highest number so we know how far the CSV header should go
      for(auto chip_it = mReadoutStats.begin(); chip_it != mReadoutStats.end(); chip_it++) {
        for(auto pixel_it = chip_it->second.begin(); pixel_it != chip_it->second.end(); pixel_it++) {
          if(pixel_it->first > highest_readout_count)
            highest_readout_count = pixel_it->first;
        }
      }

      // Write CSV header
      file << "Chip ID";
      for(unsigned int count = 0; count <= highest_readout_count; count++) {
        file << ";" << count;
      }


      // Write readout stats per chip
      for(auto chip_it = mReadoutStats.begin(); chip_it != mReadoutStats.end(); chip_it++) {
        file << std::endl;
        file << chip_it->first; // Write chip ID to CSV file

        unsigned int readout_count = 0;

        // Write readout counts for this chip. Assumes that the map is ordered
        for(auto pixel_it = chip_it->second.begin(); pixel_it != chip_it->second.end(); pixel_it++) {
          // Fill in zeros in CSV for readout counts that are not in the map
          while(pixel_it->first > readout_count) {
            file << ";" << 0;
            readout_count++;
          }
          file << ";" << pixel_it->second;
          readout_count++;
        }

        // Fill in zeros for the remaining readout counts that were not in the map
        while(readout_count <= highest_readout_count) {
          file << ";" << 0;
          readout_count++;
        }
      }
      file.close();
    } else {
      std::cout << "Error opening file \"" << filename << "\"" << std::endl;
    }
  }
};


#endif
///@}
