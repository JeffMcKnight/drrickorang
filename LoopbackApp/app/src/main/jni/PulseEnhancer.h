//
// Created by JeffMcKnight on 11/16/16.
//

#ifndef LOOPBACKAPP_PULSE_ENHANCER_H
#define LOOPBACKAPP_PULSE_ENHANCER_H


// FILTER_FREQUENCY_HZ should match Constant.LOOPBACK_FREQUENCY
static const float FILTER_FREQUENCY_HZ = 19.0f*1000.0f;
// FILTER_RESONANCE chosen to make feedback fairly stable
static const float FILTER_RESONANCE = 1.0f;

#include <SLES/OpenSLES.h>
#include <SuperpoweredFilter.h>
#include <SuperpoweredSimple.h>

class PulseEnhancer {
private:
    SuperpoweredFilter *filter;
    void enhancePulse(float *stereoBuffer, SLuint32 bufSizeInFrames, float *volume);
    char *stereoFloatToMonoShortInt(float *stereoBuffer,
                                    SLuint32 bufSizeInFrames,
                                    SLuint32 channelCount);
    float *monoShortIntToStereoFloat(const char *monoShortIntBuffer,
                                     SLuint32 bufSizeInFrames,
                                     SLuint32 channelCount);
public:
    SuperpoweredFilter * createFilter(int samplingRate);
    char *processForOpenAir(SLuint32 bufSizeInFrames,
                            SLuint32 channelCount,
                            float *volume,
                            char *recordBuffer);
};



#endif //LOOPBACKAPP_PULSE_ENHANCER_H
