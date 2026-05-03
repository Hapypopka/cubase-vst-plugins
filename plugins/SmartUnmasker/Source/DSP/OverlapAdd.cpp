#include "OverlapAdd.h"
#include <cmath>

OverlapAdd::OverlapAdd()
{
    buildWindow();
    reset();
}

void OverlapAdd::prepare()
{
    reset();
}

void OverlapAdd::reset()
{
    inputBuffer.fill(0.0f);
    outputBuffer.fill(0.0f);
    writePos = 0;
    outputReadPos = 0;
    outputWritePos = 0;
}

void OverlapAdd::buildWindow()
{
    for (int i = 0; i < fftSize; ++i)
    {
        // Hann window
        window[i] = 0.5f * (1.0f - std::cos(2.0f * juce::MathConstants<float>::pi
                                              * static_cast<float>(i) / static_cast<float>(fftSize)));
    }
}

void OverlapAdd::pushSample(float sample)
{
    if (writePos < fftSize)
        inputBuffer[writePos] = sample;
    ++writePos;
}

void OverlapAdd::getFrame(float* output)
{
    for (int i = 0; i < fftSize; ++i)
        output[i] = inputBuffer[i] * window[i];

    // Shift input buffer by hopSize
    std::memmove(inputBuffer.data(), inputBuffer.data() + hopSize,
                 sizeof(float) * (fftSize - hopSize));
    std::memset(inputBuffer.data() + (fftSize - hopSize), 0, sizeof(float) * hopSize);
    writePos = fftSize - hopSize;
}

void OverlapAdd::addProcessedFrame(const float* frame)
{
    for (int i = 0; i < fftSize; ++i)
    {
        int idx = (outputWritePos + i) % (fftSize * 2);
        outputBuffer[idx] += frame[i] * window[i];
    }
    outputWritePos = (outputWritePos + hopSize) % (fftSize * 2);
}

float OverlapAdd::popSample()
{
    float sample = outputBuffer[outputReadPos];
    outputBuffer[outputReadPos] = 0.0f;
    outputReadPos = (outputReadPos + 1) % (fftSize * 2);
    return sample;
}
