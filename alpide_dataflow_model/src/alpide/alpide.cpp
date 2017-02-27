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
               int tru_fifo_size, bool enable_readout_traces,
               bool enable_clustering, bool continuous_mode)
  : sc_core::sc_module(name)
  , PixelMatrix(continuous_mode)
  , s_top_readout_fifo(tru_fifo_size)
{
  mChipId = chip_id;
  mEnableReadoutTraces = enable_readout_traces;

  s_event_buffers_used = 0;
  s_total_number_of_hits = 0;
  s_oldest_event_number_of_hits = 0;

  mTRU = new TopReadoutUnit("TRU", chip_id);

  // Allocate/create/name SystemC FIFOs for the regions and connect the
  // Region Readout Units (RRU) FIFO outputs to Top Readout Unit (TRU) FIFO inputs
  //s_region_fifos.reserve(N_REGIONS);
  mRRUs.reserve(N_REGIONS);
  for(int i = 0; i < N_REGIONS; i++) {
    std::stringstream ss;
    ss << "RRU_" << i;
    mRRUs[i] = new RegionReadoutUnit(ss.str().c_str(), i, region_fifo_size, enable_clustering);

    //s_region_fifos[i] = new sc_fifo<AlpideDataWord>(region_fifo_size);
    
    // Conenct RRU->TRU FIFOs
//    mRRUs[i]->s_region_fifo_out(*s_region_fifos[i]);
//    mTRU->s_region_fifo_in[i](*s_region_fifos[i]);
    mTRU->s_region_fifo_in[i](mRRUs[i]->s_region_fifo);

    // Connect RRU->TRU region empty signals
    mTRU->s_region_empty_in[i](s_region_empty[i]);
    mRRUs[i]->s_region_empty_out(s_region_empty[i]);    
  }

  mTRU->s_clk_in(s_system_clk_in);
  mTRU->s_event_buffers_used_in(s_event_buffers_used);
  mTRU->s_current_event_hits_left_in(s_oldest_event_number_of_hits);
  mTRU->s_tru_fifo_out(s_top_readout_fifo);
  
  SC_METHOD(matrixReadout);
  sensitive_pos << s_matrix_readout_clk_in;

  SC_METHOD(dataTransmission);
  sensitive_pos << s_system_clk_in;
}


///@brief Matrix readout SystemC method. This method is clocked by the matrix readout clock.
///       The matrix readout period can be specified by the user in a register in the Alpide,
///       and is intended to allow the priority encoder a little more time to "settle" because
///       it is a relatively slow asynchronous circuit.
///       The method here triggers readout of a pixel from each region, into region buffers,
///       and updates some status signals related to regions/event-buffers.
void Alpide::matrixReadout(void)
{
  uint64_t time_now = sc_time_stamp().value();
  
  // Update signal with number of event buffers
  s_event_buffers_used = getNumEvents();

  // Update signal with total number of hits in all event buffers
  s_total_number_of_hits = getHitTotalAllEvents();

  s_oldest_event_number_of_hits = getHitsRemainingInOldestEvent();
  
  ///@todo Rewrite this... Iterate over RRU class objects, call the RRUs' readoutNextPixel(),
  ///      and let the RRUs read out the pixels from the pixel matrix.
  ///      This allows the readout of pixels to be controlled by the priority encoder clock,
  ///      but the readout from the FIFOs can be done from a process running at a higher clock rate
  
  // Read out a pixel from each region in the matrix
  for(int region_num = 0; region_num < N_REGIONS; region_num++) {
    mRRUs[region_num]->readoutNextPixel(*this, time_now);
  }
}


///@brief Data transmission SystemC method. Currently runs on 40MHz clock.
///@todo Implement more advanced data transmission method.
void Alpide::dataTransmission(void)
{
  AlpideDataWord dw;
  
  if(s_top_readout_fifo.nb_read(dw)) {
    sc_uint<24> data = dw.data[2] << 16 | dw.data[1] << 8 | dw.data[0];
    s_serial_data_output = data;
  }
}

///@brief Add SystemC signals to log in VCD trace file.
///@param wf Pointer to VCD trace file object
///@param name_prefix Name prefix to be added to all the trace names
void Alpide::addTraces(sc_trace_file *wf, std::string name_prefix) const
{
  std::stringstream ss;
  ss << name_prefix << "alpide_" << mChipId << "/";
  std::string alpide_name_prefix = ss.str();

  ss.str("");
  ss << alpide_name_prefix << "event_buffers_used";
  std::string str_event_buffers_used(ss.str());

  ss.str("");
  ss << alpide_name_prefix << "hits_in_matrix";
  std::string str_hits_in_matrix(ss.str());

  ss.str("");
  ss << alpide_name_prefix << "serial_data_output";
  std::string str_serial_data_output(ss.str());  
  
  sc_trace(wf, s_event_buffers_used, str_event_buffers_used);
  sc_trace(wf, s_total_number_of_hits, str_hits_in_matrix);
  sc_trace(wf, s_serial_data_output, str_serial_data_output);  

  mTRU->addTraces(wf, alpide_name_prefix);

  for(int i = 0; i < N_REGIONS; i++)
    mRRUs[i]->addTraces(wf, alpide_name_prefix);
}
