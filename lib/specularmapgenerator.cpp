// Copyright © 2015 by Simon Wendsche
// SPDX-License-Identifier: GPL-3.0-only

#include "specularmapgenerator.h"
#include <QColor>

SpecularmapGenerator::SpecularmapGenerator(IntensityMap::Mode mode, double redMultiplier, double greenMultiplier, double blueMultiplier, double alphaMultiplier)
{
    this->mode = mode;
    this->redMultiplier = redMultiplier;
    this->greenMultiplier = greenMultiplier;
    this->blueMultiplier = blueMultiplier;
    this->alphaMultiplier = alphaMultiplier;
}

QImage SpecularmapGenerator::calculateSpecmap(const QImage &input, double scale, double contrast) {
    QImage result(input.width(), input.height(), QImage::Format_ARGB32);
    
    //generate contrast lookup table
    unsigned short contrastLookup[256];
    double newValue = 0;
    
    for(int i = 0; i < 256; i++) {
        newValue = (double)i;
        newValue /= 255.0;
        newValue -= 0.5;
        newValue *= contrast;
        newValue += 0.5;
        newValue *= 255;
    
        if(newValue < 0)
            newValue = 0;
        if(newValue > 255)
            newValue = 255;
        
        contrastLookup[i] = (unsigned short)newValue;
    }
    
    // This is outside of the loop because the multipliers are the same for every pixel
    double multiplierSum = ((redMultiplier != 0.0) + (greenMultiplier != 0.0) +
            (blueMultiplier != 0.0) + (alphaMultiplier != 0.0));
    if(multiplierSum == 0.0)
        multiplierSum = 1.0;

    #pragma omp parallel for  // OpenMP
    //for every row of the image
    for(int y = 0; y < result.height(); y++) {
        QRgb *scanline = (QRgb*) result.scanLine(y);

        //for every column of the image
        for(int x = 0; x < result.width(); x++) {
            double intensity = 0.0;

            const QColor pxColor = QColor(input.pixel(x, y));

            const double r = pxColor.redF() * redMultiplier;
            const double g = pxColor.greenF() * greenMultiplier;
            const double b = pxColor.blueF() * blueMultiplier;
            const double a = pxColor.alphaF() * alphaMultiplier;

            if(mode == IntensityMap::AVERAGE) {
                //take the average out of all selected channels
                intensity = (r + g + b + a) / multiplierSum;
            }
            else if(mode == IntensityMap::MAX) {
                //take the maximum out of all selected channels
                const double tempMaxRG = std::max(r, g);
                const double tempMaxBA = std::max(b, a);
                intensity = std::max(tempMaxRG, tempMaxBA);
            }

            //apply scale (brightness)
            intensity *= scale;

            //clamp
            if(intensity > 1.0)
                intensity = 1.0;

            //convert intensity to the 0-255 range
            int c = (int)(255.0 * intensity);
            
            //apply contrast
            c = (int)contrastLookup[c];
            
            //write color into image pixel
            scanline[x] = qRgba(c, c, c, pxColor.alpha());
        }
    }

    return result;
}
