#include "focal_detector_plane.hpp"
#include "../../src/Detector/Focal/Focal_constants.hpp"
#include "../../src/Detector/Focal/FocalDetectorConfig.hpp"

const double chip_size_x_mm = 30;
const double chip_size_y_mm = 15;


///@brief Create the polygons/bins for the chips in the Focal
///       detector plane on a TH2Poly 2d-histogram object
///@param th2 Pointer to TH2Poly instance to create the polygons/bins for
void create_focal_chip_bins(TH2Poly* th2)
{
  for(unsigned int quadrant = 1; quadrant <= 4; quadrant++) {
    for(unsigned int half_patch_num = 0; half_patch_num < Focal::HALF_PATCHES_PER_QUADRANT; half_patch_num++) {
      for(unsigned int stave_num_in_half_patch = 0;
          stave_num_in_half_patch < Focal::STAVES_PER_HALF_PATCH;
          stave_num_in_half_patch++) {
        unsigned int stave_num = half_patch_num*Focal::STAVES_PER_HALF_PATCH+stave_num_in_half_patch;

        double y_low = stave_num*chip_size_y_mm;
        double y_high = y_low+chip_size_y_mm;
        double x_start = 0;

        if(y_low < Focal::GAP_SIZE_Y_MM/2)
          x_start = Focal::GAP_SIZE_X_MM/2;

        for(unsigned int chip_num = 0; chip_num < Focal::CHIPS_PER_STAVE; chip_num++) {
          double x_low = x_start + chip_num*chip_size_x_mm;
          double x_high = x_low+chip_size_x_mm;

          if(quadrant == 1) {
            th2->AddBin(x_low, y_low, x_high, y_high);
          } else if(quadrant == 2) {
            th2->AddBin(-x_low, y_low, -x_high, y_high);
          } else if(quadrant == 3) {
            th2->AddBin(-x_low, -y_low, -x_high, -y_high);
          } else {
            th2->AddBin(x_low, -y_low, x_high, -y_high);
          }
        }
      }
    }
  }
}


///@brief Get a rough radius/distance of the specified bin number (ie. chip)
///       from the beam center
///@param bin_num Bin number in Focal plane
///@return Radius in units of chip widths (3 cm) + from gap around beam center.
///        E.g. return value 0 = 4cm + 0 x 3cm = 4cm radius
///                          1 = 4cm + 1 x 3cm = 7cm radius
///                          2 = 4cm + 2 x 3cm = 10cm radius
///                          and so on..
unsigned int bin_number_to_radius(unsigned int bin_num)
{
  unsigned int radius = 0;
  unsigned int chip_id = bin_num-1;

  Detector::DetectorPosition pos = Focal::Focal_global_chip_id_to_position(chip_id);

  unsigned int quadrant = pos.stave_id / Focal::STAVES_PER_QUADRANT;
  unsigned int stave_num_in_quadrant = pos.stave_id - quadrant * Focal::STAVES_PER_QUADRANT;
  unsigned int half_patch_num = stave_num_in_quadrant / Focal::STAVES_PER_HALF_PATCH;

  unsigned int chip_num_in_stave = pos.module_chip_id;

  if(pos.module_id > 0) {
    if(stave_num_in_quadrant < Focal::INNER_STAVES_PER_QUADRANT)
      chip_num_in_stave += Focal::CHIPS_PER_FOCAL_IB_MODULE;
    else
      chip_num_in_stave += pos.module_id * Focal::CHIPS_PER_FOCAL_OB_MODULE;
  }

  if(half_patch_num > 0) {
    radius = (stave_num_in_quadrant-3)/2;

    if(chip_num_in_stave > 1) {
      radius = radius + (pos.module_chip_id - 1);

      if(radius > Focal::CHIPS_PER_STAVE)
        radius = Focal::CHIPS_PER_STAVE;
    }
  } else {
    if(chip_num_in_stave >= Focal::CHIPS_PER_STAVE)
      radius = Focal::CHIPS_PER_STAVE;
    else
      radius = chip_num_in_stave;
  }

  return radius;
}
