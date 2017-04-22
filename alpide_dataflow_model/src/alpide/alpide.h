/**
 * @file   alpide.h
 * @author Simon Voigt Nesbo
 * @date   December 11, 2016
 * @brief  Header file for Alpide class.
 */

#ifndef ALPIDE_H
#define ALPIDE_H


#include "alpide_data_format.h"
#include "pixel_matrix.h"
#include "region_readout.h"
#include "top_readout.h"

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
class Alpide : sc_core::sc_module, public PixelMatrix
{
public:
  ///@defgroup SystemC ports
  ///@{
  
  ///@brief 40MHz LHC clock
  sc_in_clk s_system_clk_in;
  sc_in<bool> s_strobe_n_in;

  ///@brief Indicates that the chip is ready to accept hits and setPixel() can be called.
  sc_out<bool> s_chip_ready_out;
  
  sc_out<sc_uint<24>> s_serial_data_output;
  ///@}
  
  ///@todo Should these signals be private maybe?
  ///@defgroup SystemC signals
  ///@{

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
  
  
  sc_fifo<AlpideDataWord> s_dmu_fifo;
  
  sc_signal<sc_uint<8> > s_dmu_fifo_size;
  sc_signal<bool> s_chip_ready_internal;
  ///@}

private:
  tlm::tlm_fifo<FrameStartFifoWord> s_frame_start_fifo;
  tlm::tlm_fifo<FrameEndFifoWord> s_frame_end_fifo;
  //sc_fifo<FrameStartFifoWord> s_frame_start_fifo;
  //sc_fifo<FrameEndFifoWord> s_frame_end_fifo;

  FrameEndFifoWord mNextFrameEndWord;

  enum FROMU_readout_state_t {
    WAIT_FOR_EVENTS = 0,
    REGION_READOUT_START = 1,
    WAIT_FOR_REGION_READOUT = 2,
    REGION_READOUT_DONE = 3
  };

  sc_signal<sc_uint<8>> s_fromu_readout_state;  
    
private:
  int mChipId;
  bool mEnableReadoutTraces;
  bool mStrobeActive;
  uint16_t mBunchCounter;
  
  ///@brief Number of (trigger) events that are accepted into an MEB by the chip
  uint64_t mTriggerEventsAccepted = 0;

  ///@brief Triggered mode: If 3 MEBs are already full, the chip will not accept more events
  ///                       until one of those 3 MEBs have been read out. This variable is counted
  ///                       up for each event that is not accepted.
  uint64_t mTriggerEventsRejected = 0;

  ///@brief Continuous mode only.
  ///       The Alpide chip will try to guarantee that there is a free MEB slice in continuous mode.
  ///       It does this by deleting the oldest MEB slice (even if it has not been read out) when
  ///       the 3rd one is filled. This variable counts up in that case.
  uint64_t mTriggerEventsFlushed = 0;  

  std::vector<RegionReadoutUnit*> mRRUs;
  TopReadoutUnit* mTRU;


  void mainProcess(void);  
  void strobeInput(void);
  void frameReadout(void); // FROMU    
  void dataTransmission(void);
  bool getFrameReadoutDone(void);
  void updateBusyStatus(void);

public:
  Alpide(sc_core::sc_module_name name, int chip_id, int region_fifo_size,
         int dmu_fifo_size, bool enable_clustering, bool continuous_mode,
         bool matrix_readout_speed);
  int getChipId(void) {return mChipId;}
  void addTraces(sc_trace_file *wf, std::string name_prefix) const;
  uint64_t getTriggerEventsAcceptedCount(void) const {return mTriggerEventsAccepted;}
  uint64_t getTriggerEventsRejectedCount(void) const {return mTriggerEventsRejected;}  
};


#endif
