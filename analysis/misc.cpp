/**
 * @file   misc.cpp
 * @author Simon Voigt Nesbo
 * @date   February 13, 2018
 * @brief  Misc functions
 *
 */

#include "misc.h"

void scale_eff_plot_y_range(TH1* h)
{
  double min;
  double max;
  double delta;

  min = h->GetMinimum();
  max = h->GetMaximum();
  delta = max - min;

  max = (max+(delta/10)) > 1.0 ? 1.02 : (max+(delta/10));
  min = (min-(delta/10)) < 0.0 ? 0.0 : (min-(delta/10));

  h->SetMaximum(max);
  h->SetMinimum(min);
}
