/**
 * @file   data_format.h
 * @Author Simon Voigt Nesbo
 * @date   November 27, 2016
 * @brief  Defines the different data words etc. 
 *         used in the Alpide chip's data transmission
 *
 * Detailed description of file.
 */

enum DataWordID {IDLE, CHIP_HEADER, CHIP_TRAILER, CHIP_EMPTY_FRAME, REGION_HEADER, DATA_SHORT, DATA_LONG, BUSY_ON, BUSY_OFF, INCOMPLETE};

class DataWordBase
{
private:
  int size;
  int size_received;
  int size_transmitted;
  DataWordBase data_word;
};


class DataWordIdle : DataWordBase
{
public:
  DataWordIdle() : size(1), size_received(0), size_transmitted(0), data_word(IDLE) {}
};


class DataWordChipHeader : DataWordBase
{
public:
  DataWordChipHeader() : size(2), size_received(0), size_transmitted(0), data_word(CHIP_HEADER) {}
};


class DataWordChipTrailer : DataWordBase
{
public:
  DataWordChipTrailer() : size(1), size_received(0), size_transmitted(0), data_word(CHIP_TRAILER) {}
};


class DataWordChipEmptyFrame : DataWordBase
{
public:
  DataWordChipEmptyFrame() : size(2), size_received(0), size_transmitted(0), data_word(CHIP_EMPTY_FRAME) {}
};


class DataWordRegionHeader : DataWordBase
{
public:
  DataWordRegionHeader() : size(1), size_received(0), size_transmitted(0), data_word(REGION_HEADER) {}
};

  
class DataWordDataShort : DataWordBase
{
public:
  DataWordDataShort() : size(2), size_received(0), size_transmitted(0), data_word(DATA_SHORT) {}
};


class DataWordDataLong : DataWordBase
{
public:
  DataWordDataLong() : size(3), size_received(0), size_transmitted(0), data_word(DATA_LONG) {}
};  


class DataWordBusyOn : DataWordBase
{
public:
  DataWordBusyOn() : size(1), size_received(0), size_transmitted(0), data_word(BUSY_ON) {}
};


class DataWordBusyOff : DataWordBase
{
public:
  DataWordBusyOff() : size(1), size_received(0), size_transmitted(0), data_word(BUSY_OFF) {}
};


class DataWordIncomplete : DataWordBase
{
public:
  DataWordIncomplete(int data_word_size) : size(data_word_size), size_received(0), size_transmitted(0), data_word(INCOMPLETE) {}
};
