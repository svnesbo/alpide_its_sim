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

#include "AlpideConfig.hpp"
#include "AlpideDataWord.hpp"
#include "AlpideInterface.hpp"
#include "PixelMatrix.hpp"
#include "PixelFrontEnd.hpp"
#include "RegionReadoutUnit.hpp"
#include "TopReadoutUnit.hpp"

// Ignore warnings about use of auto_ptr and unused parameters in SystemC library
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
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

  ///@brief Data output socket. Not used for OB slave chips (can be left unbound)
  DataInitiatorSocket s_data_output;

  ///@brief Obsolete: don't use.
  /// Used to indicate that the chip is ready to accept hits and
  /// setPixel() could be called.
  sc_export<sc_signal<bool>> s_chip_ready_out;

  ///@brief Serial data output. This is an alternative
  ///       representation of the data on s_data_output.
  sc_export<sc_signal<sc_uint<24>>> s_serial_data_out_exp;

  ///@brief Trigger ID for data that is currently being sent out.
  sc_export<sc_signal<uint64_t>> s_serial_data_trig_id_exp;

  ///@brief Fifo interfaces to slave chips' DMU FIFOs, in OB mode
  ///       Used instead of the parallel interface in the real chips.
  std::vector<sc_port<sc_fifo_in_if<AlpideDataWord>>> s_local_bus_data_in;

  ///@brief DMU FIFO output is available on this port.
  ///       Only used by master chip in OB configuration
  sc_port<sc_fifo_in_if<AlpideDataWord>> s_local_bus_data_out;

  ///@brief Busy line inputs from slave chips in OB mode.
  ///       Used instead of the BUSY line with pullup in the real chips.
  std::vector<sc_in<bool>> s_local_busy_in;

  ///@brief Busy line output from slave chips in OB mode
  sc_export<sc_signal<bool>> s_local_busy_out;

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
  sc_signal<uint64_t>    s_serial_data_trig_id;

  ///@brief FIFO used to represent the encoding delay in the DTU
  sc_fifo<sc_uint<24>> s_dtu_delay_fifo;

  ///@brief FIFO used to delay trigger output signal s_serial_data_trig_id_exp
  ///       by as many cycles as the data is delayed
  sc_fifo<uint64_t> s_dtu_delay_fifo_trig;

  ///@brief Represents the FIFO written to by the BMU in the real ALPIDE chip
  sc_fifo<AlpideDataWord> s_busy_fifo;


  sc_signal<sc_uint<8> > s_dmu_fifo_size;
  sc_signal<sc_uint<8> > s_busy_fifo_size;
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

  ///@brief True: Continuous, False: Triggered
  bool mContinuousMode;

  bool mEnableReadoutTraces;
  bool mEnableDtuDelay;
  bool mStrobeActive;
  bool mStrobeExtensionEnable;
  bool mStrobeExtended = false;
  uint16_t mBunchCounter;
  uint16_t mStrobeLengthNs;
  uint64_t mStrobeStartTime;
  uint16_t mMinBusyCycles;
  uint16_t mBusyCycleCount = 0;

  bool mObMode;
  bool mObMaster;

  ///@brief Number of slave chips connected to outer barrel master
  const unsigned int mObSlaveCount;

  ///@brief Chip select on "local bus" in outer barrel mode
  unsigned int mObChipSel = 0;

  ///@brief Next chip on "local bus" in outer barrel mode
  unsigned int mObNextChipSel = 0;

  ///@brief Bytes remaining in transmission of an up to 24-bit data word in OB mode
  unsigned int mObDwBytesRemaining = 0;

  ///@brief Index of byte to transmit in current 24-bit data word
  unsigned int mObDwByteIndex = 0;

  ///@brief Holds 24-bit data word to be transmitted over 3 clock cycles,
  ///       in outer barrel mode.
  AlpideDataWord mObDataWord;

  ///@brief Trigger ID counter
  uint64_t mTrigIdCount = 0;

  ///@brief Trigger ID for the currently active strobe
  uint64_t mTrigIdForStrobe = 0;

  ///@brief Numbers of triggers received by the chip
  uint64_t mTriggersReceived = 0;

  ///@brief Number of triggers that are accepted by the chip
  uint64_t mTriggersAccepted = 0;

  ///@brief Number of triggers that were "rejected" (ie. all 3 MEBs were full)
  uint64_t mTriggersRejected = 0;

  ///@brief Number of "positive" busy transitions (ie. chip went into busy state)
  uint64_t mBusyTransitions = 0;

  ///@brief Number of busy violations.
  ///@todo This counter is currently only increased by the strobeInput() function,
  ///      for every single strobe/trigger where there are no free MEBs.
  ///      Since there are other mechanisms for busy (such as frame fifo), and
  ///      the chip can enter data overrun mode where it sends out empty packages
  ///      and discards the data, this busy violation count here may not match
  ///      the number of busy violations the Readout Unit actually sees.
  uint64_t mBusyViolations = 0;

  uint64_t mFlushedIncompleteCount = 0;

  uint64_t mEventIdCount = 0;

  void newEvent(uint64_t event_time);
  void mainMethod(void);
  void triggerMethod(void);
  void strobeDurationMethod(void);
  void busyFifoMethod(void);

  void strobeInput(void);
  void frameReadout(void); // FROMU
  void dataTransmission(void);
  void updateBusyStatus(void);
  bool getFrameReadoutDone(void);
  ControlResponsePayload processCommand(ControlRequestPayload const &request);

public:
  Alpide(sc_core::sc_module_name name, const int chip_id, const AlpideConfig& chip_cfg,
         bool outer_barrel_mode = false, bool outer_barrel_master = false,
         int outer_barrel_slave_count = 0);
  int getChipId(void) {return mChipId;}
  void addTraces(sc_trace_file *wf, std::string name_prefix) const;

  uint64_t getTriggersReceivedCount(void) const {return mTriggersReceived;}
  uint64_t getTriggersAcceptedCount(void) const {return mTriggersAccepted;}
  uint64_t getTriggersRejectedCount(void) const {return mTriggersRejected;}
  uint64_t getBusyCount(void) const {return mBusyTransitions;}
  uint64_t getBusyViolationCount(void) const {return mBusyViolations;}
  uint64_t getFlushedIncompleteCount(void) const {return mFlushedIncompleteCount;}
};


#endif
///@}
