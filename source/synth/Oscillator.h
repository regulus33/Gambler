#pragma once

#if GAMBLER_HAS_SYNTH

#include <cmath>

/**
 * Vector-blend oscillator with PolyBLEP antialiasing.
 * Single phase accumulator generates pulse, saw, and sine.
 * Blend parameter crossfades: 0.0=pulse, 0.5=saw, 1.0=sine.
 */
class VectorOscillator
{
public:
    VectorOscillator() = default;

    void setFrequency (double hz, double sampleRate)
    {
        phaseIncrement = hz / sampleRate;
    }

    void setBlend (float b)
    {
        blend = b;
    }

    void reset()
    {
        phase = 0.0;
    }

    float getNextSample()
    {
        // Generate raw waveforms from shared phase
        float pulse = generatePulse();
        float saw = generateSaw();
        float sine = generateSine();

        // Advance phase
        phase += phaseIncrement;
        if (phase >= 1.0)
            phase -= 1.0;

        // Piecewise linear crossfade between adjacent pairs
        if (blend <= 0.5f)
        {
            // 0.0 = pure pulse, 0.5 = pure saw
            float t = blend * 2.0f;
            return pulse * (1.0f - t) + saw * t;
        }
        else
        {
            // 0.5 = pure saw, 1.0 = pure sine
            float t = (blend - 0.5f) * 2.0f;
            return saw * (1.0f - t) + sine * t;
        }
    }

private:
    double phase = 0.0;
    double phaseIncrement = 0.0;
    float blend = 0.5f; // default to saw

    // PolyBLEP correction for discontinuities
    static double polyBlep (double t, double dt)
    {
        if (t < dt)
        {
            t /= dt;
            return t + t - t * t - 1.0;
        }
        else if (t > 1.0 - dt)
        {
            t = (t - 1.0) / dt;
            return t * t + t + t + 1.0;
        }
        return 0.0;
    }

    float generatePulse() const
    {
        double dt = phaseIncrement;
        double raw = (phase < 0.5) ? 1.0 : -1.0;
        raw += polyBlep (phase, dt);
        raw -= polyBlep (std::fmod (phase + 0.5, 1.0), dt);
        return static_cast<float> (raw);
    }

    float generateSaw() const
    {
        double dt = phaseIncrement;
        double raw = 2.0 * phase - 1.0;
        raw -= polyBlep (phase, dt);
        return static_cast<float> (raw);
    }

    float generateSine() const
    {
        return static_cast<float> (std::sin (2.0 * M_PI * phase));
    }
};

#endif // GAMBLER_HAS_SYNTH
