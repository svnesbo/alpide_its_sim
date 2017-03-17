/**
 * @file   alpide_data_format.h
 * @author Simon Voigt Nesbo
 * @date   February 20, 2017
 * @brief  Definitions for data format used in Alpide chip.
 */

#ifndef ALPIDE_DATA_FORMAT_H
#define ALPIDE_DATA_FORMAT_H

#include <cstdint>
#include <ostream>

using std::uint8_t;

///@defgroup Alpide data format definitions
///@{
///
/// Alpide Data format and valid data words (from Alpide manual)
/// IDLE                 1111 1111
/// CHIP HEADER          1010<chip id[3:0]><BUNCH COUNTER FOR FRAME[10:3]>
/// CHIP TRAILER         1011<readout flags[3:0]>
/// CHIP EMPTY FRAME     1110<chip id[3:0]><BUNCH COUNTER FOR FRAME[10:3]>
/// REGION HEADER        110<region id[4:0]>
/// DATA SHORT           01<encoder id[3:0]><addr[9:0]>
/// DATA LONG            00<encoder id[3:0]><addr[9:0]> 0 <hit map[6:0]> 1111 0001 1111
/// BUSY ON              1111 0001
/// BUSY OFF             1111 0000


/// Alpide data words, used to initialize the 24-bit FIFOs in the Alpide chip.
/// The MSBs in the words identify datawords, the LSBs are parameters.
/// Note 1: There is not a fixed width for the MSB identifier part.
/// Note 2: Not to be confused with the definitions in AlpideDataTypes in alpide_data_parser.h,
///         which is for identifying individual bytes in a datastream.
const uint8_t DW_IDLE               = 0b11111111;
const uint8_t DW_CHIP_HEADER        = 0b10100000;
const uint8_t DW_CHIP_TRAILER       = 0b10110000;
const uint8_t DW_CHIP_EMPTY_FRAME   = 0b11100000;
const uint8_t DW_REGION_HEADER      = 0b11000000;
const uint8_t DW_DATA_SHORT         = 0b01000000;
const uint8_t DW_DATA_LONG          = 0b00000000;
const uint8_t DW_BUSY_ON            = 0b11110001;
const uint8_t DW_BUSY_OFF           = 0b11110000;

/// Mask for busy and idle words
const uint8_t MASK_IDLE_BUSY = 0b11111111;

/// Mask for chip header/trailer/empty frame words
const uint8_t MASK_CHIP = 0b11110000;

/// Mask for region header word
const uint8_t MASK_REGION_HEADER = 0b11100000;

/// Mask for data short/long words
const uint8_t MASK_DATA = 0b11000000;
///@}  


///@brief The FIFOs in the Alpide chip are 24 bits, or 3 bytes, wide. This is a base class for the
///       data words that holds 3 bytes, and is used as the data type in the SystemC FIFO templates.
///       This class shouldn't be used on its own, the various types of data words are implemented
///       in derived classes.
class AlpideDataWord
{
public:
  uint8_t data[3];

  bool signalBusyOn(void) {
    if(data[0] == DW_IDLE) {
      data[0] = DW_BUSY_ON;
      return true;
    } else if(data[1] == DW_IDLE) {
      data[1] = DW_BUSY_ON;
      return true;
    } else if(data[2] == DW_IDLE) {
      data[2] = DW_BUSY_ON;
      return true;
    } else {
      return false;
    }
  }
  bool signalBusyOff(void) {
    if(data[0] == DW_IDLE) {
      data[0] = DW_BUSY_OFF;
      return true;
    } else if(data[1] == DW_IDLE) {
      data[1] = DW_BUSY_OFF;
      return true;
    } else if(data[2] == DW_IDLE) {
      data[2] = DW_BUSY_OFF;
      return true;
    } else {
      return false;
    }
  }    
};

///@todo Overload this for all AlpideDataWord classes, so SystemC can print them to trace files properly?
static std::ostream& operator<< (std::ostream& stream, const AlpideDataWord& alpide_dw) {
  stream << "0x";
  stream << std::hex << alpide_dw.data[0];
  stream << std::hex << alpide_dw.data[1];
  stream << std::hex << alpide_dw.data[2];
  return stream;
}  


class AlpideIdle : public AlpideDataWord
{
public:
  AlpideIdle() {
    data[2] = DW_IDLE;
    data[1] = DW_IDLE;    
    data[0] = DW_IDLE;
  }
};


class AlpideChipHeader : public AlpideDataWord
{
public:
  AlpideChipHeader(uint8_t chip_id, uint16_t bunch_counter) {
    // Mask out bits 10:3 of the bunch counter
    uint16_t bc_masked = (bunch_counter & 0x7F8) >> 3;
    
    data[2] = DW_CHIP_HEADER | (chip_id & 0x0F);
    data[1] = bc_masked;
    data[0] = DW_IDLE;
  }
};


class AlpideChipTrailer : public AlpideDataWord
{
public:
  AlpideChipTrailer(uint8_t readout_flags) {
    data[2] = DW_CHIP_TRAILER | (readout_flags & 0x0F);
    data[1] = DW_IDLE;
    data[0] = DW_IDLE;
  }
};


class AlpideChipEmptyFrame : public AlpideDataWord
{
public:
  AlpideChipEmptyFrame(uint8_t chip_id, uint16_t bunch_counter) {
    // Mask out bits 10:3 of the bunch counter
    uint16_t bc_masked = (bunch_counter & 0x7F8) >> 3;
    
    data[2] = DW_CHIP_EMPTY_FRAME | (chip_id & 0x0F);
    data[1] = bc_masked;
    data[0] = DW_IDLE;
  }
};


class AlpideRegionHeader : public AlpideDataWord
{
public:
  AlpideRegionHeader(uint8_t region_id) {
    data[2] = DW_REGION_HEADER | (region_id & 0x1F);
    data[1] = DW_IDLE;
    data[0] = DW_IDLE;
  }
};


class AlpideDataShort : public AlpideDataWord
{
public:
  AlpideDataShort(uint8_t encoder_id, uint16_t addr) {
    data[2] = DW_DATA_SHORT | ((encoder_id & 0x0F) << 2) | ((addr >> 8) & 0x03);
    data[1] = addr & 0xFF;
    data[0] = DW_IDLE;
  }
};


class AlpideDataLong : public AlpideDataWord
{
public:
  AlpideDataLong(uint8_t encoder_id, uint16_t addr, uint8_t hitmap) {
    data[2] = DW_DATA_LONG | ((encoder_id & 0x0F) << 2) | ((addr >> 8) & 0x03);
    data[1] = addr & 0xFF;
    data[0] = hitmap & 0x7F;
  }
};


class AlpideBusyOn : public AlpideDataWord
{
public:
  AlpideBusyOn() {
    data[2] = DW_BUSY_ON;
    data[1] = DW_IDLE;
    data[0] = DW_IDLE;
  }
};


class AlpideBusyOff : public AlpideDataWord
{
public:
  AlpideBusyOff() {
    data[2] = DW_BUSY_OFF;
    data[1] = DW_IDLE;
    data[0] = DW_IDLE;
  }
};

#endif
