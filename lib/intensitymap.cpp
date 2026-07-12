// Copyright © 2015 by Simon Wendsche
// SPDX-License-Identifier: GPL-3.0-only

#include "intensitymap.h"
#include <QColor>
#include <iostream>

IntensityMap::IntensityMap() {

}

IntensityMap::IntensityMap(int width, int height) {
    map = std::vector< std::vector<double> >(height, std::vector<double>(width, 0.0));
}

IntensityMap::IntensityMap(const QImage& rgbImage, Mode mode, double redMultiplier, double greenMultiplier,
                           double blueMultiplier, double alphaMultiplier)
{
    map = std::vector< std::vector<double> >(rgbImage.height(), std::vector<double>(rgbImage.width(), 0.0));

    #pragma omp parallel for
    //for every row of the image
    for(int y = 0; y < rgbImage.height(); y++) {
        //for every column of the image
        for(int x = 0; x < rgbImage.width(); x++) {
            double intensity = 0.0;

            const double r = QColor(rgbImage.pixel(x, y)).redF() * redMultiplier;
            const double g = QColor(rgbImage.pixel(x, y)).greenF() * greenMultiplier;
            const double b = QColor(rgbImage.pixel(x, y)).blueF() * blueMultiplier;
            const double  a = QColor(rgbImage.pixel(x, y)).alphaF() * alphaMultiplier;

            if(mode == AVERAGE) {
                //take the average out of all selected channels
                int num_channels = 0;

                if(redMultiplier != 0.0) {
                    intensity += r;
                    num_channels++;
                }
                if(greenMultiplier != 0.0) {
                    intensity += g;
                    num_channels++;
                }
                if(blueMultiplier != 0.0) {
                    intensity += b;
                    num_channels++;
                }
                if(alphaMultiplier != 0.0) {
                    intensity += a;
                    num_channels++;
                }

                if(num_channels != 0)
                    intensity /= num_channels;
                else
                    intensity = 0.0;
            }
            else if(mode == MAX) {
                //take the maximum out of all selected channels
                const double tempMaxRG = std::max(r, g);
                const double tempMaxBA = std::max(b, a);
                intensity = std::max(tempMaxRG, tempMaxBA);
            }

            //add resulting pixel intensity to intensity map
            this->map.at(y).at(x) = intensity;
        }
    }
}

double IntensityMap::at(int x, int y) const {
    return this->map.at(y).at(x);
}

double IntensityMap::at(int pos) const {
    const int x = pos % this->getWidth();
    const int y = pos / this->getWidth();

    return this->at(x, y);
}

void IntensityMap::setValue(int x, int y, double value) {
    this->map.at(y).at(x) = value;
}

void IntensityMap::setValue(int pos, double value) {
    const int x = pos % this->getWidth();
    const int y = pos / this->getWidth();

    this->map.at(y).at(x) = value;
}

size_t IntensityMap::getWidth() const {
    return this->map.at(0).size();
}

size_t IntensityMap::getHeight() const {
    return this->map.size();
}

void IntensityMap::invert() {
    #pragma omp parallel for
    for(size_t y = 0; y < this->getHeight(); y++) {
        for(size_t x = 0; x < this->getWidth(); x++) {
            const double inverted = 1.0 - this->map.at(y).at(x);
            this->map.at(y).at(x) = inverted;
        }
    }
}

QImage IntensityMap::convertToQImage() const {
    QImage result(this->getWidth(), this->getHeight(), QImage::Format_ARGB32);

    for(size_t y = 0; y < this->getHeight(); y++) {
        QRgb *scanline = (QRgb*) result.scanLine(y);

        for(size_t x = 0; x < this->getWidth(); x++) {
            const int c = 255 * map.at(y).at(x);
            scanline[x] = qRgba(c, c, c, 255);
        }
    }

    return result;
}
