/**
 * @file   alpide_data_format.h
 * @author Simon Voigt Nesbo
 * @date   February 20, 2017
 * @brief  Definitions for data format used in Alpide chip.
 */


///@defgroup data_format Alpide Data Format
///@ingroup alpide
///@{
#ifndef ALPIDE_DATA_FORMAT_H
#define ALPIDE_DATA_FORMAT_H

#include <cstdint>
#include <ostream>

// Ignore warnings about use of auto_ptr in SystemC library
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#include <systemc.h>
#pragma GCC diagnostic pop


using std::uint8_t;


/// Enumerations used to easily identify the type of data word stored
/// in the AlpideDataWord objects, to avoid having to parse it.
///
/// Not to be confused with the DW_<data word type> constants below, which
/// are the actual data words used on the serial link. These DW_ words are
/// mixed with data bits making it hard to parse them.
///
/// Note that the region trailer word should never appear in the output data
/// stream, they are only used internally in the ALPIDE chip.
enum AlpideDataType {ALPIDE_IDLE,
                     ALPIDE_CHIP_HEADER1,
                     ALPIDE_CHIP_HEADER2,
                     ALPIDE_CHIP_TRAILER,
                     ALPIDE_CHIP_EMPTY_FRAME1,
                     ALPIDE_CHIP_EMPTY_FRAME2,
                     ALPIDE_REGION_HEADER,
                     ALPIDE_REGION_TRAILER,
                     ALPIDE_DATA_SHORT1,
                     ALPIDE_DATA_SHORT2,
                     ALPIDE_DATA_LONG1,
                     ALPIDE_DATA_LONG2,
                     ALPIDE_DATA_LONG3,
                     ALPIDE_BUSY_ON,
                     ALPIDE_BUSY_OFF,
                     ALPIDE_COMMA,
                     ALPIDE_UNKNOWN};


/// Alpide Data format and valid data words (from Alpide manual)
/// Data word             | Header bits | Parameter bits
/// --------------------- | ----------- | --------------
/// IDLE                  | 1111 1111   | None
/// CHIP HEADER           | 1010        | <chip id[3:0]><BUNCH COUNTER FOR FRAME[10:3]>
/// CHIP TRAILER          | 1011        | <readout flags[3:0]>
/// CHIP EMPTY FRAME      | 1110        | <chip id[3:0]><BUNCH COUNTER FOR FRAME[10:3]>
/// REGION HEADER         | 110         | <region id[4:0]>
/// REGION_TRAILER        | 1111 0011   | None
/// DATA SHORT            | 01          | <encoder id[3:0]><addr[9:0]>
/// DATA LONG             | 00          | <encoder id[3:0]><addr[9:0]> 0 <hit map[6:0]> 1111 0001 1111
/// BUSY ON               | 1111 0001   | None
/// BUSY OFF              | 1111 0000   | None
/// COMMA                 | 1011 1100   | Note: 1111 1110 used instead in this code


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
const uint8_t DW_REGION_TRAILER     = 0b11110011;
const uint8_t DW_DATA_SHORT         = 0b01000000;
const uint8_t DW_DATA_LONG          = 0b00000000;
const uint8_t DW_BUSY_ON            = 0b11110001;
const uint8_t DW_BUSY_OFF           = 0b11110000;

/// This is not the correct COMMA word, but using this value instead makes this
/// simulation model a bit simpler because the word cannot be confused with CHIP TRAILER
/// The normal (correct) comma word is 0xBC = 0b10111100
const uint8_t DW_COMMA              = 0b11111110;

const uint8_t READOUT_FLAGS_BUSY_VIOLATION     = 0b00001000;
const uint8_t READOUT_FLAGS_FLUSHED_INCOMPLETE = 0b00000100;
const uint8_t READOUT_FLAGS_STROBE_EXTENDED    = 0b00000010;
const uint8_t READOUT_FLAGS_BUSY_TRANSITION    = 0b00000001;

/// Mask for busy, idle and comma words
const uint8_t MASK_IDLE_BUSY_COMMA = 0b11111111;

/// Mask for chip header/trailer/empty frame words
const uint8_t MASK_CHIP = 0b11110000;

/// Mask for region header word
const uint8_t MASK_REGION_HEADER = 0b11100000;

/// Mask for data short/long words
const uint8_t MASK_DATA = 0b11000000;


/// Data word stored in FRAME START FIFO
struct FrameStartFifoWord {
  bool busy_violation;
  uint16_t BC_for_frame; // Bunch counter

  inline bool operator==(const FrameStartFifoWord& rhs) const {
    return (this->busy_violation == rhs.busy_violation &&
            this->BC_for_frame == rhs.BC_for_frame);
  }

  inline FrameStartFifoWord& operator=(const FrameStartFifoWord& rhs) {
    busy_violation = rhs.busy_violation;
    BC_for_frame = rhs.BC_for_frame;
    return *this;
  }

  inline friend void sc_trace(sc_trace_file *tf, const FrameStartFifoWord& dw,
                              const std::string& name ) {
    sc_trace(tf, dw.busy_violation, name + ".busy_violation");
    sc_trace(tf, dw.BC_for_frame, name + ".BC_for_frame");
  }

///@todo Overload this for all FrameStartFifoWord classes, so SystemC can print them to trace files properly?
  inline friend std::ostream& operator<<(std::ostream& stream, const FrameStartFifoWord& dw) {
    stream << dw.busy_violation;
    stream << ":";
    stream << dw.BC_for_frame;
    return stream;
  }
};


/// Data word stored in FRAME END FIFO
struct FrameEndFifoWord {
  bool flushed_incomplete;
  bool strobe_extended;
  bool busy_transition;

  inline bool operator==(const FrameEndFifoWord& rhs) const {
    return (this->flushed_incomplete == rhs.flushed_incomplete &&
            this->strobe_extended == rhs.strobe_extended &&
            this->busy_transition == rhs.busy_transition);
  }

  inline FrameEndFifoWord& operator=(const FrameEndFifoWord& rhs) {
    flushed_incomplete = rhs.flushed_incomplete;
    strobe_extended = rhs.strobe_extended;
    busy_transition = rhs.busy_transition;
    return *this;
  }

  inline friend void sc_trace(sc_trace_file *tf, const FrameEndFifoWord& dw,
                              const std::string& name ) {
    sc_trace(tf, dw.flushed_incomplete, name + ".flushed_incomplete");
    sc_trace(tf, dw.strobe_extended, name + ".strobe_extended");
    sc_trace(tf, dw.busy_transition, name + ".busy_transition");
  }

///@todo Overload this for all FrameEndFifoWord classes, so SystemC can print them to trace files properly?
  inline friend std::ostream& operator<<(std::ostream& stream, const FrameEndFifoWord& dw) {
    stream << "0b";
    stream << dw.flushed_incomplete;
    stream << dw.strobe_extended;
    stream << dw.busy_transition;
    return stream;
  }
};


///@brief The FIFOs in the Alpide chip are 24 bits, or 3 bytes, wide.
///       This is a base class for the data words that holds 3 bytes,
///       and is used as the data type in the SystemC FIFO templates.
///       This class shouldn't be used on its own, the various types
///       of data words are implemented in derived classes.
///       The data_type variable is used to more easily tell what
///       kind of data word this is, without parsing the data.
///       Virtual functions could have been used for this in the
///       derived classes, but it was done this way since virtual
///       functions are a bit slow and would be very frequently called.
class AlpideDataWord
{
public:
  uint8_t data[3];
  AlpideDataType data_type[3] = {ALPIDE_UNKNOWN,
                                 ALPIDE_UNKNOWN,
                                 ALPIDE_UNKNOWN};

  inline bool operator==(const AlpideDataWord& rhs) const {
    return (this->data[0] == rhs.data[0] &&
            this->data[1] == rhs.data[1] &&
            this->data[2] == rhs.data[2]);
  }

  inline AlpideDataWord& operator=(const AlpideDataWord& rhs) {
    data[0] = rhs.data[0];
    data[1] = rhs.data[1];
    data[2] = rhs.data[2];
    data_type[0] = rhs.data_type[0];
    data_type[1] = rhs.data_type[1];
    data_type[2] = rhs.data_type[2];
    return *this;
  }

  inline friend void sc_trace(sc_trace_file *tf, const AlpideDataWord& dw,
                              const std::string& name ) {
    sc_trace(tf, dw.data[0], name + ".byte0");
    sc_trace(tf, dw.data[1], name + ".byte1");
    sc_trace(tf, dw.data[2], name + ".byte2");
  }

///@todo Overload this for all AlpideDataWord classes, so SystemC can print them to trace files properly?
  inline friend std::ostream& operator<<(std::ostream& stream, const AlpideDataWord& alpide_dw) {
    stream << "0x";
    stream << std::hex << alpide_dw.data[0];
    stream << std::hex << alpide_dw.data[1];
    stream << std::hex << alpide_dw.data[2];
    return stream;
  }
};



class AlpideIdle : public AlpideDataWord
{
public:
  AlpideIdle() {
    data[2] = DW_IDLE;
    data[1] = DW_IDLE;
    data[0] = DW_IDLE;

    data_type[0] = ALPIDE_IDLE;
    data_type[1] = ALPIDE_IDLE;
    data_type[2] = ALPIDE_IDLE;
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

    data_type[0] = ALPIDE_CHIP_HEADER1;
    data_type[1] = ALPIDE_CHIP_HEADER2;
    data_type[2] = ALPIDE_IDLE;
  }
  AlpideChipHeader(uint8_t chip_id, FrameStartFifoWord& frame_start)
    : AlpideChipHeader(chip_id, frame_start.BC_for_frame) {}
};


class AlpideChipTrailer : public AlpideDataWord
{
public:
  AlpideChipTrailer(uint8_t readout_flags) {
    data[2] = DW_CHIP_TRAILER | (readout_flags & 0x0F);
    data[1] = DW_IDLE;
    data[0] = DW_IDLE;

    data_type[0] = ALPIDE_CHIP_TRAILER;
    data_type[1] = ALPIDE_IDLE;
    data_type[2] = ALPIDE_IDLE;
  }
  AlpideChipTrailer(FrameStartFifoWord frame_start,
                    FrameEndFifoWord frame_end,
                    bool fatal_state,
                    bool readout_abort) {
    // Special combinations of the readout flags are observed in
    // data overrun mode (ie. readout abort is set), and in fatal mode.
    if(fatal_state) {
      frame_start.busy_violation = true;
      frame_end.flushed_incomplete = true;
      frame_end.strobe_extended = true;
      frame_end.busy_transition = false;
    } else if(readout_abort) {
      frame_start.busy_violation = true;
      frame_end.flushed_incomplete = true;
      frame_end.strobe_extended = false;
      frame_end.busy_transition = false;
    }

    data[2] = DW_CHIP_TRAILER
      | (frame_start.busy_violation ? READOUT_FLAGS_BUSY_VIOLATION : 0)
      | (frame_end.flushed_incomplete ? READOUT_FLAGS_FLUSHED_INCOMPLETE : 0)
      | (frame_end.strobe_extended ? READOUT_FLAGS_STROBE_EXTENDED : 0)
      | (frame_end.busy_transition ? READOUT_FLAGS_BUSY_TRANSITION : 0);
    data[1] = DW_IDLE;
    data[0] = DW_IDLE;

    data_type[0] = ALPIDE_CHIP_TRAILER;
    data_type[1] = ALPIDE_IDLE;
    data_type[2] = ALPIDE_IDLE;
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

    data_type[0] = ALPIDE_CHIP_EMPTY_FRAME1;
    data_type[1] = ALPIDE_CHIP_EMPTY_FRAME2;
    data_type[2] = ALPIDE_IDLE;
  }
  AlpideChipEmptyFrame(uint8_t chip_id, FrameStartFifoWord& frame_start)
    : AlpideChipEmptyFrame(chip_id, frame_start.BC_for_frame) {}
};


class AlpideRegionHeader : public AlpideDataWord
{
public:
  AlpideRegionHeader(uint8_t region_id) {
    data[2] = DW_REGION_HEADER | (region_id & 0x1F);
    data[1] = DW_IDLE;
    data[0] = DW_IDLE;

    data_type[0] = ALPIDE_REGION_HEADER;
    data_type[1] = ALPIDE_IDLE;
    data_type[2] = ALPIDE_IDLE;
  }
};


class AlpideRegionTrailer : public AlpideDataWord
{
public:
  AlpideRegionTrailer() {
    // Region trailer word is triplicated in Alpide for SEU protection
    data[2] = DW_REGION_TRAILER;
    data[1] = DW_REGION_TRAILER;
    data[0] = DW_REGION_TRAILER;

    data_type[0] = ALPIDE_REGION_TRAILER;
    data_type[1] = ALPIDE_REGION_TRAILER;
    data_type[2] = ALPIDE_REGION_TRAILER;
  }
};


class AlpideDataShort : public AlpideDataWord
{
public:
  AlpideDataShort(uint8_t encoder_id, uint16_t addr) {
    data[2] = DW_DATA_SHORT | ((encoder_id & 0x0F) << 2) | ((addr >> 8) & 0x03);
    data[1] = addr & 0xFF;
    data[0] = DW_IDLE;

    data_type[0] = ALPIDE_DATA_SHORT1;
    data_type[1] = ALPIDE_DATA_SHORT2;
    data_type[2] = ALPIDE_IDLE;
  }
};


class AlpideDataLong : public AlpideDataWord
{
public:
  AlpideDataLong(uint8_t encoder_id, uint16_t addr, uint8_t hitmap) {
    data[2] = DW_DATA_LONG | ((encoder_id & 0x0F) << 2) | ((addr >> 8) & 0x03);
    data[1] = addr & 0xFF;
    data[0] = hitmap & 0x7F;

    data_type[0] = ALPIDE_DATA_LONG1;
    data_type[1] = ALPIDE_DATA_LONG2;
    data_type[2] = ALPIDE_DATA_LONG3;
  }
};


class AlpideBusyOn : public AlpideDataWord
{
public:
  AlpideBusyOn() {
    data[2] = DW_BUSY_ON;
    data[1] = DW_IDLE;
    data[0] = DW_IDLE;

    data_type[0] = ALPIDE_BUSY_ON;
    data_type[1] = ALPIDE_IDLE;
    data_type[2] = ALPIDE_IDLE;
  }
};


class AlpideBusyOff : public AlpideDataWord
{
public:
  AlpideBusyOff() {
    data[2] = DW_BUSY_OFF;
    data[1] = DW_IDLE;
    data[0] = DW_IDLE;

    data_type[0] = ALPIDE_BUSY_OFF;
    data_type[1] = ALPIDE_IDLE;
    data_type[2] = ALPIDE_IDLE;
  }
};


///@brief Included, but not really used. Should this even
///       appear on the serial data stream, or is it only
///       after encoding?
class AlpideComma : public AlpideDataWord
{
public:
  AlpideComma() {
    data[2] = DW_COMMA;
    data[1] = DW_COMMA;
    data[0] = DW_COMMA;

    data_type[0] = ALPIDE_COMMA;
    data_type[1] = ALPIDE_COMMA;
    data_type[2] = ALPIDE_COMMA;
  }
};


#endif
///@}
