#ifndef HIT_H
#define HIT_H

// TODO: reinclude pixel_col.h when I get it to compile :)
class PixelData
{
protected:
  int mCol;
  int mRow;

public:  
  PixelData(int col = 0, int row = 0) :mCol(col) ,mRow(row) {}
};


const int pixel_shaping_dead_time_ns = 200;
const int pixel_shaping_active_time_ns = 6000;

class Hit : public PixelData
{
private:
  int mChipId;
  int mDeadTimeCounter;
  int mActiveTimeCounter;
public:
  Hit();
  Hit(int chip_id, int col, int row,
      int dead_time_ns = pixel_shaping_dead_time_ns,
      int active_time_ns = pixel_shaping_active_time_ns);
  Hit(const Hit& h);
  bool isActive(void);
  void decreaseTimers(int time_ns);
  int timeLeft(void) const;  
  bool operator==(const Hit& rhs) const;
  bool operator>(const Hit& rhs) const;
  bool operator<(const Hit& rhs) const;
  bool operator>=(const Hit& rhs) const;
  bool operator<=(const Hit& rhs) const;
  
  Hit& operator=(const Hit& rhs);
  int getChipId(void) const {return mChipId;}
  int getCol(void) const {return mCol;}
  int getRow(void) const {return mRow;}
};


#endif
