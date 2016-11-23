//
// Created by JeffMcKnight on 11/18/16.
//

#ifndef LOOPBACKAPP_NOISEGATE_H
#define LOOPBACKAPP_NOISEGATE_H

static const int SUBFRAME_COUNT = 5;

#include <queue>


class NoiseGate {

public:
    NoiseGate(int sampleRate,
              int thresholdWindowMsec,
              float thresholdFactor,
              int subframeSizeMsec);
    void process(float *stereoBuffer, unsigned int bufferSizeInFrames);
    bool isPulseDetected();

    bool isPeakDetected();

    int openGateCount();

    void processSubframe(float *subFrame, unsigned int subframeSizeSamples);

private:
    const unsigned int SAMPLES_STEREO_ONE_MSEC;
    const int SAMPLE_RATE;
    const int SUBFRAME_SIZE_MSEC;
    const unsigned int SUBFRAME_SIZE_SAMPLES;
    const int THRESHOLD_WINDOW_MSEC;
    const float THRESHOLD_FACTOR;

    /** Indicates whether the noise gate is open for the current buffer subframe */
    bool mIsGateOpen;
    /** Indicates if test pulse was detected in the current buffer frame being processed*/
    bool mIsPulseDetected;
    bool mIsPeakDetected;
    float mLastPeak;
    float mNoiseFloor;
    int mOpenGateCount;
    std::queue<float> mPeakQueue;

    void updateNoiseFloor(float peak);
    void openGate(float *buffer, unsigned int sampleCount);
    void closeGate(float *buffer, unsigned int sampleCount);
    void mute(float *buffer, unsigned int sampleCount);
    void changeVolume(float *buffer, float volumeStart, float volumeEnd,
                          unsigned int bufferSampleCount);

    float threshold();

    int bufferSizeInMsec(int frames);

    unsigned int maxSubframes() ;

    int subframeSizeSamples(int bufferSizeInFrames, int subFrameCount);

    int mSubframeCount;
};


#endif //LOOPBACKAPP_NOISEGATE_H
