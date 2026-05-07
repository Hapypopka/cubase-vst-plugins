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

        g.setColour(juce::Colour(0xff0d0d1a));
        g.fillRoundedRectangle(bounds, 4.0f);

        auto plotArea = bounds.reduced(40, 12);

        drawGrid(g, bounds, plotArea);

        SpaceCarverAudioProcessor::SpectrumData data;
        {
            const juce::SpinLock::ScopedLockType lock(processor.spectrumLock);
            data = processor.spectrumForDisplay;
        }

        // Reduction fill (below 0, red)
        drawFill(g, plotArea, data.reductionDb, juce::Colour(0x50ff3333), true);

        // Sidechain fill (above 0, orange)
        drawFill(g, plotArea, data.sideSpectrum, juce::Colour(0x40ff8c00), false);

        // Reduction curve (below 0)
        drawSmoothCurve(g, plotArea, data.reductionDb, juce::Colour(0xffff4444), 2.0f);

        // Sidechain curve (above 0)
        drawSmoothCurve(g, plotArea, data.sideSpectrum, juce::Colour(0xffff8c00), 1.5f);

        // 0 dB reference line (on top)
        float zeroY = plotArea.getY() + dbToY(0.0f, plotArea.getHeight());
        g.setColour(juce::Colour(0xff5577aa));
        g.drawHorizontalLine((int)zeroY, plotArea.getX(), plotArea.getRight());

        drawLegend(g, bounds);
    }

    void resized() override {}

private:
    SpaceCarverAudioProcessor& processor;

    // Range: -24 (bottom, max reduction) to +24 (top, max sidechain)
    static constexpr float minDb = -24.0f;
    static constexpr float maxDb = 24.0f;
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

    float getInterpolatedDb(const std::array<float, SpaceCarverAudioProcessor::numDisplayBins>& spectrum, float freq) const
    {
        float sr = (float)processor.getAnalysisSampleRate();
        int fftSz = processor.getFFTSize();
        float binFloat = freq * float(fftSz) / sr;
        int bin0 = (int)binFloat;
        float frac = binFloat - float(bin0);

        const int maxBin = SpaceCarverAudioProcessor::numDisplayBins - 1;
        bin0 = juce::jlimit(0, maxBin, bin0);
        int bin1 = juce::jmin(bin0 + 1, maxBin);

        return spectrum[bin0] + frac * (spectrum[bin1] - spectrum[bin0]);
    }

    void drawSmoothCurve(juce::Graphics& g, juce::Rectangle<float> plotArea,
                          const std::array<float, SpaceCarverAudioProcessor::numDisplayBins>& spectrum,
                          juce::Colour colour, float thickness)
    {
        const int numPoints = 200;
        std::vector<juce::Point<float>> points;
        points.reserve(numPoints);

        for (int i = 0; i < numPoints; ++i)
        {
            float t = float(i) / float(numPoints - 1);
            float freq = minFreq * std::pow(maxFreq / minFreq, t);
            float db = juce::jlimit(minDb, maxDb, getInterpolatedDb(spectrum, freq));

            float x = plotArea.getX() + freqToX(freq, plotArea.getWidth());
            float y = plotArea.getY() + dbToY(db, plotArea.getHeight());
            points.push_back({x, y});
        }

        if (points.empty()) return;

        juce::Path path;
        path.startNewSubPath(points[0]);
        for (size_t i = 1; i < points.size() - 1; ++i)
        {
            float midX = (points[i].x + points[i + 1].x) * 0.5f;
            float midY = (points[i].y + points[i + 1].y) * 0.5f;
            path.quadraticTo(points[i].x, points[i].y, midX, midY);
        }
        if (points.size() >= 2)
            path.lineTo(points.back());

        g.setColour(colour);
        g.strokePath(path, juce::PathStrokeType(thickness, juce::PathStrokeType::curved));
    }

    void drawFill(juce::Graphics& g, juce::Rectangle<float> plotArea,
                  const std::array<float, SpaceCarverAudioProcessor::numDisplayBins>& spectrum,
                  juce::Colour colour, bool fillDown)
    {
        const int numPoints = 200;
        float zeroY = plotArea.getY() + dbToY(0.0f, plotArea.getHeight());

        juce::Path fillPath;
        fillPath.startNewSubPath(plotArea.getX(), zeroY);

        for (int i = 0; i < numPoints; ++i)
        {
            float t = float(i) / float(numPoints - 1);
            float freq = minFreq * std::pow(maxFreq / minFreq, t);
            float db = juce::jlimit(minDb, maxDb, getInterpolatedDb(spectrum, freq));

            float x = plotArea.getX() + freqToX(freq, plotArea.getWidth());
            float y = plotArea.getY() + dbToY(db, plotArea.getHeight());
            fillPath.lineTo(x, y);
        }

        fillPath.lineTo(plotArea.getRight(), zeroY);
        fillPath.closeSubPath();

        g.setColour(colour);
        g.fillPath(fillPath);
    }

    void drawGrid(juce::Graphics& g, juce::Rectangle<float> bounds, juce::Rectangle<float> plotArea)
    {
        g.setColour(juce::Colour(0xff222240));
        g.setFont(juce::Font(9.0f));

        const float freqs[] = { 50, 100, 200, 500, 1000, 2000, 5000, 10000, 20000 };
        for (float f : freqs)
        {
            float x = plotArea.getX() + freqToX(f, plotArea.getWidth());
            g.drawVerticalLine((int)x, plotArea.getY(), plotArea.getBottom());

            g.setColour(juce::Colour(0xff555577));
            juce::String label = (f >= 1000) ? juce::String((int)(f / 1000.0f)) + "k" : juce::String((int)f);
            g.drawText(label, (int)x - 15, (int)plotArea.getBottom() + 1, 30, 10, juce::Justification::centred);
            g.setColour(juce::Colour(0xff222240));
        }

        for (float db = minDb; db <= maxDb; db += 6.0f)
        {
            float y = plotArea.getY() + dbToY(db, plotArea.getHeight());

            if (std::abs(db) < 0.1f)
                g.setColour(juce::Colour(0xff5577aa));
            else
                g.setColour(juce::Colour(0xff222240));

            g.drawHorizontalLine((int)y, plotArea.getX(), plotArea.getRight());

            if ((int)db % 12 == 0 || std::abs(db) < 0.1f)
            {
                g.setColour(juce::Colour(0xff555577));
                g.drawText(juce::String((int)db), (int)plotArea.getX() - 38, (int)y - 5, 34, 10,
                           juce::Justification::centredRight);
            }
        }
    }

    void drawLegend(juce::Graphics& g, juce::Rectangle<float> bounds)
    {
        g.setFont(juce::Font(10.0f));
        int x = (int)bounds.getX() + 48;
        int y = (int)bounds.getY() + 6;

        g.setColour(juce::Colour(0xffff8c00));
        g.fillRect(x, y + 2, 12, 4);
        g.drawText("SIDECHAIN", x + 16, y, 80, 10, juce::Justification::centredLeft);

        g.setColour(juce::Colour(0xffff4444));
        g.fillRect(x + 100, y + 2, 12, 4);
        g.drawText("REDUCTION", x + 116, y, 80, 10, juce::Justification::centredLeft);
    }

    void timerCallback() override
    {
        repaint();
    }
};
