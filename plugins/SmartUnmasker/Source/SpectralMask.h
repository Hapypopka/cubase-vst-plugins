#pragma once
#include <JuceHeader.h>
#include <vector>
#include <cmath>

class SpectralMask
{
public:
    void prepare(int fftSize)
    {
        envelope.resize(fftSize, 0.0f);
    }

    void process(float* mainFFT, float* sideFFT, int size, float amount, float attackCoeff, float releaseCoeff)
    {
        for (int i = 0; i < size; i += 2)
        {
            float magM = std::sqrt(mainFFT[i] * mainFFT[i] + mainFFT[i + 1] * mainFFT[i + 1]);
            float magS = std::sqrt(sideFFT[i] * sideFFT[i] + sideFFT[i + 1] * sideFFT[i + 1]);

            float target = magS / (magM + 1e-6f);
            target = juce::jlimit(0.0f, 1.0f, target * amount);

            if (target > envelope[i])
                envelope[i] += attackCoeff * (target - envelope[i]);
            else
                envelope[i] += releaseCoeff * (target - envelope[i]);

            float gain = 1.0f - envelope[i];
            mainFFT[i] *= gain;
            mainFFT[i + 1] *= gain;
        }
    }

    void reset()
    {
        std::fill(envelope.begin(), envelope.end(), 0.0f);
    }

private:
    std::vector<float> envelope;
};
