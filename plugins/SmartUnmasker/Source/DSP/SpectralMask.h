#pragma once
#include <JuceHeader.h>
#include "FFTProcessor.h"

class SpectralMask
{
public:
    SpectralMask();

    void prepare(double sampleRate);

    void computeGainReduction(const float* mainMag, const float* sideMag,
                              float* gainReduction);

    void applySmoothing(float* gainReduction);

    void setAmount(float amount) { this->amount = amount; }
    void setAttack(float attackMs) { this->attackMs = attackMs; }
    void setRelease(float releaseMs) { this->releaseMs = releaseMs; }
    void setFreqSmooth(float smooth) { this->freqSmooth = smooth; }
    void setLowProtect(float freqHz) { this->lowProtectHz = freqHz; }

    static constexpr int numBins = FFTProcessor::fftSize / 2 + 1;

private:
    float amount = 1.0f;
    float attackMs = 5.0f;
    float releaseMs = 50.0f;
    float freqSmooth = 0.5f;
    float lowProtectHz = 100.0f;

    double currentSampleRate = 44100.0;

    std::array<float, numBins> envelopeState;
};
