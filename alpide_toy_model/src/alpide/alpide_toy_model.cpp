/**
 * @file   alpide_toy_model.h
 * @author Simon Voigt Nesbo
 * @date   December 12, 2016
 * @brief  Source file for AlpideToyModel class.
 */

#include "alpide_toy_model.h"


SC_HAS_PROCESS(AlpideToyModel);
///@brief Constructor for AlpideToyModel.
///@param name    SystemC module name
///@param chip_id Desired chip id
AlpideToyModel::AlpideToyModel(sc_core::sc_module_name name, int chip_id)
  : sc_core::sc_module(name)
{
  mChipId = chip_id;

  SC_METHOD(matrixReadout);
  sensitive_pos << s_clk_in;
}


///@brief Matrix readout SystemC method. This function is run one time per 40MHz clock cycle,
///       and will read out one pixel from each region (if there are pixels available in that region).
void AlpideToyModel::matrixReadout(void)
{
  // Update signal with number of event buffers
  //s_event_buffers_used = getNumEvents();

  // Update signal with total number of hits in all event buffers
  //s_total_number_of_hits = getHitTotalAllEvents();
  
  // Read out a pixel from each region in the matrix
  for(int region_num = 0; region_num < N_REGIONS; region_num++) {
    readPixelRegion(region_num);
  }
}
