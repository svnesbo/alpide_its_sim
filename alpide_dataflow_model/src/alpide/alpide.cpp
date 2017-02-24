/**
 * @file   alpide.h
 * @author Simon Voigt Nesbo
 * @date   December 12, 2016
 * @brief  Source file for Alpide class.
 */

#include "alpide.h"
#include <string>
#include <sstream>


SC_HAS_PROCESS(Alpide);
///@brief Constructor for Alpide.
///@param name    SystemC module name
///@param chip_id Desired chip id
Alpide::Alpide(sc_core::sc_module_name name, int chip_id, int region_fifo_size,
               bool enable_readout_traces, bool continuous_mode)
  : sc_core::sc_module(name)
  , PixelMatrix(continuous_mode)
{
  mChipId = chip_id;
  mEnableReadoutTraces = enable_readout_traces;

  s_event_buffers_used = 0;
  s_total_number_of_hits = 0;


  // Allocate/create/name SystemC FIFOs for the regions and connect the
  // Region Readout Units (RRU) FIFO outputs to Top Readout Unit (TRU) FIFO inputs
  s_region_fifos.reserve(N_REGIONS);
  for(int i = 0; i < N_REGIONS; i++) {
    std::stringstream ss << "region_" << i << "_fifo";    
    s_region_fifos.emplace_back(ss.str(), region_fifo_size);
                                
    mRRU[i].s_region_fifo_out(s_region_fifo.back());
    mTRU.s_region_fifo_in[i](s_region_fifo.back());
  }

  mTRU.s_clk_in(s_system_clk_in);
  
  SC_METHOD(matrixReadout);
  sensitive_pos << s_matrix_readout_clk_in;
}


///@brief Matrix readout SystemC method. This function is run one time per 40MHz clock cycle,
///       and will read out one pixel from each region (if there are pixels available in that region).
void Alpide::matrixReadout(void)
{
  uint64_t time_now = sc_time_stamp().value();
  
  if(mEnableReadoutTraces) {
    // Update signal with number of event buffers
    s_event_buffers_used = getNumEvents();

    // Update signal with total number of hits in all event buffers
    s_total_number_of_hits = getHitTotalAllEvents();
  }


  
  ///@todo Rewrite this... Iterate over RRU class objects, call the RRUs' readoutNextPixel(),
  ///      and let the RRUs read out the pixels from the pixel matrix.
  ///      This allows the readout of pixels to be controlled by the priority encoder clock,
  ///      but the readout from the FIFOs can be done from a process running at a higher clock rate
  
  // Read out a pixel from each region in the matrix
  for(int region_num = 0; region_num < N_REGIONS; region_num++) {
    mRRU[region_num].readoutNextPixel(*this, time_now);
  }
}


void Alpide::addTraces(sc_trace_file *wf) const
{
  std::stringstream ss;
  ss << "alpide_" << mChipId << "/event_buffers_used";
  std::string str_event_buffers_used(ss.str());

  ss.str("");
  ss << "alpide_" << mChipId << "/hits_in_matrix";
  std::string str_hits_in_matrix(ss.str());
  
  sc_trace(wf, s_event_buffers_used, str_event_buffers_used);
  sc_trace(wf, s_total_number_of_hits, str_hits_in_matrix);
}
