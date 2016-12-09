#include "../alpide/pixel_col.h"
#define BOOST_TEST_MODULE PixelDoubleColumnTest
//#include <boost/test/unit_test.hpp>
#include <boost/test/included/unit_test.hpp>


BOOST_AUTO_TEST_CASE( pixel_col_test )
{
  const int test_col_num = 1;  
  const int test_row_num = 100;

  BOOST_TEST_MESSAGE("Creating PixelDoubleColumn object.");
  PixelDoubleColumn pixcol;

  BOOST_TEST_MESSAGE("Writing and reading out a pixel.");
  pixcol.setPixel(test_col_num, test_row_num);

  PixelData pixel = pixcol.readPixel();

  BOOST_CHECK_EQUAL(pixel.col, test_col_num);
  BOOST_CHECK_EQUAL(pixel.row, test_row_num);

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
    pixel = pixcol.readPixel();
    BOOST_CHECK_EQUAL(pixel.col, test_col_prioritized[i]);
    BOOST_CHECK_EQUAL(pixel.row, test_row_prioritized[i]);
  }
  
  BOOST_TEST_MESSAGE("Checking that pixels out of range throws exception.");

  BOOST_CHECK_THROW(pixcol.setPixel(0, N_PIXEL_ROWS), std::out_of_range);
  BOOST_CHECK_THROW(pixcol.setPixel(0, -1), std::out_of_range);
  BOOST_CHECK_THROW(pixcol.setPixel(-1, 0), std::out_of_range);
  BOOST_CHECK_THROW(pixcol.setPixel(2, 0), std::out_of_range);  
}
