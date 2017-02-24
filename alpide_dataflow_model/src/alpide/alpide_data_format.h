/**
 * @file   alpide_data_format.h
 * @author Simon Voigt Nesbo
 * @date   February 20, 2017
 * @brief  Definitions for data format used in Alpide chip
 */

#ifndef ALPIDE_DATA_FORMAT_H
#define ALPIDE_DATA_FORMAT_H

#include <cstdint>
#include <ostream>

using std::uint8_t;

/// Alpide data words. The MSBs identify datawords, the LSBs are parameters.
/// Note: there is not a fixed width for the MSB identifier part.
const uint8_t DW_IDLE = 0b11111111;
const uint8_t DW_CHIP_HEADER = 0b10100000;
const uint8_t DW_CHIP_TRAILER = 0b10110000;
const uint8_t DW_CHIP_EMPTY_FRAME = 0b11100000;
const uint8_t DW_REGION_HEADER = 0b11000000;
const uint8_t DW_DATA_SHORT = 0b01000000;
const uint8_t DW_DATA_LONG = 0b00000000;
const uint8_t DW_BUSY_ON = 0b11110001;
const uint8_t DW_BUSY_OFF = 0b11110000;


class AlpideDataWord
{
public:
  uint8_t data[3];  
};

///@todo Overload this for all AlpideDataWord classes, so SystemC can print them to trace files properly?
std::ostream& operator<< (std::ostream& stream, const AlpideDataWord& alpide_dw) {
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
    data[0] = DW_IDLE;
    data[1] = DW_IDLE;
    data[2] = DW_IDLE;    
  }
};


class AlpideChipHeader : public AlpideDataWord
{
public:
  AlpideChipHeader(uint8_t chip_id, uint16_t bunch_counter) {
    // Mask out bits 10:3 of the bunch counter
    uint16_t bc_masked = (bunch_counter & 0x7F8) >> 3;
    
    data[0] = DW_CHIP_HEADER | (chip_id & 0x0F);
    data[1] = bc_masked;
    data[2] = DW_IDLE;
  }
};


class AlpideChipTrailer : public AlpideDataWord
{
public:
  AlpideChipTrailer(uint8_t readout_flags) {
    data[0] = DW_CHIP_TRAILER | (readout_flags & 0x0F);
    data[1] = DW_IDLE;
    data[2] = DW_IDLE;
  }
};


class AlpideChipEmptyFrame : public AlpideDataWord
{
public:
  AlpideChipEmptyFrame(uint8_t chip_id, uint16_t bunch_counter) {
    // Mask out bits 10:3 of the bunch counter
    uint16_t bc_masked = (bunch_counter & 0x7F8) >> 3;
    
    data[0] = DW_CHIP_EMPTY_FRAME | (chip_id & 0x0F);
    data[1] = bc_masked;
    data[2] = DW_IDLE;
  }
};


class AlpideRegionHeader : public AlpideDataWord
{
public:
  AlpideRegionHeader(uint8_t region_id) {
    data[0] = DW_REGION_HEADER | (region_id & 0x1F);
    data[1] = DW_IDLE;
    data[2] = DW_IDLE;
  }
};


class AlpideDataShort : public AlpideDataWord
{
public:
  AlpideDataShort(uint8_t encoder_id, uint16_t addr) {
    data[0] = DW_DATA_SHORT | ((encoder_id & 0x0F) << 4) | ((addr >> 8) & 0x03);
    data[1] = addr & 0xFF;
    data[2] = DW_IDLE;
  }
};


class AlpideDataLong : public AlpideDataWord
{
public:
  AlpideDataLong(uint8_t encoder_id, uint16_t addr, uint8_t hitmap) {
    data[0] = DW_DATA_LONG | ((encoder_id & 0x0F) << 4) | ((addr >> 8) & 0x03);
    data[1] = addr & 0xFF;
    data[2] = hitmap & 0x7F;
  }
};


class AlpideBusyOn : public AlpideDataWord
{
public:
  AlpideBusyOn() {
    data[0] = DW_BUSY_ON;
    data[1] = DW_IDLE;
    data[2] = DW_IDLE;
  }
};


class AlpideBusyOff : public AlpideDataWord
{
public:
  AlpideBusyOff() {
    data[0] = DW_BUSY_OFF;
    data[1] = DW_IDLE;
    data[2] = DW_IDLE;
  }
};

#endif
