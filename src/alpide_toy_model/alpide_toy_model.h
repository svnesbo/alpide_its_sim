/**
 * @file   alpide_toy_model.h
 * @Author Simon Voigt Nesbo
 * @date   December 11, 2016
 * @brief  "Toy model" for the ALPIDE chip. It only implements the MEBs, 
 *          no RRU FIFOs, and no TRU FIFO. It will be used to run some initial
 *          estimations for probability of MEB overflow (busy).
 *
 * Detailed description of file.
 */

#ifndef ALPIDE_TOY_MODEL_H
#define ALPIDE_TOY_MODEL_H


class AlpideToyModel : public AlpideBase
{
// public: // SystemC signals  
//   sc_fifo <DataByte> s_serial_data_out;
//   sc_fifo <DataByte> s_parallel_data_out;
//   sc_in<bool> s_trigger_in;
//   sc_in_clk s_clk_in;
    
// private:
//   std::queue<hit_data> mOutputFifo;
//   PixelMatrix mMatrix;
//   int mChipId;

//   //@brief Toggle between serial (1.2Gbps) and parallel (0.4Gbps) bus
//   bool mParallelBusEnable = false;

//   //@brief Toggle between master and slave chip. Should only be used when
//   //       parallel bus is enabled (middle/outer barrels).
//   bool mMasterChipEnable = true;
  
// public:
//   Alpide(int chip_id) {mChipId = chip_id;}
//   setPixel(unsigned int col_num, unsigned int row_num) {
//     mMatrix.setPixel(col_num, row_num);
//   }
//   int getChipId(void) {return mChipId;}
//   FifoSizes* getFifoSizes(void);
};


#endif
