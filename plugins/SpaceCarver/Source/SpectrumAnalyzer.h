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

        drawGrid(g, bounds);

        SpaceCarverAudioProcessor::SpectrumData data;
        {
            const juce::SpinLock::ScopedLockType lock(processor.spectrumLock);
            data = processor.spectrumForDisplay;
        }

        auto plotArea = bounds.reduced(40, 12);

        drawReductionFill(g, plotArea, data.reductionDb, data.mainSpectrum);
        drawCurve(g, plotArea, data.sideSpectrum, juce::Colour(0xffff8c00), 1.5f);
        drawCurve(g, plotArea, data.mainSpectrum, juce::Colour(0xff4a9fff), 2.0f);
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

    // Get interpolated dB value at fractional bin position
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

    // Build a smooth path using fewer control points and cubic interpolation
    juce::Path buildSmoothPath(juce::Rectangle<float> plotArea,
                                const std::array<float, SpaceCarverAudioProcessor::numDisplayBins>& spectrum) const
    {
        // Sample ~200 points on log scale, then use quadratic curves
        const int numPoints = 200;
        std::vector<juce::Point<float>> points;
        points.reserve(numPoints);

        for (int i = 0; i < numPoints; ++i)
        {
            float t = float(i) / float(numPoints - 1);
            float freq = minFreq * std::pow(maxFreq / minFreq, t);
            float db = getInterpolatedDb(spectrum, freq);
            db = juce::jlimit(minDb, maxDb, db);

            float x = plotArea.getX() + freqToX(freq, plotArea.getWidth());
            float y = plotArea.getY() + dbToY(db, plotArea.getHeight());
            points.push_back({x, y});
        }

        if (points.empty())
            return {};

        juce::Path path;
        path.startNewSubPath(points[0]);

        // Use quadratic bezier curves through midpoints for smoothness
        for (size_t i = 1; i < points.size() - 1; ++i)
        {
            float midX = (points[i].x + points[i + 1].x) * 0.5f;
            float midY = (points[i].y + points[i + 1].y) * 0.5f;
            path.quadraticTo(points[i].x, points[i].y, midX, midY);
        }

        if (points.size() >= 2)
            path.lineTo(points.back());

        return path;
    }

    void drawCurve(juce::Graphics& g, juce::Rectangle<float> plotArea,
                   const std::array<float, SpaceCarverAudioProcessor::numDisplayBins>& spectrum,
                   juce::Colour colour, float thickness)
    {
        auto path = buildSmoothPath(plotArea, spectrum);
        g.setColour(colour);
        g.strokePath(path, juce::PathStrokeType(thickness, juce::PathStrokeType::curved));
    }

    void drawReductionFill(juce::Graphics& g, juce::Rectangle<float> plotArea,
                           const std::array<float, SpaceCarverAudioProcessor::numDisplayBins>& reduction,
                           const std::array<float, SpaceCarverAudioProcessor::numDisplayBins>& mainSpec)
    {
        const int numPoints = 200;
        juce::Path fillPath;
        std::vector<juce::Point<float>> bottomPoints;
        bool started = false;

        for (int i = 0; i < numPoints; ++i)
        {
            float t = float(i) / float(numPoints - 1);
            float freq = minFreq * std::pow(maxFreq / minFreq, t);

            float mainDb = juce::jlimit(minDb, maxDb, getInterpolatedDb(mainSpec, freq));
            float redDb = getInterpolatedDb(reduction, freq);

            float x = plotArea.getX() + freqToX(freq, plotArea.getWidth());
            float yTop = plotArea.getY() + dbToY(mainDb, plotArea.getHeight());
            float yBottom = plotArea.getY() + dbToY(mainDb - redDb, plotArea.getHeight());

            if (!started)
            {
                fillPath.startNewSubPath(x, yTop);
                started = true;
            }
            else
            {
                fillPath.lineTo(x, yTop);
            }
            bottomPoints.push_back({x, yBottom});
        }

        // Close by going backwards along bottom
        for (int i = (int)bottomPoints.size() - 1; i >= 0; --i)
            fillPath.lineTo(bottomPoints[i]);

        fillPath.closeSubPath();
        g.setColour(juce::Colour(0x40ff3333));
        g.fillPath(fillPath);
    }

    void drawGrid(juce::Graphics& g, juce::Rectangle<float> bounds)
    {
        auto plotArea = bounds.reduced(40, 12);

        g.setColour(juce::Colour(0xff222240));

        const float freqs[] = { 50, 100, 200, 500, 1000, 2000, 5000, 10000, 20000 };
        g.setFont(juce::Font(9.0f));

        for (float f : freqs)
        {
            float x = plotArea.getX() + freqToX(f, plotArea.getWidth());
            g.drawVerticalLine((int)x, plotArea.getY(), plotArea.getBottom());

            g.setColour(juce::Colour(0xff555577));
            juce::String label;
            if (f >= 1000)
                label = juce::String((int)(f / 1000.0f)) + "k";
            else
                label = juce::String((int)f);
            g.drawText(label, (int)x - 15, (int)plotArea.getBottom() + 1, 30, 10, juce::Justification::centred);
            g.setColour(juce::Colour(0xff222240));
        }

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
