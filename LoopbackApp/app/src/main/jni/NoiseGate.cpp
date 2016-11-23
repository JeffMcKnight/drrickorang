//
// Created by JeffMcKnight on 11/18/16.
//

#include <SuperpoweredSimple.h>
#include "NoiseGate.h"
#include "../../../../../../../Shared/Library/Android/sdk/ndk-bundle/platforms/android-16/arch-x86/usr/include/android/log.h"

NoiseGate::NoiseGate(int sampleRate,
                     int thresholdWindowMsec,
                     float thresholdFactor,
                     int subframeSizeMsec) :
        SAMPLE_RATE(sampleRate),
        THRESHOLD_WINDOW_MSEC(thresholdWindowMsec),
        SAMPLES_STEREO_ONE_MSEC((unsigned int) (2 * sampleRate / 1000)),
        SUBFRAME_SIZE_MSEC(subframeSizeMsec),
        SUBFRAME_SIZE_SAMPLES(subframeSizeMsec * SAMPLES_STEREO_ONE_MSEC),
        mIsGateOpen(true),
        mNoiseFloor(1.0F),
        mLastPeak(0.0F),
        mSubframeCount(0),
        THRESHOLD_FACTOR(thresholdFactor) {
    __android_log_print(ANDROID_LOG_DEBUG, "NoiseGate", "NoiseGate() -- mPeakQueue.size(): %d", mPeakQueue.size());

}

/**
 * Apply a noise gate to the current buffer frame.  We reset {@link #mIsPulseDetected} so
 */
void NoiseGate::process(float *stereoBuffer, unsigned int bufferSizeInFrames) {
    mIsPulseDetected = false;
    mIsPeakDetected = false;
    mOpenGateCount = 0;
//    int sizeInMsec = bufferSizeInMsec(bufferSizeInFrames);
//    __android_log_print(ANDROID_LOG_DEBUG, "NoiseGate", "process() -- bufferSizeInFrames: %d -- sizeInMsec: %d", bufferSizeInFrames, sizeInMsec);
    int subframeSamples = subframeSizeSamples(bufferSizeInFrames, SUBFRAME_COUNT);
    for (int i=0; i < SUBFRAME_COUNT; i++){
        processSubframe(stereoBuffer + (subframeSamples * i), (unsigned int) subframeSamples);
    }
}

/**
 * Check the peak level of {@code subFrame} and set the noise gate appropriately.
 */
void NoiseGate::processSubframe(float *subFrame, unsigned int subframeSizeSamples) {
//    __android_log_print(ANDROID_LOG_INFO, "NoiseGate", "processSubframe() -- subFrame: %p -- subframeSizeSamples: %d -- threshold(): %f", subFrame, subframeSizeSamples, threshold());
    float peak = SuperpoweredPeak(subFrame, subframeSizeSamples);
    if (peak > threshold()) {
        mIsPulseDetected = true;
        if (!mIsGateOpen) {
            openGate(subFrame, subframeSizeSamples);
            mIsGateOpen = true;
            __android_log_print(ANDROID_LOG_INFO, "NoiseGate", "processSubframe() -- OPEN GATE -- mSubframeCount: %i -- peak: %f -- threshold(): %f", mSubframeCount, peak, threshold());
        }
        // The previous subframe is the peak subframe if it is louder than the peak of the current subframe
        if (peak < mLastPeak) {
            mIsPeakDetected = true;
        }
        mOpenGateCount++;
    } else {
        if (mIsGateOpen){
            closeGate(subFrame, subframeSizeSamples);
            mIsGateOpen = false;
            __android_log_print(ANDROID_LOG_INFO, "NoiseGate", "processSubframe() -- CLOSE GATE -- mSubframeCount: %i -- peak: %f -- threshold(): %f", mSubframeCount, peak, threshold());
        } else {
            mute(subFrame, subframeSizeSamples);
        }
    }
    updateNoiseFloor(peak);
    mLastPeak = peak;
    mSubframeCount++;
}

/**
 *
 */
void NoiseGate::updateNoiseFloor(float peak) {
    float noiseSum = mNoiseFloor * mPeakQueue.size();
    mPeakQueue.push(peak);
    noiseSum = noiseSum + mPeakQueue.back();
    __android_log_print(ANDROID_LOG_VERBOSE, "NoiseGate", "updateNoiseFloor() -- mNoiseFloor: %f -- noiseSum: %f -- peak: %f -- mPeakQueue.size(): %u", mNoiseFloor, noiseSum, peak, mPeakQueue.size());
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

void NoiseGate::openGate(float *buffer, unsigned int sampleCount) {
    changeVolume(buffer, 0.0F, 1.0F, sampleCount);
}

void NoiseGate::closeGate(float *buffer, unsigned int sampleCount) {
    changeVolume(buffer, 1.0F, 0.0F, sampleCount);
}

void NoiseGate::mute(float *buffer, unsigned int sampleCount) {
    changeVolume(buffer, 0.0F, 0.0F, sampleCount);
}

void NoiseGate::changeVolume(float *buffer, float volumeStart, float volumeEnd,
                             unsigned int bufferSampleCount) {
//    __android_log_print(ANDROID_LOG_DEBUG, "NoiseGate", "changeVolume() -- volumeStart: %f -- volumeEnd: %f", volumeStart, volumeEnd);
    SuperpoweredVolume(buffer, buffer, volumeStart, volumeEnd, bufferSampleCount);
}

float NoiseGate::threshold() {
    return THRESHOLD_FACTOR * mNoiseFloor;
}

int NoiseGate::subframeSizeSamples(int bufferSizeInFrames, int subFrameCount) {
    int subframeSampleCount = 2 * bufferSizeInFrames / subFrameCount;
    __android_log_print(ANDROID_LOG_DEBUG, "NoiseGate", "subframeSizeSamples() -- subframeSampleCount: %i", subframeSampleCount);
    return subframeSampleCount
//            SUBFRAME_SIZE_MSEC  // msec
//            * SAMPLE_RATE       // frames/sec
//            * channelCount      // samples/frame
//           / 1000
            ;              // msec/sec
}

bool NoiseGate::isPulseDetected() {
    return mIsPulseDetected;
}

bool NoiseGate::isPeakDetected() {
    return mIsPeakDetected;
}

int NoiseGate::openGateCount() {
    return mOpenGateCount;
}





















