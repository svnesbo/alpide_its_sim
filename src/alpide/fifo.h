/**
 * @file   fifo.h
 * @Author Simon Voigt Nesbo
 * @date   November 27, 2016
 * @brief  Class for FIFO used in Alpide chip, based on the sc_fifo with added logic.
 *
 * Detailed description of file.
 */

#ifndef ALPIDE_FIFO_H
#define ALPIDE_FIFO_H


#include <systemc>
#include "data_format.h"

class AlpideFIFO : public sc_core::sc_module
{
public:
  sc_fifo <DataWordBase> data;
  sc_in_clk clock;
  sc_out<bool> fifo_empty;
  sc_out<bool> fifo_full;
  sc_out<bool> fifo_almost_full;
  sc_out<bool> fifo_busy;
  sc_in<bool> read_enable;
  sc_in<bool> write_enable;
  
private:
  unsigned int fifo_size;

public:
  AlpideFIFO(sc_core::sc_module_name name_in);
  int getFifoSize(void)
  void insertDataWord(DataWordBase data_word);
  DataWordBase getDataWord(void);
};


struct FifoSizes
{
public:
  int mTopRegionReadoutFifoSize;
  std::vector<int> mRegionReadoutFifoSize;
};


#endif
