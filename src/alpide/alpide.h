/**
 * @file   alpide.h
 * @Author Simon Voigt Nesbo
 * @date   November 27, 2016
 * @brief  Classes for alpide chip implementation
 *
 * Detailed description of file.
 */

#ifndef ALPIDE_H
#define ALPIDE_H

#include <queue>
#include <set>
#include "alpide_constants.h"

// See figures 2.1 and 2.2 in alpide operations manual
// RRU - Region Readout Unit
// TRU - Top Region Unit
// DMU - Data Management Unit
// DTU - Data Transfer Unit

// RRU - 128x24b DPRAM, 24b x 40MHz data rate
// TRU - 24b x 40MHz in, 24b x 40MHz out

// DColumn --+--- Region --- RRU ---+---- TRU (32:1 MUX) ---- DMU ---- DTU
//           |                      |
// DColumn --+                      |
//           |                      |
// DColumn --+                      |
//   ...     |                      |
// DColumn --+                      |
//                                  |
//                                  |
//                                  |
// DColumn --+--- Region --- RRU ---+ 
//           |                      |
// DColumn --+                      |
//           |                      |
// DColumn --+                      |
//   ...     |                      |
// DColumn --+                      |
//                                  |
// ...                              |
//                                  |
// DColumn --+--- Region --- RRU ---+
//           |
// DColumn --+
//           |
// DColumn --+
//   ...     |
// DColumn --+ 


// Data transfer:
// Region header is only sent for regions with data
// 


class Alpide
{
public: // SystemC signals  
  sc_fifo <DataByte> s_serial_data_out;
  sc_fifo <DataByte> s_parallel_data_out;
  sc_in<bool> s_trigger_in;
  sc_in_clk s_clk_in;
    
private:
  TopReadoutUnit mRRU;

  //@brief DMU will strip away unnecessary IDLE words from
  //       data transmission on parallel bus, but not serial bus.
  DataManagementUnit mDMU;
  
  std::queue<hit_data> mOutputFifo;
  PixelMatrix mMatrix;
  int mChipId;

  //@brief Toggle between serial (1.2Gbps) and parallel (0.4Gbps) bus
  bool mParallelBusEnable = false;

  //@brief Toggle between master and slave chip. Should only be used when
  //       parallel bus is enabled (middle/outer barrels).
  bool mMasterChipEnable = true;
  
public:
  Alpide(int chip_id) {mChipId = chip_id;}
  setPixel(unsigned int col_num, unsigned int row_num) {
    mMatrix.setPixel(col_num, row_num);
  }
  int getChipId(void) {return mChipId;}
  FifoSizes* getFifoSizes(void);
};



#endif
