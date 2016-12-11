#include "../alpide/pixel_region.h"
#define BOOST_TEST_MODULE PixelRegionTest
#include <boost/test/included/unit_test.hpp>


BOOST_AUTO_TEST_CASE( pixel_region_test )
{
  const int test_col_num = 1;  
  const int test_row_num = 100;

  BOOST_TEST_MESSAGE("Creating PixelDoubleColumn object.");
  PixelDoubleColumn pixcol;

  BOOST_TEST_MESSAGE("Writing and reading out a pixel.");
  pixcol.setPixel(test_col_num, test_row_num);

  PixelData pixel = pixcol.readPixel();
  BOOST_CHECK(pixel == PixelData(test_col_num, test_row_num));

  
  BOOST_TEST_MESSAGE("Writing some pixels, and checking that they can be inspected without deletion from MEB.");
  const int test_cols[4] = {0, 1, 0, 0};
  const int test_rows[4] = {234, 435, 123, 23};
  PixelData pixel0 = PixelData(test_cols[0], test_rows[0]);
  PixelData pixel1 = PixelData(test_cols[1], test_rows[1]);
  PixelData pixel2 = PixelData(test_cols[2], test_rows[2]);
  PixelData pixel3 = PixelData(test_cols[3], test_rows[3]);

  // Set the pixels
  pixcol.setPixel(test_cols[0], test_rows[0]);
  pixcol.setPixel(test_cols[1], test_rows[1]);
  pixcol.setPixel(test_cols[2], test_rows[2]);
  pixcol.setPixel(test_cols[3], test_rows[3]);

  // Inspect that the pixels are set, but in a different (non-prioritized) order than they were inserted
  BOOST_CHECK(pixcol.inspectPixel(test_cols[2], test_rows[2]));
  BOOST_CHECK(pixcol.inspectPixel(test_cols[1], test_rows[1]));
  BOOST_CHECK(pixcol.inspectPixel(test_cols[0], test_rows[0]));
  BOOST_CHECK(pixcol.inspectPixel(test_cols[3], test_rows[3]));

  // Check that the pixels were not removed from memory
  BOOST_CHECK_EQUAL(pixcol.pixelHitsRemaining(), 4);

  // Read out 4 pixels
  pixcol.readPixel();
  pixcol.readPixel();
  pixcol.readPixel();
  pixcol.readPixel();

  // Check that pixels were removed from memory now
  BOOST_CHECK_EQUAL(pixcol.pixelHitsRemaining(), 0);

  
  
  BOOST_TEST_MESSAGE("Testing priority encoder readout order.");

  // Pixels that are shown in figure 4.5 of ALPIDE operations manual v0.3
  const int test_col_unprioritized[16] = {  0,   0,   0,   0, 0, 0, 0, 0,   1,   1,   1,   1, 1, 1, 1, 1};
  const int test_row_unprioritized[16] = {508, 509, 510, 511, 0, 1, 2, 3, 508, 509, 510, 511, 0, 1, 2, 3};

  // The same pixels, but in the order the priority encoder should read them out
  const int test_col_prioritized[16] = {0, 1, 1, 0, 0, 1, 1, 0, 0,     1,   1,   0,   0,   1,   1,   0};
  const int test_row_prioritized[16] = {0, 0, 1, 1, 2, 2, 3, 3, 508, 508, 509, 509, 510, 510, 511, 511};
  
  // Write the pixels to the double column
  for(int i = 0; i < 16; i++) {
    pixcol.setPixel(test_col_unprioritized[i], test_row_unprioritized[i]);
  }


  // Read back pixels and check prioritization
  for(int i = 0; i < 16; i++) {
    PixelData pixel_prioritized = PixelData(test_col_prioritized[i], test_row_prioritized[i]);
    pixel = pixcol.readPixel();
    
    BOOST_CHECK(pixel == pixel_prioritized);
  }
  
  BOOST_TEST_MESSAGE("Checking that setting pixels out of range throws exception.");

  BOOST_CHECK_THROW(pixcol.setPixel(0, N_PIXEL_ROWS), std::out_of_range);
  BOOST_CHECK_THROW(pixcol.setPixel(0, -1), std::out_of_range);
  BOOST_CHECK_THROW(pixcol.setPixel(-1, 0), std::out_of_range);
  BOOST_CHECK_THROW(pixcol.setPixel(2, 0), std::out_of_range);


  BOOST_TEST_MESSAGE("Checking that inspecting pixels out of range throws exception.");

  BOOST_CHECK_THROW(pixcol.inspectPixel(0, N_PIXEL_ROWS), std::out_of_range);
  BOOST_CHECK_THROW(pixcol.inspectPixel(0, -1), std::out_of_range);
  BOOST_CHECK_THROW(pixcol.inspectPixel(-1, 0), std::out_of_range);
  BOOST_CHECK_THROW(pixcol.inspectPixel(2, 0), std::out_of_range);    
}
