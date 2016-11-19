//
// Created by JeffMcKnight on 11/18/16.
//

#ifndef LOOPBACKAPP_NOISEGATE_H
#define LOOPBACKAPP_NOISEGATE_H

#include <queue>


class NoiseGate {

public:
    NoiseGate(int sampleRate,
              int thresholdWindowMsec,
              float thresholdFactor,
              int subframeSizeMsec);
    void process(float *stereoBuffer, unsigned int bufferSizeInFrames, unsigned int i);

private:
    const unsigned int SAMPLES_STEREO_ONE_MSEC;
    const int SAMPLE_RATE;
    const int SUBFRAME_SIZE_MSEC;
    const unsigned int SUBFRAME_SIZE_SAMPLES;
    const int THRESHOLD_WINDOW_MSEC;
    const float THRESHOLD_FACTOR;

    bool mIsGateOpen;
    float mNoiseFloor;

    std::queue<float> mPeakQueue;

    void processSubframe(float *subFrame, unsigned int subframeSizeSamples);
    void updateNoiseFloor(float peak);
    void openGate(float *buffer);
    void closeGate(float *buffer);
    void mute(float *buffer);
    void changeVolume(float *buffer, float volumeStart, float volumeEnd);
    float threshold();
    int bufferSizeInMsec(int frames);
    unsigned int maxSubframes() ;
    int subframeSizeSamples(unsigned int count);
};


#endif //LOOPBACKAPP_NOISEGATE_H
