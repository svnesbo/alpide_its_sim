/**
 * @file   alpide_toy_model.h
 * @Author Simon Voigt Nesbo
 * @date   December 12, 2016
 * @brief  "Toy model" for the ALPIDE chip. It only implements the MEBs, 
 *          no RRU FIFOs, and no TRU FIFO. It will be used to run some initial
 *          estimations for probability of MEB overflow (busy).
 *
 * Detailed description of file.
 */

#include "alpide_toy_model.h"


//@brief Constructor for AlpideToyModel.
//@param name    SystemC module name
//@param chip_id Desired chip id
SC_HAS_PROCESS(AlpideToyModel);
AlpideToyModel::AlpideToyModel(sc_core::sc_module_name name, int chip_id)
  : sc_core::sc_module(name)
{
  mChipId = chip_id;


  //s_event_buffer_num.initialize(0);
  //s_total_number_of_hits.initialize(0);
  
  // Any sensitive commands after an SC_METHOD() specifies signals that this method
  // will be sensitive to. The sensitive statements will only apply to the most recent
  // SC_METHOD() call.

  // Use method or thread?
  SC_METHOD(matrixReadout);
  sensitive_pos << s_clk_in;
}


//@brief Matrix readout SystemC method. This function is run one time per 40MHz clock cycle,
//       and will read out one pixel from each region (if there are pixels available in that region).
void AlpideToyModel::matrixReadout(void)
{
  // Read out a pixel from each region in the matrix
  for(int region_num = 0; region_num < N_REGIONS; region_num++) {
    readPixelRegion(region_num);
  }

  // Update signal with number of event buffers
  s_event_buffers_used = getNumEvents();

  // Update signal with total number of hits in all event buffers
  s_total_number_of_hits = getHitTotalAllEvents();
}
