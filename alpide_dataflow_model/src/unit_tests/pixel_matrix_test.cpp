#include "pixel_matrix.h"
#define BOOST_TEST_MODULE PixelMatrixTest
#include <boost/test/included/unit_test.hpp>


BOOST_AUTO_TEST_CASE( pixel_matrix_test )
{
  const int test_col_num = 234;  
  const int test_row_num = 305;

  BOOST_TEST_MESSAGE("Creating PixelMatrix object.");
  PixelMatrix matrix;

  BOOST_TEST_MESSAGE("Creating new event.");
  matrix.newEvent();

  BOOST_TEST_MESSAGE("Check matrix contains one event now.");
  BOOST_CHECK_EQUAL(matrix.getNumEvents(), 1);
  
  BOOST_TEST_MESSAGE("Writing a pixel.");
  matrix.setPixel(test_col_num, test_row_num);

  BOOST_TEST_MESSAGE("Check matrix contains one hit now.");
  BOOST_CHECK_EQUAL(matrix.getHitsRemainingInOldestEvent(), 1);
  BOOST_CHECK_EQUAL(matrix.getHitTotalAllEvents(), 1);

  BOOST_TEST_MESSAGE("Reading out pixel and checking value.");
  PixelData pixel = matrix.readPixel();
  BOOST_CHECK_EQUAL(pixel.getCol(), test_col_num);
  BOOST_CHECK_EQUAL(pixel.getRow(), test_row_num);
  BOOST_CHECK(pixel == PixelData(test_col_num, test_row_num));

  BOOST_TEST_MESSAGE("Check matrix has zero hits and events remaining.");
  BOOST_CHECK_EQUAL(matrix.getHitsRemainingInOldestEvent(), 0);
  BOOST_CHECK_EQUAL(matrix.getHitTotalAllEvents(), 0);
  BOOST_CHECK_EQUAL(matrix.getNumEvents(), 0);  

  BOOST_TEST_MESSAGE("Attempting to read out another pixel, checking that there are no more hits.");
  pixel = matrix.readPixel();
  BOOST_CHECK(pixel == NoPixelHit);

  BOOST_TEST_MESSAGE("Writing two pixels to region 11");
  matrix.newEvent();
  matrix.setPixel(N_PIXEL_COLS_PER_REGION*11, 234);
  matrix.setPixel(1+N_PIXEL_COLS_PER_REGION*11, 432);
  BOOST_TEST_MESSAGE("Check that matrix has 2 hits now.");
  BOOST_CHECK_EQUAL(matrix.getHitsRemainingInOldestEvent(), 2);
  BOOST_CHECK_EQUAL(matrix.getHitTotalAllEvents(), 2);

  BOOST_TEST_MESSAGE("Checking that region overloaded readPixel() does not read out from region other than 11.");
  for(int i = 0; i < N_REGIONS; i++) {
    if(i == 11)
      continue;
    pixel = matrix.readPixelRegion(i);
    BOOST_CHECK(pixel == NoPixelHit);
  }
  
  BOOST_TEST_MESSAGE("Checking that region overloaded readPixel() reads out from region 11.");
  pixel = matrix.readPixelRegion(11);
  BOOST_CHECK_EQUAL(pixel.getCol(), N_PIXEL_COLS_PER_REGION*11);
  BOOST_CHECK_EQUAL(pixel.getRow(), 234);
  pixel = matrix.readPixelRegion(11);
  BOOST_CHECK_EQUAL(pixel.getCol(), 1+N_PIXEL_COLS_PER_REGION*11);
  BOOST_CHECK_EQUAL(pixel.getRow(), 432);

  BOOST_TEST_MESSAGE("Check matrix has zero hits and events remaining.");
  BOOST_CHECK_EQUAL(matrix.getHitsRemainingInOldestEvent(), 0);
  BOOST_CHECK_EQUAL(matrix.getHitTotalAllEvents(), 0);
  BOOST_CHECK_EQUAL(matrix.getNumEvents(), 0);
  
  
  BOOST_TEST_MESSAGE("Write some pixels to two different events, and check that they are read out correctly.");
  const int test_cols[4] = {100, 101, 100, 101};
  const int test_rows[4] = {234, 435, 123, 123};
  PixelData pixel0 = PixelData(test_cols[0], test_rows[0]);
  PixelData pixel1 = PixelData(test_cols[1], test_rows[1]);
  PixelData pixel2 = PixelData(test_cols[2], test_rows[2]);
  PixelData pixel3 = PixelData(test_cols[3], test_rows[3]);

  matrix.newEvent();
  matrix.setPixel(test_cols[0], test_rows[0]);
  matrix.setPixel(test_cols[1], test_rows[1]);

  matrix.newEvent();
  matrix.setPixel(test_cols[2], test_rows[2]);
  matrix.setPixel(test_cols[3], test_rows[3]);

  BOOST_CHECK_EQUAL(matrix.getNumEvents(), 2);
  BOOST_CHECK_EQUAL(matrix.getHitTotalAllEvents(), 4);
  BOOST_CHECK_EQUAL(matrix.getHitsRemainingInOldestEvent(), 2);
  pixel = matrix.readPixel();
  BOOST_CHECK(pixel == pixel0); // Note, not same order as they were inserted due to pri encoder

  BOOST_CHECK_EQUAL(matrix.getNumEvents(), 2);
  BOOST_CHECK_EQUAL(matrix.getHitTotalAllEvents(), 3);
  BOOST_CHECK_EQUAL(matrix.getHitsRemainingInOldestEvent(), 1);
  pixel = matrix.readPixel();
  BOOST_CHECK(pixel == pixel1); // Note, not same order as they were inserted due to pri encoder

  BOOST_CHECK_EQUAL(matrix.getNumEvents(), 1);
  BOOST_CHECK_EQUAL(matrix.getHitTotalAllEvents(), 2);
  BOOST_CHECK_EQUAL(matrix.getHitsRemainingInOldestEvent(), 2);
  pixel = matrix.readPixel();
  BOOST_CHECK(pixel == pixel3); // Note, not same order as they were inserted due to pri encoder

  BOOST_CHECK_EQUAL(matrix.getNumEvents(), 1);
  BOOST_CHECK_EQUAL(matrix.getHitTotalAllEvents(), 1);
  BOOST_CHECK_EQUAL(matrix.getHitsRemainingInOldestEvent(), 1);
  pixel = matrix.readPixel();
  BOOST_CHECK(pixel == pixel2); // Note, not same order as they were inserted due to pri encoder

  BOOST_CHECK_EQUAL(matrix.getNumEvents(), 0);
  BOOST_CHECK_EQUAL(matrix.getHitTotalAllEvents(), 0);
  BOOST_CHECK_EQUAL(matrix.getHitsRemainingInOldestEvent(), 0);
  pixel = matrix.readPixel();
  BOOST_CHECK(pixel == NoPixelHit);  
  

  BOOST_TEST_MESSAGE("Testing writing and reading to/from all regions");
  matrix.newEvent();
  for(int i = 0; i < N_REGIONS; i++) {
    matrix.setPixel(i*N_PIXEL_COLS_PER_REGION, i);
  }

  BOOST_CHECK_EQUAL(matrix.getNumEvents(), 1);
  BOOST_CHECK_EQUAL(matrix.getHitTotalAllEvents(), N_REGIONS);
  BOOST_CHECK_EQUAL(matrix.getHitsRemainingInOldestEvent(), N_REGIONS);

  for(int i = 0; i < N_REGIONS; i++) {
    pixel = matrix.readPixelRegion(i);
    BOOST_CHECK_EQUAL(pixel.getCol(), i*N_PIXEL_COLS_PER_REGION);
    BOOST_CHECK_EQUAL(pixel.getRow(), i);
    BOOST_CHECK_EQUAL(matrix.getHitTotalAllEvents(), N_REGIONS-(i+1));
    BOOST_CHECK_EQUAL(matrix.getHitsRemainingInOldestEvent(), N_REGIONS-(i+1));    
  }
  BOOST_CHECK_EQUAL(matrix.getNumEvents(), 0);


  BOOST_TEST_MESSAGE("Testing priority encoder readout order.");

  // Pixels that are shown in figure 4.5 of ALPIDE operations manual v0.3
  const int test_col_unprioritized[16] = {  0,   0,   0,   0, 0, 0, 0, 0,   1,   1,   1,   1, 1, 1, 1, 1};
  const int test_row_unprioritized[16] = {508, 509, 510, 511, 0, 1, 2, 3, 508, 509, 510, 511, 0, 1, 2, 3};

  // The same pixels, but in the order the priority encoder should read them out
  const int test_col_prioritized[16] = {0, 1, 1, 0, 0, 1, 1, 0, 0,     1,   1,   0,   0,   1,   1,   0};
  const int test_row_prioritized[16] = {0, 0, 1, 1, 2, 2, 3, 3, 508, 508, 509, 509, 510, 510, 511, 511};
  
  // Write the pixels to the double column
  matrix.newEvent();
  for(int i = 0; i < 16; i++) {
    matrix.setPixel(test_col_unprioritized[i], test_row_unprioritized[i]);
  }


  // Read back pixels and check prioritization
  for(int i = 0; i < 16; i++) {
    PixelData pixel_prioritized = PixelData(test_col_prioritized[i], test_row_prioritized[i]);
    pixel = matrix.readPixel();
    
    BOOST_CHECK(pixel == pixel_prioritized);
  }

  BOOST_TEST_MESSAGE("Checking that setting a pixel when there are no events throws an exception.");
  BOOST_CHECK_THROW(matrix.setPixel(0, 0), std::out_of_range);


  BOOST_TEST_MESSAGE("Checking that setting pixels out of range throws exception.");
  matrix.newEvent();
  BOOST_CHECK_THROW(matrix.setPixel(0, N_PIXEL_ROWS), std::out_of_range);
  BOOST_CHECK_THROW(matrix.setPixel(0, -1), std::out_of_range);
  BOOST_CHECK_THROW(matrix.setPixel(-1, 0), std::out_of_range);
  BOOST_CHECK_THROW(matrix.setPixel(N_PIXEL_COLS, 0), std::out_of_range);


  BOOST_TEST_MESSAGE("Checking that trying to read out pixels outside allowed range throws exception.");
  BOOST_CHECK_THROW(matrix.readPixel(-1, N_PIXEL_COLS/2), std::out_of_range);
  BOOST_CHECK_THROW(matrix.readPixel(N_PIXEL_COLS/2, N_PIXEL_COLS/2), std::out_of_range);
  BOOST_CHECK_THROW(matrix.readPixel(101, 100), std::out_of_range);
  BOOST_CHECK_THROW(matrix.readPixel(0, 0), std::out_of_range);
  BOOST_CHECK_THROW(matrix.readPixel(0, N_PIXEL_COLS/2 + 1), std::out_of_range);
  BOOST_CHECK_THROW(matrix.readPixelRegion(-1), std::out_of_range);
  BOOST_CHECK_THROW(matrix.readPixelRegion(N_REGIONS), std::out_of_range);
                    

  BOOST_TEST_MESSAGE("Checking that pixels at the boundaries can be set, and read out correctly.");
  matrix.setPixel(0, 0);
  matrix.setPixel(N_PIXEL_COLS-1, 0);  
  matrix.setPixel(0, N_PIXEL_ROWS-1);
  matrix.setPixel(N_PIXEL_COLS-1, N_PIXEL_ROWS-1);

  pixel = matrix.readPixel();
  BOOST_CHECK_EQUAL(pixel.getCol(), 0);
  BOOST_CHECK_EQUAL(pixel.getRow(), 0);

  pixel = matrix.readPixel();
  BOOST_CHECK_EQUAL(pixel.getCol(), 0);
  BOOST_CHECK_EQUAL(pixel.getRow(), N_PIXEL_ROWS-1);
  
  pixel = matrix.readPixel();
  BOOST_CHECK_EQUAL(pixel.getCol(), N_PIXEL_COLS-1);
  BOOST_CHECK_EQUAL(pixel.getRow(), 0);

  pixel = matrix.readPixel();
  BOOST_CHECK_EQUAL(pixel.getCol(), N_PIXEL_COLS-1);
  BOOST_CHECK_EQUAL(pixel.getRow(), N_PIXEL_ROWS-1);  
}
