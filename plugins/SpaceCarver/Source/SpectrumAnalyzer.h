#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class SpectrumAnalyzer : public juce::Component, private juce::Timer
{
public:
    SpectrumAnalyzer(SpaceCarverAudioProcessor& p) : processor(p)
    {
        startTimerHz(30);
    }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();

        // Background
        g.setColour(juce::Colour(0xff0d0d1a));
        g.fillRoundedRectangle(bounds, 4.0f);

        // Grid
        drawGrid(g, bounds);

        // Get spectrum data
        SpaceCarverAudioProcessor::SpectrumData data;
        {
            const juce::SpinLock::ScopedLockType lock(processor.spectrumLock);
            data = processor.spectrumForDisplay;
        }

        auto plotArea = bounds.reduced(40, 12);

        // Draw reduction fill (red, semi-transparent)
        drawReductionFill(g, plotArea, data.reductionDb, data.mainSpectrum);

        // Draw sidechain curve (orange)
        drawCurve(g, plotArea, data.sideSpectrum, juce::Colour(0xffff8c00), 1.5f);

        // Draw main curve (blue)
        drawCurve(g, plotArea, data.mainSpectrum, juce::Colour(0xff4a9fff), 2.0f);

        // Legend
        drawLegend(g, bounds);
    }

    void resized() override {}

private:
    SpaceCarverAudioProcessor& processor;

    static constexpr float minDb = -48.0f;
    static constexpr float maxDb = 12.0f;
    static constexpr float minFreq = 20.0f;
    static constexpr float maxFreq = 20000.0f;

    float freqToX(float freq, float width) const
    {
        return width * (std::log(freq / minFreq) / std::log(maxFreq / minFreq));
    }

    float dbToY(float db, float height) const
    {
        return height * (1.0f - (db - minDb) / (maxDb - minDb));
    }

    int freqToBin(float freq) const
    {
        float sr = (float)processor.getAnalysisSampleRate();
        int fftSz = processor.getFFTSize();
        return juce::jlimit(0, SpaceCarverAudioProcessor::numDisplayBins - 1,
                           (int)(freq * float(fftSz) / sr));
    }

    float binToFreq(int bin) const
    {
        float sr = (float)processor.getAnalysisSampleRate();
        int fftSz = processor.getFFTSize();
        return float(bin) * sr / float(fftSz);
    }

    void drawGrid(juce::Graphics& g, juce::Rectangle<float> bounds)
    {
        auto plotArea = bounds.reduced(40, 12);

        g.setColour(juce::Colour(0xff222240));

        // Frequency lines
        const float freqs[] = { 50, 100, 200, 500, 1000, 2000, 5000, 10000, 20000 };
        g.setFont(juce::Font(9.0f));

        for (float f : freqs)
        {
            float x = plotArea.getX() + freqToX(f, plotArea.getWidth());
            g.drawVerticalLine((int)x, plotArea.getY(), plotArea.getBottom());

            g.setColour(juce::Colour(0xff555577));
            juce::String label;
            if (f >= 1000)
                label = juce::String(f / 1000.0f, (f >= 10000) ? 0 : 0) + "k";
            else
                label = juce::String((int)f);
            g.drawText(label, (int)x - 15, (int)plotArea.getBottom() + 1, 30, 10, juce::Justification::centred);
            g.setColour(juce::Colour(0xff222240));
        }

        // dB lines
        for (float db = minDb; db <= maxDb; db += 6.0f)
        {
            float y = plotArea.getY() + dbToY(db, plotArea.getHeight());
            g.drawHorizontalLine((int)y, plotArea.getX(), plotArea.getRight());

            if ((int)db % 12 == 0)
            {
                g.setColour(juce::Colour(0xff555577));
                g.drawText(juce::String((int)db), (int)plotArea.getX() - 38, (int)y - 5, 34, 10,
                           juce::Justification::centredRight);
                g.setColour(juce::Colour(0xff222240));
            }
        }
    }

    void drawCurve(juce::Graphics& g, juce::Rectangle<float> plotArea,
                   const std::array<float, SpaceCarverAudioProcessor::numDisplayBins>& spectrum,
                   juce::Colour colour, float thickness)
    {
        juce::Path path;
        bool started = false;

        for (int px = 0; px < (int)plotArea.getWidth(); ++px)
        {
            float freq = minFreq * std::pow(maxFreq / minFreq, float(px) / plotArea.getWidth());
            int bin = freqToBin(freq);
            if (bin < 1 || bin >= SpaceCarverAudioProcessor::numDisplayBins)
                continue;

            float db = spectrum[bin];
            db = juce::jlimit(minDb, maxDb, db);
            float y = plotArea.getY() + dbToY(db, plotArea.getHeight());
            float x = plotArea.getX() + float(px);

            if (!started)
            {
                path.startNewSubPath(x, y);
                started = true;
            }
            else
            {
                path.lineTo(x, y);
            }
        }

        g.setColour(colour);
        g.strokePath(path, juce::PathStrokeType(thickness));
    }

    void drawReductionFill(juce::Graphics& g, juce::Rectangle<float> plotArea,
                           const std::array<float, SpaceCarverAudioProcessor::numDisplayBins>& reduction,
                           const std::array<float, SpaceCarverAudioProcessor::numDisplayBins>& mainSpec)
    {
        juce::Path fillPath;
        juce::Path topPath;
        bool started = false;

        for (int px = 0; px < (int)plotArea.getWidth(); ++px)
        {
            float freq = minFreq * std::pow(maxFreq / minFreq, float(px) / plotArea.getWidth());
            int bin = freqToBin(freq);
            if (bin < 1 || bin >= SpaceCarverAudioProcessor::numDisplayBins)
                continue;

            float mainDb = juce::jlimit(minDb, maxDb, mainSpec[bin]);
            float redDb = reduction[bin];

            float yTop = plotArea.getY() + dbToY(mainDb, plotArea.getHeight());
            float yBottom = plotArea.getY() + dbToY(mainDb - redDb, plotArea.getHeight());
            float x = plotArea.getX() + float(px);

            if (!started)
            {
                fillPath.startNewSubPath(x, yTop);
                topPath.startNewSubPath(x, yBottom);
                started = true;
            }
            else
            {
                fillPath.lineTo(x, yTop);
                topPath.lineTo(x, yBottom);
            }
        }

        // Close fill path by going backwards along the bottom
        for (int px = (int)plotArea.getWidth() - 1; px >= 0; --px)
        {
            float freq = minFreq * std::pow(maxFreq / minFreq, float(px) / plotArea.getWidth());
            int bin = freqToBin(freq);
            if (bin < 1 || bin >= SpaceCarverAudioProcessor::numDisplayBins)
                continue;

            float mainDb = juce::jlimit(minDb, maxDb, mainSpec[bin]);
            float redDb = reduction[bin];
            float yBottom = plotArea.getY() + dbToY(mainDb - redDb, plotArea.getHeight());
            float x = plotArea.getX() + float(px);
            fillPath.lineTo(x, yBottom);
        }

        fillPath.closeSubPath();
        g.setColour(juce::Colour(0x40ff3333));
        g.fillPath(fillPath);
    }

    void drawLegend(juce::Graphics& g, juce::Rectangle<float> bounds)
    {
        g.setFont(juce::Font(10.0f));
        int x = (int)bounds.getX() + 48;
        int y = (int)bounds.getY() + 6;

        g.setColour(juce::Colour(0xff4a9fff));
        g.fillRect(x, y + 3, 12, 3);
        g.drawText("MAIN", x + 16, y, 40, 10, juce::Justification::centredLeft);

        g.setColour(juce::Colour(0xffff8c00));
        g.fillRect(x + 64, y + 3, 12, 3);
        g.drawText("SIDECHAIN", x + 80, y, 70, 10, juce::Justification::centredLeft);

        g.setColour(juce::Colour(0x80ff3333));
        g.fillRect(x + 156, y, 12, 8);
        g.setColour(juce::Colour(0xffff3333));
        g.drawText("REDUCTION", x + 172, y, 80, 10, juce::Justification::centredLeft);
    }

    void timerCallback() override
    {
        repaint();
    }
};
