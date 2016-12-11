/**
 * @file   alpide_constants.h
 * @Author Simon Voigt Nesbo
 * @date   November 27, 2016
 * @brief  Various constants for alpide chip, 
 *         such as pixel matrix width and heigh, fifo depths, etc.
 *
 * Detailed description of file.
 */

#ifndef ALPIDE_CONSTANTS_H
#define ALPIDE_CONSTANTS_H


#define N_REGIONS 32
#define N_MULTI_EVENT_BUFFERS 3
#define N_PIXEL_ROWS 512
#define N_PIXEL_COLS 1024
#define N_PIXEL_COLS_PER_REGION (N_PIXEL_COLS/N_REGIONS)
#define N_PIXEL_DOUBLE_COLS_PER_REIGON (N_PIXEL_COLS_PER_REGION/2)
#define N_PIXELS_PER_REGION (N_PIXEL_COLS/N_REGIONS)


#endif
