#pragma once
#include <JuceHeader.h>
#include <vector>

class OverlapAdd
{
public:
    void prepare(int fftSizeIn, int hopSizeIn)
    {
        fftSize = fftSizeIn;
        hopSize = hopSizeIn;

        window.resize(fftSize);
        outputBuffer.resize(fftSize * 2, 0.0f);
        writePos = 0;

        juce::dsp::WindowingFunction<float>::fillWindowingTables(
            window.data(),
            fftSize,
            juce::dsp::WindowingFunction<float>::hann,
            false);
    }

    void applyWindow(float* data)
    {
        for (int i = 0; i < fftSize; ++i)
            data[i] *= window[i];
    }

    void addToOutput(const float* block)
    {
        for (int i = 0; i < fftSize; ++i)
        {
            int idx = (writePos + i) % (fftSize * 2);
            outputBuffer[idx] += block[i];
        }
    }

    float getNextSample()
    {
        float val = outputBuffer[writePos];
        outputBuffer[writePos] = 0.0f;
        writePos = (writePos + 1) % (fftSize * 2);
        return val;
    }

    void reset()
    {
        std::fill(outputBuffer.begin(), outputBuffer.end(), 0.0f);
        writePos = 0;
    }

private:
    int fftSize = 0;
    int hopSize = 0;
    int writePos = 0;
    std::vector<float> window;
    std::vector<float> outputBuffer;
};
