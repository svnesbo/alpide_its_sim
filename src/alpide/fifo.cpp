/**
 * @file   fifo.cpp
 * @Author Simon Voigt Nesbo
 * @date   November 27, 2016
 * @brief  Implementation for FIFO class used in Alpide chip.
 *
 * Detailed description of file.
 */


#include "fifo.h"

AlpideFIFO::AlpideFIFO(sc_core::sc_module_name name_in) :
  sc_core::sc_module(name_in)
{
  fifo_size = 0;

  // Use some of this??
  // SC_METHOD(incr_count);
  // sensitive << reset;
  // sensitive << clock.pos();
  
}

int AlpideFIFO::getFifoSize(void)
{
    return fifo_size;
}

void AlpideFIFO::insertDataWord(DataWordBase data_word)
{

}

DataWordBase AlpideFIFO::getDataWord(void)
{
  
}
