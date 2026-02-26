#pragma once

#if GAMBLER_HAS_SYNTH

#include <array>
#include <cmath>
#include <vector>
#include <algorithm>

/**
 * High-quality 8-channel Feedback Delay Network reverb.
 *
 * Signal flow:
 *   Input -> PreDelay -> Upmix to 8ch -> 4-stage PreDiffusion -> FDN Loop -> Stereo Downmix -> Haas -> Dry/Wet
 *
 * Design:
 *   - 8-line Hadamard FDN (in-place FWHT, no matrix storage)
 *   - 4-stage pre-diffusion (delay + polarity flip + Hadamard per stage)
 *   - Per-line modulated delay with linear interpolation
 *   - Per-line one-pole LP + HP damping in feedback path
 *   - Per-line RT60-derived decay gain
 *   - DC blocker in feedback
 *   - Pre-delay up to 500ms
 *   - Stereo Haas offset up to 200ms
 *   - Low-cut and high-cut output filters
 */
class FDNReverb
{
public:
    static constexpr int NUM_LINES = 8;
    static constexpr int NUM_DIFFUSION_STAGES = 4;

    FDNReverb() = default;

    void prepare (double sr, int /*blockSize*/)
    {
        sampleRate = sr;

        // Pre-delay: up to 500ms
        int maxPreDelaySamples = static_cast<int> (sr * 0.5) + 1;
        preDelayBufferL.assign (static_cast<size_t> (maxPreDelaySamples), 0.0f);
        preDelayBufferR.assign (static_cast<size_t> (maxPreDelaySamples), 0.0f);
        preDelayWriteIdx = 0;

        // Haas stereo offset: up to 200ms
        int maxHaasSamples = static_cast<int> (sr * 0.2) + 1;
        haasBuffer.assign (static_cast<size_t> (maxHaasSamples), 0.0f);
        haasWriteIdx = 0;

        // FDN delay lines — prime lengths scaled to sample rate
        static constexpr int kBaseDelays[NUM_LINES] = {
            1087, 1283, 1447, 1597, 1777, 1933, 2131, 2341
        };

        float srRatio = static_cast<float> (sr / 44100.0);
        for (int i = 0; i < NUM_LINES; ++i)
        {
            // Max buffer = base * 4.0 (largest size) * srRatio + modulation headroom
            int maxLen = static_cast<int> (kBaseDelays[i] * 4.0f * srRatio) + 64;
            fdnDelayBuffers[i].assign (static_cast<size_t> (maxLen), 0.0f);
            fdnWriteIdx[i] = 0;
            fdnBaseDelay[i] = static_cast<int> (kBaseDelays[i] * srRatio);
            fdnState[i] = 0.0f;

            // Per-line LFO — decorrelated rates and phases
            lfoPhase[i] = static_cast<float> (i) / static_cast<float> (NUM_LINES);
            lfoRate[i] = 0.7f + (static_cast<float> (i) * 0.25f); // 0.7 to 2.45 Hz

            // Damping filter state
            lpState[i] = 0.0f;
            hpState[i] = 0.0f;

            // DC blocker
            dcX1[i] = 0.0f;
            dcY1[i] = 0.0f;
        }

        // Pre-diffusion delay lines — 4 stages with halving delay ranges
        // Stage 0: ~40-80ms, Stage 1: ~20-40ms, Stage 2: ~10-20ms, Stage 3: ~5-10ms
        static constexpr float kDiffusionMaxMs[NUM_DIFFUSION_STAGES] = { 80.0f, 40.0f, 20.0f, 10.0f };
        for (int stage = 0; stage < NUM_DIFFUSION_STAGES; ++stage)
        {
            int maxSamples = static_cast<int> (kDiffusionMaxMs[stage] * 0.001f * sr) + 1;
            for (int ch = 0; ch < NUM_LINES; ++ch)
            {
                diffDelayBuffers[stage][ch].assign (static_cast<size_t> (maxSamples), 0.0f);
                diffWriteIdx[stage][ch] = 0;

                // Distribute delays across the range
                float slotSize = kDiffusionMaxMs[stage] / static_cast<float> (NUM_LINES);
                float delayMs = (static_cast<float> (ch) + 0.5f) * slotSize;
                diffDelaySamples[stage][ch] = static_cast<int> (delayMs * 0.001f * sr);
                diffDelaySamples[stage][ch] = std::max (1, diffDelaySamples[stage][ch]);
            }
        }

        // Output tone filters
        outLpStateL = outLpStateR = 0.0f;
        outHpStateL = outHpStateR = 0.0f;
    }

    struct Params
    {
        float decay = 2.0f;          // RT60 in seconds (0.2 - 30.0)
        float size = 1.0f;           // Room size multiplier (0.3 - 3.0)
        float preDelayMs = 10.0f;    // Pre-delay in ms (0 - 500)
        float lowCutHz = 100.0f;     // Output low-cut frequency (20 - 500)
        float highCutHz = 12000.0f;  // Output high-cut frequency (1000 - 20000)
        float lowDampHz = 200.0f;    // Feedback low-damp frequency (20 - 1000)
        float highDampHz = 6000.0f;  // Feedback high-damp frequency (1000 - 16000)
        float stereoMs = 0.0f;      // Haas stereo offset in ms (0 - 200)
        float dryWet = 0.3f;         // Mix (0.0 - 1.0)
    };

    void setParams (const Params& p)
    {
        params = p;
    }

    void processBlock (float* leftChannel, float* rightChannel, int numSamples)
    {
        // Precompute filter coefficients
        float lpCoeffFeedback = calcOnePoleCoeff (params.highDampHz);
        float hpCoeffFeedback = calcOnePoleCoeff (params.lowDampHz);
        float lpCoeffOut = calcOnePoleCoeff (params.highCutHz);
        float hpCoeffOut = calcOnePoleCoeff (params.lowCutHz);

        // Precompute per-line decay gains
        float decayGains[NUM_LINES];
        for (int i = 0; i < NUM_LINES; ++i)
        {
            int effectiveDelay = static_cast<int> (fdnBaseDelay[i] * params.size);
            effectiveDelay = std::max (1, effectiveDelay);
            decayGains[i] = calcDecayGain (params.decay, effectiveDelay);
        }

        int preDelaySamples = static_cast<int> (params.preDelayMs * 0.001f * static_cast<float> (sampleRate));
        preDelaySamples = std::min (preDelaySamples, static_cast<int> (preDelayBufferL.size()) - 1);
        preDelaySamples = std::max (0, preDelaySamples);

        int haasSamples = static_cast<int> (params.stereoMs * 0.001f * static_cast<float> (sampleRate));
        haasSamples = std::min (haasSamples, static_cast<int> (haasBuffer.size()) - 1);
        haasSamples = std::max (0, haasSamples);

        float modDepth = 8.0f; // samples of modulation depth

        for (int s = 0; s < numSamples; ++s)
        {
            float dryL = leftChannel[s];
            float dryR = rightChannel[s];

            // Sum to mono for reverb input
            float monoIn = (dryL + dryR) * 0.5f;

            // --- Pre-delay ---
            float preDelayedL = readDelayBuffer (preDelayBufferL, preDelayWriteIdx, preDelaySamples);
            float preDelayedR = readDelayBuffer (preDelayBufferR, preDelayWriteIdx, preDelaySamples);
            writeDelayBuffer (preDelayBufferL, preDelayWriteIdx, dryL);
            writeDelayBuffer (preDelayBufferR, preDelayWriteIdx, dryR);
            preDelayWriteIdx = (preDelayWriteIdx + 1) % static_cast<int> (preDelayBufferL.size());

            float preDelayedMono = (preDelayedL + preDelayedR) * 0.5f;

            // --- Upmix to 8 channels ---
            float channels[NUM_LINES];
            for (int i = 0; i < NUM_LINES; ++i)
                channels[i] = preDelayedMono;

            // --- Pre-diffusion (4 stages) ---
            for (int stage = 0; stage < NUM_DIFFUSION_STAGES; ++stage)
            {
                // Per-channel delay
                for (int ch = 0; ch < NUM_LINES; ++ch)
                {
                    float delayed = readDelayBuffer (diffDelayBuffers[stage][ch],
                                                     diffWriteIdx[stage][ch],
                                                     diffDelaySamples[stage][ch]);
                    writeDelayBuffer (diffDelayBuffers[stage][ch],
                                      diffWriteIdx[stage][ch],
                                      channels[ch]);
                    diffWriteIdx[stage][ch] = (diffWriteIdx[stage][ch] + 1) % static_cast<int> (diffDelayBuffers[stage][ch].size());
                    channels[ch] = delayed;
                }

                // Flip polarity on odd channels (decorrelation)
                for (int ch = 1; ch < NUM_LINES; ch += 2)
                    channels[ch] = -channels[ch];

                // Hadamard mix
                hadamardInPlace (channels);
            }

            // --- Add pre-diffused input to FDN state ---
            for (int i = 0; i < NUM_LINES; ++i)
                fdnState[i] += channels[i];

            // --- Read from FDN modulated delay lines ---
            float fdnOut[NUM_LINES];
            for (int i = 0; i < NUM_LINES; ++i)
            {
                int effectiveDelay = static_cast<int> (fdnBaseDelay[i] * params.size);
                effectiveDelay = std::max (1, effectiveDelay);

                // LFO modulation
                float lfoVal = std::sin (lfoPhase[i] * 6.2831853f);
                float modulatedDelay = static_cast<float> (effectiveDelay) + lfoVal * modDepth;
                modulatedDelay = std::max (1.0f, modulatedDelay);

                // Write current state into delay line
                writeDelayBuffer (fdnDelayBuffers[i], fdnWriteIdx[i], fdnState[i]);

                // Read with linear interpolation
                fdnOut[i] = readDelayBufferInterp (fdnDelayBuffers[i], fdnWriteIdx[i], modulatedDelay);

                fdnWriteIdx[i] = (fdnWriteIdx[i] + 1) % static_cast<int> (fdnDelayBuffers[i].size());

                // Advance LFO
                lfoPhase[i] += lfoRate[i] / static_cast<float> (sampleRate);
                if (lfoPhase[i] >= 1.0f)
                    lfoPhase[i] -= 1.0f;
            }

            // --- Feedback processing: damping + decay + DC block ---
            for (int i = 0; i < NUM_LINES; ++i)
            {
                float x = fdnOut[i];

                // One-pole lowpass (high-frequency damping)
                float lpA0 = 1.0f - lpCoeffFeedback;
                lpState[i] = lpA0 * x + lpCoeffFeedback * lpState[i];
                x = lpState[i];

                // One-pole highpass (low-frequency damping)
                float hpA0 = 1.0f - hpCoeffFeedback;
                hpState[i] = hpA0 * x + hpCoeffFeedback * hpState[i];
                x = x - hpState[i];

                // Apply decay gain
                x *= decayGains[i];

                // DC blocker
                float dcOut = x - dcX1[i] + 0.995f * dcY1[i];
                dcX1[i] = x;
                dcY1[i] = dcOut;
                x = dcOut;

                fdnOut[i] = x;
            }

            // --- Hadamard feedback mixing ---
            hadamardInPlace (fdnOut);

            // --- Write feedback back to FDN state ---
            for (int i = 0; i < NUM_LINES; ++i)
                fdnState[i] = fdnOut[i];

            // --- Stereo downmix (alternate channels L/R) ---
            float wetL = 0.0f, wetR = 0.0f;
            for (int i = 0; i < NUM_LINES; i += 2)
            {
                wetL += fdnOut[i];
                wetR += fdnOut[i + 1];
            }
            wetL *= 0.25f; // 4 channels summed
            wetR *= 0.25f;

            // --- Output tone filters ---
            // Low-cut (highpass)
            float outHpA0 = 1.0f - hpCoeffOut;
            outHpStateL = outHpA0 * wetL + hpCoeffOut * outHpStateL;
            wetL = wetL - outHpStateL;
            outHpStateR = outHpA0 * wetR + hpCoeffOut * outHpStateR;
            wetR = wetR - outHpStateR;

            // High-cut (lowpass)
            float outLpA0 = 1.0f - lpCoeffOut;
            outLpStateL = outLpA0 * wetL + lpCoeffOut * outLpStateL;
            wetL = outLpStateL;
            outLpStateR = outLpA0 * wetR + lpCoeffOut * outLpStateR;
            wetR = outLpStateR;

            // --- Haas stereo offset (delay left channel) ---
            if (haasSamples > 0)
            {
                float haasDelayed = readDelayBuffer (haasBuffer, haasWriteIdx, haasSamples);
                writeDelayBuffer (haasBuffer, haasWriteIdx, wetL);
                haasWriteIdx = (haasWriteIdx + 1) % static_cast<int> (haasBuffer.size());
                wetL = haasDelayed;
            }

            // --- Dry/Wet mix ---
            // Use pre-delayed dry signal for natural pre-delay behavior
            float wet = params.dryWet;
            float dry = 1.0f - wet;
            leftChannel[s] = dryL * dry + wetL * wet;
            rightChannel[s] = dryR * dry + wetR * wet;
        }
    }

private:
    double sampleRate = 44100.0;
    Params params;

    // --- Pre-delay ---
    std::vector<float> preDelayBufferL, preDelayBufferR;
    int preDelayWriteIdx = 0;

    // --- Haas stereo offset ---
    std::vector<float> haasBuffer;
    int haasWriteIdx = 0;

    // --- FDN delay lines ---
    std::array<std::vector<float>, NUM_LINES> fdnDelayBuffers;
    std::array<int, NUM_LINES> fdnWriteIdx {};
    std::array<int, NUM_LINES> fdnBaseDelay {};
    std::array<float, NUM_LINES> fdnState {};

    // --- Per-line LFO ---
    std::array<float, NUM_LINES> lfoPhase {};
    std::array<float, NUM_LINES> lfoRate {};

    // --- Per-line damping ---
    std::array<float, NUM_LINES> lpState {};
    std::array<float, NUM_LINES> hpState {};

    // --- Per-line DC blocker ---
    std::array<float, NUM_LINES> dcX1 {};
    std::array<float, NUM_LINES> dcY1 {};

    // --- Pre-diffusion ---
    std::array<std::array<std::vector<float>, NUM_LINES>, NUM_DIFFUSION_STAGES> diffDelayBuffers;
    std::array<std::array<int, NUM_LINES>, NUM_DIFFUSION_STAGES> diffWriteIdx {};
    std::array<std::array<int, NUM_LINES>, NUM_DIFFUSION_STAGES> diffDelaySamples {};

    // --- Output tone filters ---
    float outLpStateL = 0.0f, outLpStateR = 0.0f;
    float outHpStateL = 0.0f, outHpStateR = 0.0f;

    // ============================
    // DSP helpers
    // ============================

    static void hadamardInPlace (float* data)
    {
        // In-place 8-point Hadamard via butterfly (FWHT)
        for (int blockSize = 1; blockSize < NUM_LINES; blockSize *= 2)
        {
            for (int i = 0; i < NUM_LINES; i += blockSize * 2)
            {
                for (int j = 0; j < blockSize; ++j)
                {
                    float a = data[i + j];
                    float b = data[i + j + blockSize];
                    data[i + j] = a + b;
                    data[i + j + blockSize] = a - b;
                }
            }
        }
        // Normalize for energy preservation
        static constexpr float kScale = 1.0f / 2.828427f; // 1/sqrt(8)
        for (int i = 0; i < NUM_LINES; ++i)
            data[i] *= kScale;
    }

    float calcOnePoleCoeff (float freqHz) const
    {
        return std::exp (-6.2831853f * freqHz / static_cast<float> (sampleRate));
    }

    float calcDecayGain (float rt60Seconds, int delaySamples) const
    {
        if (rt60Seconds <= 0.0f)
            return 0.0f;
        float loopsPerRt60 = (rt60Seconds * static_cast<float> (sampleRate)) / static_cast<float> (delaySamples);
        float dbPerLoop = -60.0f / loopsPerRt60;
        return std::pow (10.0f, dbPerLoop * 0.05f);
    }

    static float readDelayBuffer (const std::vector<float>& buf, int writeIdx, int delaySamples)
    {
        int readIdx = writeIdx - delaySamples;
        if (readIdx < 0)
            readIdx += static_cast<int> (buf.size());
        return buf[static_cast<size_t> (readIdx)];
    }

    static float readDelayBufferInterp (const std::vector<float>& buf, int writeIdx, float delaySamples)
    {
        float readPos = static_cast<float> (writeIdx) - delaySamples;
        int bufSize = static_cast<int> (buf.size());
        while (readPos < 0.0f)
            readPos += static_cast<float> (bufSize);

        int idx0 = static_cast<int> (readPos) % bufSize;
        int idx1 = (idx0 + 1) % bufSize;
        float frac = readPos - std::floor (readPos);

        return buf[static_cast<size_t> (idx0)] * (1.0f - frac) + buf[static_cast<size_t> (idx1)] * frac;
    }

    static void writeDelayBuffer (std::vector<float>& buf, int writeIdx, float value)
    {
        buf[static_cast<size_t> (writeIdx)] = value;
    }
};

#endif // GAMBLER_HAS_SYNTH
