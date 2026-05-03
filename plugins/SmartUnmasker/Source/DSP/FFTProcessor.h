#pragma once
#include <JuceHeader.h>

class FFTProcessor
{
public:
    static constexpr int fftOrder = 11;
    static constexpr int fftSize = 1 << fftOrder; // 2048

    FFTProcessor();

    void forward(const float* input, float* complexOutput);
    void inverse(const float* complexInput, float* output);

    void computeMagnitude(const float* complexData, float* magnitude);

    int getFFTSize() const { return fftSize; }

private:
    juce::dsp::FFT fft;
};
