#pragma once
#include <JuceHeader.h>

class FFTProcessor
{
public:
    FFTProcessor(int order)
        : fft(order),
          window(1 << order, juce::dsp::WindowingFunction<float>::hann)
    {
        size = 1 << order;
        buffer.resize(size * 2, 0.0f);
    }

    void forward(const float* input)
    {
        std::memcpy(buffer.data(), input, sizeof(float) * size);
        window.multiplyWithWindowingTable(buffer.data(), size);
        fft.performRealOnlyForwardTransform(buffer.data());
    }

    void inverse(float* output)
    {
        fft.performRealOnlyInverseTransform(buffer.data());
        std::memcpy(output, buffer.data(), sizeof(float) * size);
    }

    float* getData() { return buffer.data(); }
    int getSize() const { return size; }

private:
    int size;
    juce::dsp::FFT fft;
    juce::dsp::WindowingFunction<float> window;
    std::vector<float> buffer;
};
