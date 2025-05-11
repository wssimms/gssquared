/*
 *   Copyright (c) 2025 Jawaid Bazyar

 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.

 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.

 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <cmath>
#include <vector>

class LowPassFilter {
private:
    double a0, a1, a2, b0, b1, b2;
    double x1, x2, y1, y2;

public:
    // Constructor - initialize the filter with default values
    LowPassFilter();

    // Calculate filter coefficients based on cutoff frequency and sample rate
    void setCoefficients(double cutoffFreq, double sampleRate);

    // Process a single sample
    double process(double input);
    
    // Process a block of samples
    std::vector<double> processBlock(const std::vector<double>& inputBlock);
    
    // Reset filter state
    void reset();
};
