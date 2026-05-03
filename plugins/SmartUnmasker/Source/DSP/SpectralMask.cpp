#include "SpectralMask.h"
#include <cmath>
#include <algorithm>

SpectralMask::SpectralMask()
{
    envelopeState.fill(1.0f);
}

void SpectralMask::prepare(double sampleRate)
{
    currentSampleRate = sampleRate;
    envelopeState.fill(1.0f);
}

void SpectralMask::computeGainReduction(const float* mainMag, const float* sideMag,
                                         float* gainReduction)
{
    const float hopSize = static_cast<float>(FFTProcessor::fftSize) / 4.0f;
    const float hopDurationSec = hopSize / static_cast<float>(currentSampleRate);

    const float attackCoeff = 1.0f - std::exp(-2.2f / (attackMs * 0.001f / hopDurationSec));
    const float releaseCoeff = 1.0f - std::exp(-2.2f / (releaseMs * 0.001f / hopDurationSec));

    const float binFreqStep = static_cast<float>(currentSampleRate) / static_cast<float>(FFTProcessor::fftSize);

    for (int i = 0; i < numBins; ++i)
    {
        const float binFreq = static_cast<float>(i) * binFreqStep;

        float ratio = 1.0f;
        if (mainMag[i] > 1e-10f && sideMag[i] > 1e-10f)
        {
            float maskStrength = sideMag[i] / (mainMag[i] + sideMag[i]);
            ratio = 1.0f - maskStrength * amount;
            ratio = std::max(ratio, 0.01f);
        }

        // Low-frequency protection: fade in the effect above lowProtectHz
        if (binFreq < lowProtectHz)
        {
            float protectFactor = binFreq / std::max(lowProtectHz, 1.0f);
            protectFactor = protectFactor * protectFactor; // smooth curve
            ratio = 1.0f - protectFactor * (1.0f - ratio);
        }

        // Envelope smoothing (attack/release)
        float coeff = (ratio < envelopeState[i]) ? attackCoeff : releaseCoeff;
        envelopeState[i] += coeff * (ratio - envelopeState[i]);

        gainReduction[i] = envelopeState[i];
    }

    // Frequency smoothing (simple box blur)
    if (freqSmooth > 0.01f)
        applySmoothing(gainReduction);
}

void SpectralMask::applySmoothing(float* gainReduction)
{
    int width = static_cast<int>(freqSmooth * 20.0f) + 1;
    width = std::min(width, numBins / 4);

    std::array<float, numBins> temp;
    for (int i = 0; i < numBins; ++i)
    {
        float sum = 0.0f;
        int count = 0;
        for (int j = std::max(0, i - width); j <= std::min(numBins - 1, i + width); ++j)
        {
            sum += gainReduction[j];
            ++count;
        }
        temp[i] = sum / static_cast<float>(count);
    }
    std::memcpy(gainReduction, temp.data(), sizeof(float) * numBins);
}
