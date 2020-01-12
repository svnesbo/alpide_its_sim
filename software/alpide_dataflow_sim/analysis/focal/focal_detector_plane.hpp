#ifndef FOCAL_DETECTOR_PLANE_HPP
#define FOCAL_DETECTOR_PLANE_HPP

#include <TH2Poly.h>

void create_focal_chip_bins(TH2Poly* th2);
unsigned int bin_number_to_radius_bin(unsigned int bin_num);

#endif
