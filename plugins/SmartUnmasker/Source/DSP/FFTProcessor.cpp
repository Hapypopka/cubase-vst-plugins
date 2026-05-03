#include "FFTProcessor.h"

FFTProcessor::FFTProcessor()
    : fft(fftOrder)
{
}

void FFTProcessor::forward(const float* input, float* complexOutput)
{
    std::memcpy(complexOutput, input, sizeof(float) * fftSize);
    std::memset(complexOutput + fftSize, 0, sizeof(float) * fftSize);
    fft.performRealOnlyForwardTransform(complexOutput, true);
}

void FFTProcessor::inverse(const float* complexInput, float* output)
{
    std::memcpy(output, complexInput, sizeof(float) * fftSize * 2);
    fft.performRealOnlyInverseTransform(output);
}

void FFTProcessor::computeMagnitude(const float* complexData, float* magnitude)
{
    const int numBins = fftSize / 2 + 1;
    for (int i = 0; i < numBins; ++i)
    {
        float re = complexData[i * 2];
        float im = complexData[i * 2 + 1];
        magnitude[i] = std::sqrt(re * re + im * im);
    }
}
