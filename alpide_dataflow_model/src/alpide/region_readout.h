/**
 * @file   region_readout.h
 * @Author Simon Voigt Nesbo
 * @date   February 20, 2017
 * @brief  Class for implementing the Region Readout Unit (RRU) in the Alpide chip.
 *
 */

#ifndef REGION_READOUT_H
#define REGION_READOUT_H

#include <systemc.h>
#include <cstdint>

/// The RegionReadoutUnit class is a simple representation of the RRU in the Alpide chip.
/// It has a member function that accepts pixel hits inputs, the RRU class will hold on to
/// these pixels to determine if there are several pixels in the same cluster, and then put
/// DATA_LONG or DATA_SHORT words into a SystemC FIFO.
class RegionReadoutUnit
{
public:
  // SystemC signals  
  sc_core::sc_fifo<AlpideDataWord> RRU_FIFO;  ///@todo Is this a SystemC input?? Fix!
  sc_out<bool> s_region_empty;
  sc_out<std::uint16_t> s_fifo_size;
  sc_out<bool> s_fifo_empty;
  sc_out<bool> s_busy_out;
  
private:
  /// The region handled by this RRU
  unsigned int mRegion;

  /// Corresponds to pixel address in DATA SHORT/LONG words, in priority encoder order
  std::uint8_t mPixelHitBaseAddr;

  /// Corresponds to hitmap in DATA LONG word
  std::uint8_t mPixelHitmap;

  unsigned int mFifoSizeLimit;

  bool mFifoSizeLimitEnabled;
  bool mBusySignaled;
  bool mClusteringEnabled;

  ///@brief Used in conjunction with mClusteringEnabled. Indicates that we have already
  ///       received the first pixel in a potential cluster (stored in mPixelHitBaseAddr),
  ///       and should continue building this cluster with subsequent hits that fall into
  ///       the same pixel cluster range.
  bool mClusterStarted;

public:
  RegionReadoutUnit(unsigned int region_num, unsigned int fifo_size);
  void readoutNextPixel(PixelMatrix& matrix);
};



#endif
