/**
 * @file   alpide_constants.h
 * @author Simon Voigt Nesbo
 * @date   November 27, 2016
 * @brief  Various constants for alpide chip,
 *         such as pixel matrix width and heigh, fifo depths, etc.
 */


///@defgroup alpide_constants Alpide Constants
///@ingroup alpide
///@{
#ifndef ALPIDE_CONSTANTS_H
#define ALPIDE_CONSTANTS_H


#define N_REGIONS 32
#define N_PIXEL_ROWS 512
#define N_PIXEL_COLS 1024
#define N_PIXEL_COLS_PER_REGION (N_PIXEL_COLS/N_REGIONS)
#define N_PIXEL_DOUBLE_COLS_PER_REGION (N_PIXEL_COLS_PER_REGION/2)
#define N_PIXELS_PER_REGION (N_PIXEL_COLS/N_REGIONS)

#define TRU_FRAME_FIFO_ALMOST_FULL1 48
#define TRU_FRAME_FIFO_ALMOST_FULL2 56
#define TRU_FRAME_FIFO_SIZE 64

#define BUSY_FIFO_SIZE 4

#define DATA_LONG_PIXMAP_SIZE ((unsigned int) 7)

#define LHC_ORBIT_BUNCH_COUNT 3564

#define CHIP_WIDTH_CM 3
#define CHIP_HEIGHT_CM 1.5


#endif
///@}
