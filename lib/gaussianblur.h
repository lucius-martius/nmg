// Copyright © 2015 by Simon Wendsche
// SPDX-License-Identifier: GPL-3.0-only

#ifndef GAUSSIANBLUR_H
#define GAUSSIANBLUR_H

#include "intensitymap.h"

class GaussianBlur
{
public:
    GaussianBlur();
    IntensityMap calculate(IntensityMap& input, double radius, bool tileable);

private:
    std::vector<double> boxesForGauss(double sigma, int n);
    void gaussBlur(IntensityMap &input, IntensityMap &result, double radius, bool tileable);
    void boxBlur(IntensityMap &input, IntensityMap &result, double radius, bool tileable);
    void boxBlurH(IntensityMap &input, IntensityMap &result, double radius, bool tileable);
    void boxBlurT(IntensityMap &input, IntensityMap &result, double radius, bool tileable);
    int handleEdges(int iterator, int max, bool tileable) const;
};

#endif // GAUSSIANBLUR_H
