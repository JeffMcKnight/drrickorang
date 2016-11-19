//
// Created by JeffMcKnight on 11/18/16.
//

#include <SuperpoweredSimple.h>
#include "NoiseGate.h"
#include "../../../../../../../Shared/Library/Android/sdk/ndk-bundle/platforms/android-16/arch-x86/usr/include/android/log.h"

NoiseGate::NoiseGate(int sampleRate, int thresholdWindowMsec,
                     float thresholdFactor,
                     int subframeSizeMsec) :
        SAMPLE_RATE(sampleRate),
        THRESHOLD_WINDOW_MSEC(thresholdWindowMsec),
        SAMPLES_STEREO_ONE_MSEC((unsigned int) (2 * sampleRate / 1000)),
        SUBFRAME_SIZE_MSEC(subframeSizeMsec),
        SUBFRAME_SIZE_SAMPLES(subframeSizeMsec * SAMPLES_STEREO_ONE_MSEC),
        mIsGateOpen(true),
        mNoiseFloor(0.0F),
        THRESHOLD_FACTOR(thresholdFactor) {
    __android_log_print(ANDROID_LOG_DEBUG, "NoiseGate", "NoiseGate() -- mPeakQueue.size(): %d", mPeakQueue.size());

}

/**
 *
 */
void NoiseGate::process(float *stereoBuffer, unsigned int bufferSizeInFrames, unsigned int channelCount) {
    int sizeInMsec = bufferSizeInMsec(bufferSizeInFrames);
//    __android_log_print(ANDROID_LOG_DEBUG, "NoiseGate", "process() -- bufferSizeInFrames: %d -- sizeInMsec: %d", bufferSizeInFrames, sizeInMsec);
    int subframeSamples = subframeSizeSamples(channelCount);
    for (int i=0; i < sizeInMsec; i++){
        processSubframe(stereoBuffer + (subframeSamples * i), (unsigned int) subframeSamples);
    }
}

/**
 *
 */
void NoiseGate::processSubframe(float *subFrame, unsigned int subframeSizeSamples) {
//    __android_log_print(ANDROID_LOG_INFO, "NoiseGate", "processSubframe() -- subFrame: %p -- subframeSizeSamples: %d -- threshold(): %f", subFrame, subframeSizeSamples, threshold());
    float peak = SuperpoweredPeak(subFrame, subframeSizeSamples);
    if (peak > threshold()) {
        if (!mIsGateOpen) {
            openGate(subFrame);
            mIsGateOpen = true;
        }     
    } else {
        if (mIsGateOpen){
            closeGate(subFrame);
            mIsGateOpen = false;
        } else {
            mute(subFrame);
        }
    }
    updateNoiseFloor(peak);
}

/**
 *
 */
void NoiseGate::updateNoiseFloor(float peak) {
    float noiseSum = mNoiseFloor * mPeakQueue.size();
    mPeakQueue.push(peak);
    noiseSum = noiseSum + mPeakQueue.back();
//    __android_log_print(ANDROID_LOG_VERBOSE, "NoiseGate", "updateNoiseFloor() -- mPeakQueue.size(): %u -- noiseSum: %f -- peak: %f", mPeakQueue.size(), noiseSum, peak);
    if (mPeakQueue.size() > maxSubframes()){
        noiseSum = noiseSum - mPeakQueue.front();
        mPeakQueue.pop();
    }
    mNoiseFloor = noiseSum/mPeakQueue.size();
}

unsigned int NoiseGate::maxSubframes() {
    return (unsigned int) (THRESHOLD_WINDOW_MSEC / SUBFRAME_SIZE_MSEC);
}

int NoiseGate::bufferSizeInMsec(int frames) {
    int chunkCount = frames // frames
                     * 1000 // msec per second
                     / SAMPLE_RATE;  // samples per second
    return chunkCount;
}

void NoiseGate::openGate(float *buffer) {
    changeVolume(buffer, 0.0F, 1.0F);
}

void NoiseGate::closeGate(float *buffer) {
    changeVolume(buffer, 1.0F, 0.0F);
}

void NoiseGate::mute(float *buffer) {
    changeVolume(buffer, 0.0F, 0.0F);
}

void NoiseGate::changeVolume(float *buffer, float volumeStart, float volumeEnd) {
//    __android_log_print(ANDROID_LOG_DEBUG, "NoiseGate", "changeVolume() -- volumeStart: %f -- volumeEnd: %f", volumeStart, volumeEnd);
    SuperpoweredVolume(buffer, buffer, volumeStart, volumeEnd, SUBFRAME_SIZE_SAMPLES);
}

float NoiseGate::threshold() {
    return THRESHOLD_FACTOR * mNoiseFloor;
}

int NoiseGate::subframeSizeSamples(unsigned int channelCount) {
    return
            SUBFRAME_SIZE_MSEC  // msec
            * SAMPLE_RATE       // frames/sec
            * channelCount      // samples/frame
           / 1000;              // msec/sec
}















