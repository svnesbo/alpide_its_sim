/**
 * @file   region_readout.h
 * @author Simon Voigt Nesbo
 * @date   February 20, 2017
 * @brief  Class for implementing the Region Readout Unit (RRU) in the Alpide chip.
 *
 */


///@defgroup region_readout Region Readout
///@ingroup alpide
///@{
#ifndef REGION_READOUT_H
#define REGION_READOUT_H

#include "alpide_data_format.hpp"
#include "pixel_matrix.hpp"
#include <cstdint>

// Ignore warnings about use of auto_ptr in SystemC library
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#include <systemc.h>
#pragma GCC diagnostic pop

// Ignore warnings about functions with unused variables in TLM library
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#include <tlm.h>
#pragma GCC diagnostic pop


namespace RO_FSM {
  enum{
    IDLE = 0,
    START_READOUT = 1,
    READOUT_AND_CLUSTERING = 2,
    REGION_TRAILER = 3
  };
}

namespace VALID_FSM {
  enum {
    IDLE = 0,
    EMPTY = 1,
    VALID = 2,
    POP = 3
  };
}

namespace HEADER_FSM {
  enum {
    HEADER = 0,
    DATA = 1,
  };
}


/// The RegionReadoutUnit class is a simple representation of the RRU in the Alpide chip.
/// It has a member function that accepts pixel hits inputs, the RRU class will hold on to
/// these pixels to determine if there are several pixels in the same cluster, and then put
/// DATA_LONG or DATA_SHORT words into a SystemC FIFO.
class RegionReadoutUnit : sc_core::sc_module
{
public:
  ///@brief 40MHz LHC clock
  sc_in_clk s_system_clk_in;

  /// This signal comes from FROMU, on deassertion of trigger, and indicates that start of
  /// readout from current pixel matrix event buffer to region FIFO can start
  sc_in<bool> s_frame_readout_start_in;

  sc_in<bool> s_readout_abort_in;

  /// This comes from TRU, when reaodut of next frame from region FIFO to TRU FIFO should start
  sc_in<bool> s_region_event_start_in;

  /// This comes from TRU, when reaodut of next frame from region FIFO to TRU FIFO should start
  sc_in<bool> s_region_event_pop_in;

  sc_in<bool> s_region_data_read_in;

  sc_out<bool> s_frame_readout_done_out;
  sc_out<bool> s_region_fifo_empty_out;
  sc_out<bool> s_region_valid_out;
  sc_out<AlpideDataWord> s_region_data_out;

private:
  sc_signal<sc_uint<8>> s_rru_readout_state;
  sc_signal<sc_uint<8>> s_rru_valid_state;
  sc_signal<sc_uint<1>> s_rru_header_state;
  sc_signal<bool> s_generate_region_header;

  /// Delayed one clock cycle compared to when it is used..
  sc_signal<bool> s_region_matrix_empty_debug;

  sc_signal<sc_uint<2> > s_matrix_readout_delay_counter;

  tlm::tlm_fifo<AlpideDataWord> s_region_fifo;
  sc_signal<sc_uint<8> > s_region_fifo_size;

  AlpideRegionHeader mRegionHeader;

private:
  /// The region handled by this RRU
  unsigned int mRegionId;

  /// Corresponds to Matrix Readout Speed bit in 0x0001 Mode Control register in Alpide chip.
  /// True: 20MHz readout. False: 10MHz readout.
  bool mMatrixReadoutSpeed;

  /// Used with mMatrixReadoutSpeed to implement a delay when readout out pixel matrix.
  bool mMatrixReadoutCounter;

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

  PixelMatrix* mPixelMatrix;

private:
  bool readoutNextPixel(PixelMatrix& matrix);
  void flushRegionFifo(void);

public:
  RegionReadoutUnit(sc_core::sc_module_name name, PixelMatrix* matrix,
                    unsigned int region_num, unsigned int fifo_size,
                    bool matrix_readout_speed, bool cluster_enable);
  void regionReadoutProcess(void);
  void regionMatrixReadoutFSM(void);
  void regionValidFSM(void);
  void regionHeaderFSM(void);
  void addTraces(sc_trace_file *wf, std::string name_prefix) const;
};


#endif
///@}
