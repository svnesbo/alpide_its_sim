/**
 * @file   AlpideConfig.hpp
 * @author Simon Voigt Nesbo
 * @date   January 14, 2019
 * @brief  Data structure with common configuration settings for Alpide
 */

#ifndef ALPIDE_CONFIG_HPP
#define ALPIDE_CONFIG_HPP

#include <iostream>
#include <vector>

struct AlpideConfig {
  ///@brief Number of clock cycle delays associated with Data Transfer Unit (DTU)
  unsigned int dtu_delay_cycles;

  ///@brief Strobe length (in nanoseconds)
  unsigned int strobe_length_ns;

  ///@brief Minimum number of clock cycles the internal busy status must
  ///       be asserted before the chip reports BUSY_ON
  unsigned int min_busy_cycles;

  ///@brief Enable/disable strobe extension
  ///       (if new strobe received before the previous strobe interval ended)
  bool strobe_extension;

  ///@brief Enable clustering and use of DATA LONG words
  bool data_long_en;

  ///@brief Enable continuous mode (triggered mode if false)
  bool continuous_mode;

  ///@brief True for fast readout (2 clock cycles), false is slow (4 cycles).
  bool matrix_readout_speed;
};


#endif
