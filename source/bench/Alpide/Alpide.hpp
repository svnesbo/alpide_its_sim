/**
 * @file   Alpide.hpp
 * @author Simon Voigt Nesbo
 * @date   December 11, 2016
 * @brief  Header file for Alpide class.
 */


///@defgroup alpide Alpide SystemC Model
///@{
#ifndef ALPIDE_H
#define ALPIDE_H

#include "AlpideDataWord.hpp"
#include "AlpideInterface.hpp"
#include "PixelMatrix.hpp"
#include "PixelFrontEnd.hpp"
#include "RegionReadoutUnit.hpp"
#include "TopReadoutUnit.hpp"

// Ignore warnings about use of auto_ptr in SystemC library
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#include <systemc.h>
#pragma GCC diagnostic pop

#include <vector>
#include <list>
#include <string>


/// Alpide main class. Currently it only implements the MEBs,
/// no RRU FIFOs, and no TRU FIFO. It will be used to run some initial
/// estimations for probability of MEB overflow (busy).
class Alpide : sc_core::sc_module, public PixelMatrix, public PixelFrontEnd
{
public:
  ///@brief 40MHz LHC clock
  sc_in_clk s_system_clk_in;

  ControlTargetSocket s_control_input;
  DataInitiatorSocket s_data_output;

  ///@brief Indicates that the chip is ready to accept hits and setPixel() can be called.
  //sc_out<bool> s_chip_ready_out;
  sc_export<sc_signal<bool>> s_chip_ready_out;

  sc_export<sc_signal<sc_uint<24>>> s_serial_data_out_exp;

private:
  sc_signal<sc_uint<8>> s_fromu_readout_state;

  ///@brief Number of events stored in the chip at any given time
  sc_signal<sc_uint<8>> s_event_buffers_used_debug;

  sc_signal<sc_uint<8>> s_frame_start_fifo_size_debug;
  sc_signal<sc_uint<8>> s_frame_end_fifo_size_debug;

  ///@brief Sum of all hits in all multi event buffers
  sc_signal<sc_uint<32>> s_total_number_of_hits;

  ///@brief Number of hits in oldest multi event buffer
  sc_signal<sc_uint<32> > s_oldest_event_number_of_hits;

  sc_signal<bool> s_region_fifo_empty[N_REGIONS];
  sc_signal<bool> s_region_valid[N_REGIONS];
  sc_signal<bool> s_region_data_read[N_REGIONS];
  sc_signal<bool> s_region_event_start;
  sc_signal<bool> s_region_event_pop;
  sc_signal<AlpideDataWord> s_region_data[N_REGIONS];

  ///@brief Frame Readout Managment Unit (FROMU) signals
  sc_signal<bool> s_frame_readout_start;
  sc_signal<bool> s_frame_readout_done[N_REGIONS];
  sc_signal<bool> s_frame_readout_done_all;
  sc_signal<bool> s_frame_fifo_busy;
  sc_signal<bool> s_multi_event_buffers_busy;
  sc_signal<bool> s_fatal_state;
  sc_signal<bool> s_readout_abort;
  sc_signal<bool> s_flushed_incomplete;
  sc_signal<bool> s_busy_violation;
  sc_signal<bool> s_busy_status;


  /// Data is transferred in the following order:
  /// TRU --> s_dmu_fifo --+---> s_dtu_delay_fifo --> s_serial_data_output
  ///                      |
  ///                      +---> s_serial_data_dtu_input_debug
  sc_fifo<AlpideDataWord> s_dmu_fifo;

  sc_signal<sc_uint<24>> s_serial_data_dtu_input_debug;
  sc_signal<sc_uint<24>> s_serial_data_out;

  ///@brief FIFO used to represent the encoding delay in the DTU
  sc_fifo<AlpideDataWord> s_dtu_delay_fifo;


  sc_signal<sc_uint<8> > s_dmu_fifo_size;
  sc_signal<bool> s_chip_ready_internal;
  sc_signal<bool> s_strobe_n;

  sc_event E_trigger;
  sc_event E_strobe_interval_done;

  tlm::tlm_fifo<FrameStartFifoWord> s_frame_start_fifo;
  tlm::tlm_fifo<FrameEndFifoWord> s_frame_end_fifo;

  std::vector<RegionReadoutUnit*> mRRUs;
  TopReadoutUnit* mTRU;

  FrameEndFifoWord mNextFrameEndWord;

  enum FROMU_readout_state_t {
    WAIT_FOR_EVENTS = 0,
    REGION_READOUT_START = 1,
    WAIT_FOR_REGION_READOUT = 2,
    REGION_READOUT_DONE = 3
  };

private:
  int mChipId;
  bool mEnableReadoutTraces;
  bool mEnableDtuDelay;
  bool mStrobeActive;
  bool mStrobeExtensionEnable;
  uint16_t mBunchCounter;
  uint16_t mStrobeLengthNs;
  uint64_t mStrobeStartTime;

  ///@brief Number of triggers that are accepted by the chip
  uint64_t mTriggersAccepted = 0;

  ///@brief Number of triggers that were "rejected" (ie. all 3 MEBs were full)
  uint64_t mTriggersRejected = 0;

  ///@brief Number of "positive" busy transitions (ie. chip went into busy state)
  uint64_t mBusyTransitions = 0;

  uint64_t mBusyViolations = 0;
  uint64_t mFlushedIncompleteCount = 0;

  uint64_t mEventIdCount = 0;

  void newEvent(uint64_t event_time);
  void mainMethod(void);
  void triggerMethod(void);
  void strobeDurationMethod(void);
  void strobeInput(void);

  void frameReadout(void); // FROMU
  void dataTransmission(void);
  void updateBusyStatus(void);
  bool getFrameReadoutDone(void);
  ControlResponsePayload processCommand(ControlRequestPayload const &request);

public:
  Alpide(sc_core::sc_module_name name, int chip_id, int region_fifo_size,
         int dmu_fifo_size, int dtu_delay_cycles, int strobe_length_ns,
         bool strobe_extension, bool enable_clustering, bool continuous_mode,
         bool matrix_readout_speed);
  int getChipId(void) {return mChipId;}
  void addTraces(sc_trace_file *wf, std::string name_prefix) const;
  uint64_t getTriggersAcceptedCount(void) const {return mTriggersAccepted;}
  uint64_t getEventFramesRejectedCount(void) const {return mTriggersRejected;}
};


#endif
///@}
