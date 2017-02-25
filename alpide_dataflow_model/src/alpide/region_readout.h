/**
 * @file   region_readout.h
 * @Author Simon Voigt Nesbo
 * @date   February 20, 2017
 * @brief  Class for implementing the Region Readout Unit (RRU) in the Alpide chip.
 *
 */

#ifndef REGION_READOUT_H
#define REGION_READOUT_H

#include "alpide_data_format.h"
#include "pixel_matrix.h"
#include <systemc.h>
#include <cstdint>

/// The RegionReadoutUnit class is a simple representation of the RRU in the Alpide chip.
/// It has a member function that accepts pixel hits inputs, the RRU class will hold on to
/// these pixels to determine if there are several pixels in the same cluster, and then put
/// DATA_LONG or DATA_SHORT words into a SystemC FIFO.
class RegionReadoutUnit : sc_core::sc_module
{
public:
  // SystemC signals  
  sc_port<sc_fifo_out_if<AlpideDataWord> > s_region_fifo_out;
  sc_out<bool> s_region_empty_out;
  sc_signal<bool> s_busy_out;
  
private:
  /// The region handled by this RRU
  unsigned int mRegionId;

  /// Corresponds to pixel address in DATA SHORT/LONG words, in priority encoder order
  std::uint16_t mPixelHitBaseAddr;

  /// Corresponds to priority encoder id in DATA SHORT/LONG words, which is the
  /// priority encoder id (within the current region) that the current pixel belongs to.
  std::uint8_t mPixelHitEncoderId;

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
  RegionReadoutUnit(sc_core::sc_module_name name, unsigned int region_num,
                    unsigned int fifo_size, bool cluster_enable);
  void readoutNextPixel(PixelMatrix& matrix, uint64_t time_now);
  void addTraces(sc_trace_file *wf, std::string name_prefix) const;  
};



#endif
