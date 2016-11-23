//
// Created by JeffMcKnight on 11/16/16.
//

#ifndef LOOPBACKAPP_PULSE_ENHANCER_H
#define LOOPBACKAPP_PULSE_ENHANCER_H

static const int WARMUP_PERIOD_MSEC = 20;

#include <SLES/OpenSLES.h>
#include <SuperpoweredAudioBuffers.h>
#include <SuperpoweredFilter.h>
#include <SuperpoweredLimiter.h>
#include <SuperpoweredSimple.h>
#include "NoiseGate.h"


class PulseEnhancer {
public:
    static PulseEnhancer *create(int samplingRate, int injectedFrequencyHz);
    void processForOpenAir(char *inputBuffer, char *outputBuffer, SLuint32 bufSizeInFrames,
                               SLuint32 channelCount);

private:
    PulseEnhancer(int i, int injectedFrequencyHz);

    const float FILTER_FREQUENCY_HZ;
    const float FILTER_RESONANCE;
    const float TARGET_PEAK;
    const unsigned int SAMPLES_MONO_ONE_MSEC;
    const unsigned int SAMPLES_STEREO_ONE_MSEC;
    const int SAMPLE_RATE;
    int mTimeElapsed;
    float mVolume;
    NoiseGate *mNoiseGate;
    SuperpoweredFilter *mFilter;
    SuperpoweredLimiter *mLimiter;

    void createFilter(unsigned int samplingRate);
    void createLimiter(unsigned int samplingRate);
    void enhancePulse(float *stereoBuffer, SLuint32 bufSizeInFrames);
    void stereoFloatToMonoShortInt(float *stereoInputBuffer, char *monoOutputBuffer,
                                       SLuint32 bufSizeInFrames, SLuint32 channelCount);
    void monoShortIntToStereoFloat(const char *monoShortIntBuffer,
                                   SLuint32 bufSizeInFrames, SLuint32 channelCount,
                                   float *stereoFloatBuffer);
    void predictLimiter(float *buffer, SLuint32 frames);
    void locatePulse(float *rawBuffer, float *filteredBuffer, int bufSizeInFrames);

    int bufferSizeInMsec(int bufSizeInFrames) const;

    void mute(float *audioBuffer, unsigned int numberOfSamples);
};


#endif //LOOPBACKAPP_PULSE_ENHANCER_H
