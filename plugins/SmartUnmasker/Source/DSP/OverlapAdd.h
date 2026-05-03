#pragma once
#include <JuceHeader.h>
#include "FFTProcessor.h"

class OverlapAdd
{
public:
    static constexpr int fftSize = FFTProcessor::fftSize;
    static constexpr int hopSize = fftSize / 4;
    static constexpr int numOverlaps = fftSize / hopSize; // 4

    OverlapAdd();

    void prepare();
    void reset();

    void pushSample(float sample);
    bool isFrameReady() const { return writePos >= fftSize; }

    void getFrame(float* output);
    void addProcessedFrame(const float* frame);
    float popSample();

private:
    void buildWindow();

    std::array<float, fftSize> inputBuffer;
    std::array<float, fftSize * 2> outputBuffer;
    std::array<float, fftSize> window;

    int writePos = 0;
    int readPos = 0;
    int outputReadPos = 0;
    int outputWritePos = 0;
};
