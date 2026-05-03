#pragma once
#include <JuceHeader.h>
#include <vector>
#include <cmath>

struct SpectralMaskParams
{
    float amount = 1.0f;
    float focus = 0.5f;         // 0 = broad reduction, 1 = surgical/narrow
    float protectBody = 0.0f;   // 0 = no protection, 1 = full low-end protection
    float transients = 0.5f;    // 0 = no transient protection, 1 = full
    float attackCoeff = 0.2f;
    float releaseCoeff = 0.05f;
};

class SpectralMask
{
public:
    void prepare(int fftSizeIn, double sampleRate)
    {
        fftSize = fftSizeIn;
        sr = sampleRate;
        const int numBins = fftSizeIn;
        envelope.resize(numBins, 0.0f);
        gainReduction.resize(numBins, 0.0f);
        prevMainMag.resize(numBins / 2, 0.0f);
    }

    void reset()
    {
        std::fill(envelope.begin(), envelope.end(), 0.0f);
        std::fill(gainReduction.begin(), gainReduction.end(), 0.0f);
        std::fill(prevMainMag.begin(), prevMainMag.end(), 0.0f);
    }

    // Process and store gain reduction separately so we can compute delta
    void process(float* mainFFT, int size, const float* sideFFT, const SpectralMaskParams& p)
    {
        const int numBins = size / 2;
        const float bodyFreqLimit = 300.0f;
        const float bodyBinLimit = bodyFreqLimit * float(fftSize) / float(sr);

        // Detect transients: compare current energy to previous
        float currentEnergy = 0.0f;
        float prevEnergy = 0.0f;
        for (int bin = 0; bin < numBins; ++bin)
        {
            int i = bin * 2;
            float mag = std::sqrt(mainFFT[i] * mainFFT[i] + mainFFT[i + 1] * mainFFT[i + 1]);
            currentEnergy += mag;
            prevEnergy += prevMainMag[bin];
            prevMainMag[bin] = mag;
        }
        float transientFactor = 1.0f;
        if (prevEnergy > 1e-6f)
        {
            float ratio = currentEnergy / prevEnergy;
            if (ratio > 1.5f)
                transientFactor = 1.0f - p.transients * juce::jlimit(0.0f, 1.0f, (ratio - 1.5f) / 2.0f);
        }

        // Spectral blur width based on focus (low focus = more blur = broader)
        const int blurWidth = juce::jmax(0, (int)((1.0f - p.focus) * 10.0f));

        for (int bin = 0; bin < numBins; ++bin)
        {
            int i = bin * 2;
            float magM = std::sqrt(mainFFT[i] * mainFFT[i] + mainFFT[i + 1] * mainFFT[i + 1]);

            // Average sidechain magnitude over blur range for broader detection
            float magS = 0.0f;
            int count = 0;
            for (int b = juce::jmax(0, bin - blurWidth); b <= juce::jmin(numBins - 1, bin + blurWidth); ++b)
            {
                int j = b * 2;
                magS += std::sqrt(sideFFT[j] * sideFFT[j] + sideFFT[j + 1] * sideFFT[j + 1]);
                count++;
            }
            magS /= float(count);

            float target = magS / (magM + 1e-6f);
            target = juce::jlimit(0.0f, 1.0f, target * p.amount);

            // Protect body: reduce processing below bodyFreqLimit
            if (p.protectBody > 0.0f && float(bin) < bodyBinLimit)
            {
                float bodyScale = 1.0f - p.protectBody * (1.0f - float(bin) / bodyBinLimit);
                target *= bodyScale;
            }

            // Apply transient protection
            target *= transientFactor;

            // Envelope follower
            if (target > envelope[i])
                envelope[i] += p.attackCoeff * (target - envelope[i]);
            else
                envelope[i] += p.releaseCoeff * (target - envelope[i]);

            float gain = 1.0f - envelope[i];
            gainReduction[i] = envelope[i];
            gainReduction[i + 1] = envelope[i];

            mainFFT[i] *= gain;
            mainFFT[i + 1] *= gain;
        }
    }

    const std::vector<float>& getGainReduction() const { return gainReduction; }

private:
    int fftSize = 0;
    double sr = 44100.0;
    std::vector<float> envelope;
    std::vector<float> gainReduction;
    std::vector<float> prevMainMag;
};
