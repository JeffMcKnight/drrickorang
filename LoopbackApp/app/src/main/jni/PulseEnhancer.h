//
// Created by JeffMcKnight on 11/16/16.
//

#ifndef LOOPBACKAPP_PULSE_ENHANCER_H
#define LOOPBACKAPP_PULSE_ENHANCER_H

#include <SLES/OpenSLES.h>
#include <SuperpoweredFilter.h>
#include <SuperpoweredSimple.h>


class PulseEnhancer {
public:
    static PulseEnhancer *create(int samplingRate);
    char *processForOpenAir(SLuint32 bufSizeInFrames, SLuint32 channelCount, char *recordBuffer);
private:
    PulseEnhancer();
    const float FILTER_FREQUENCY_HZ;
    const float FILTER_RESONANCE;
    const float TARGET_PEAK;
    SuperpoweredFilter *filter;
    SLuint32 bufSizeInFrames;
    SLuint32 channelCount;
    float volume;
    void createFilter(unsigned int samplingRate);
    void enhancePulse(float *stereoBuffer, SLuint32 bufSizeInFrames);
    char *stereoFloatToMonoShortInt(float *stereoBuffer,
                                    SLuint32 bufSizeInFrames,
                                    SLuint32 channelCount);
    float *monoShortIntToStereoFloat(const char *monoShortIntBuffer,
                                     SLuint32 bufSizeInFrames,
                                     SLuint32 channelCount);
};



#endif //LOOPBACKAPP_PULSE_ENHANCER_H
