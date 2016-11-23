//
// Created by JeffMcKnight on 11/16/16.
//

#include "PulseEnhancer.h"
#include "../../../../../../../Shared/Library/Android/sdk/ndk-bundle/platforms/android-16/arch-x86/usr/include/android/log.h"

/**
 * Constructor to initialize fields and constants with default values.
 * <b>Do not directly use this method to create an instance of {@link PulseEnhancer}.  Use the factory
 * method instead. </b>
 * FILTER_FREQUENCY_HZ should match Constant.LOOPBACK_FREQUENCY
 * FILTER_RESONANCE chosen to make feedback fairly stable
 */
PulseEnhancer::PulseEnhancer(int sampleRate, int injectedFrequencyHz) :
        SAMPLE_RATE(sampleRate),
        SAMPLES_MONO_ONE_MSEC((unsigned int) (sampleRate / 1000)),
        SAMPLES_STEREO_ONE_MSEC(2 * SAMPLES_MONO_ONE_MSEC),
        mVolume(1.0F),
        mTimeElapsed(0),
        FILTER_FREQUENCY_HZ(static_cast<float>(injectedFrequencyHz)),
        FILTER_RESONANCE(2.0f),
        TARGET_PEAK(1.0f) {
    __android_log_print(ANDROID_LOG_INFO, "PulseEnhancer", "::create -- SAMPLE_RATE: %d -- SAMPLES_MONO_ONE_MSEC: %d -- SAMPLES_STEREO_ONE_MSEC: %d", SAMPLE_RATE, SAMPLES_MONO_ONE_MSEC, SAMPLES_STEREO_ONE_MSEC);
}

/**
 * Factory method. This is the preferred method to create instances of {@link PulseEnhancer}.
 * It creates the object and also sets up the audio {@link #filter}
 */
PulseEnhancer *PulseEnhancer::create(int samplingRate, int injectedFrequencyHz) {
    PulseEnhancer *enhancer = new PulseEnhancer(samplingRate, injectedFrequencyHz);
    enhancer->createFilter(static_cast<unsigned int>(samplingRate));
    enhancer->createLimiter(static_cast<unsigned int>(samplingRate));
    enhancer->mNoiseGate = new NoiseGate(samplingRate, 50, 1.0F, 1);
    return enhancer;
}

/**
 * Process the raw record buffer so we can do the latency measurement test in open air without the
 * loopback dongle.
 */
void PulseEnhancer::processForOpenAir(char *inputBuffer,
                                      char *outputBuffer,
                                      SLuint32 bufSizeInFrames,
                                      SLuint32 channelCount) {
//    __android_log_print(ANDROID_LOG_INFO, "PulseEnhancer", "processForOpenAir() -- buffer: %p -- frames: %u -- channelCount: %u", recordBuffer, frames, channelCount);
    // Convert the raw buffer to stereo floating point;  pSles->rxBuffers stores the data recorded
    unsigned int bufferSizeInSamples = channelCount * bufSizeInFrames;
    float *stereoBuffer = new float[2 * bufSizeInFrames];
    monoShortIntToStereoFloat(inputBuffer, bufSizeInFrames, channelCount, stereoBuffer);
    if (mTimeElapsed < WARMUP_PERIOD_MSEC) {
        mute(stereoBuffer, bufferSizeInSamples);
    }
    //  Filter the mic input to get a cleaner pulse and reduce runaway feedback (not exactly Larsen effect, but similar)
    mFilter->process(stereoBuffer, stereoBuffer, bufSizeInFrames);
    // Apply a basic noise gate to remove ambient noise
    mNoiseGate->process(stereoBuffer, bufSizeInFrames);
//    mNoiseGate->processSubframe(stereoBuffer, 2*bufSizeInFrames);
    // Apply a limiter so we do not get clipping
    mLimiter->process(stereoBuffer, stereoBuffer, bufSizeInFrames);
//    enhancePulse(stereoBuffer, bufSizeInFrames);
//    predictLimiter(stereoBuffer, bufSizeInFrames);
    // Convert buffer back to 16 bit mono
    stereoFloatToMonoShortInt(stereoBuffer, outputBuffer, bufSizeInFrames, channelCount);
    delete[] stereoBuffer;
    mTimeElapsed += bufferSizeInMsec(bufSizeInFrames);
//    __android_log_print(ANDROID_LOG_INFO, "PulseEnhancer", "processForOpenAir() -- mTimeElapsed : %i ", mTimeElapsed);
}

/**
 * Boost the pulse and attenuate/mute ambient noise.
 */
void PulseEnhancer::enhancePulse(float *stereoBuffer, SLuint32 bufSizeInFrames) {
//    __android_log_print(ANDROID_LOG_INFO, "PulseEnhancer", "enhancePulse() -- buffer: %p -- frames: %u", buffer, frames);
    // Quick and dirty Gain Limiter --
    float peak = SuperpoweredPeak(stereoBuffer, 2 * bufSizeInFrames);
    // TARGET_PEAK should always be >0.0f so we never try to divide by 0
    if (peak > TARGET_PEAK) {
        float targetVolume = TARGET_PEAK / peak;
        if (mVolume > targetVolume) {
            mVolume = targetVolume;
        }
    }
    SuperpoweredVolume(stereoBuffer, stereoBuffer, mVolume, mVolume, bufSizeInFrames);
//        SLES_PRINTF("recorderCallback() "
//                            " --- peak: %.2f"
//                            " --- pSles->volume: %.2f",
//                    peak,
//                    pSles->volume
//        );
}

/**
 * Convert stereo floating point buffer back to mono 16 bit buffer which matches the format of the
 * recording callback.
 */
void PulseEnhancer::stereoFloatToMonoShortInt(float *stereoInputBuffer,
                                              char *monoOutputBuffer,
                                              SLuint32 bufSizeInFrames,
                                              SLuint32 channelCount) {
//    __android_log_print(ANDROID_LOG_INFO, "PulseEnhancer", "stereoFloatToMonoShortInt() -- outputBuffer: %p -- frames: %u -- channelCount: %u", outputBuffer, frames, channelCount);
    // Deinterleave back to a mono audio frame
    float *right = new float[bufSizeInFrames];
    float *left = new float[bufSizeInFrames];
    SuperpoweredDeInterleave(stereoInputBuffer, left, right, bufSizeInFrames);
    // Convert floating point linear PCM back to 16bit linear PCM
    SuperpoweredFloatToShortInt(left, (short *) monoOutputBuffer, bufSizeInFrames, channelCount);
    // Release audio frame memory resources
    delete[] left;
    delete[] right;
}

/**
 * Convert mono 16 bit linear PCM buffer to stereo floating point buffer because most Superpowered
 * methods expect stereo float frames
 * TODO: do this without using so many buffer resources
*/
void PulseEnhancer::monoShortIntToStereoFloat(const char *monoShortIntBuffer,
                                              SLuint32 bufSizeInFrames, SLuint32 channelCount,
                                              float *stereoFloatBuffer) {
    //            = (float *) SuperpoweredAudiobufferPool::getBuffer(2 * bufSizeInFrames * sizeof(float));
    float *monoFloatBuffer
            = new float[bufSizeInFrames];
//            = (float *) SuperpoweredAudiobufferPool::getBuffer(bufSizeInFrames * sizeof(float));
    // Convert mono 16bit buffer to mono floating point buffer
    SuperpoweredShortIntToFloat((short int *) monoShortIntBuffer,
                                monoFloatBuffer,
                                bufSizeInFrames,
                                channelCount);
    // Convert mono float buffer to stereo float buffer
    SuperpoweredInterleave(monoFloatBuffer, monoFloatBuffer, stereoFloatBuffer, bufSizeInFrames);
//    SuperpoweredAudiobufferPool::releaseBuffer(monoFloatBuffer);
    delete[] monoFloatBuffer;
}

/**
 * Create the 19kHz high pass resonant filter
 */
void PulseEnhancer::createFilter(unsigned int samplingRate) {
/** Set up lowpass filter */
//    filter = new SuperpoweredFilter(SuperpoweredFilter_Resonant_Lowpass, samplingRate);
//    filter->setResonantParameters(200.0f, 0.2f);

//    SLES_PRINTF("slesInit"
//                            " -- samplingRate: %d"
//        ,samplingRate);
    /** Set up highpass filter */
    mFilter = new SuperpoweredFilter(SuperpoweredFilter_Resonant_Highpass, samplingRate);
    mFilter->setResonantParameters(FILTER_FREQUENCY_HZ, FILTER_RESONANCE);

    /** Set up bandpass filter with a bandwidth of 0.1 octaves */
//    filter = new SuperpoweredFilter(SuperpoweredFilter_Bandlimited_Bandpass, samplingRate);
//    float filterWidthInOctaves = 0.1f;
//    filter->setBandlimitedParameters(19.0f * 1000.0f, filterWidthInOctaves);

    mFilter->enable(true);
}

int PulseEnhancer::bufferSizeInMsec(int bufSizeInFrames) const {
    int chunkCount = bufSizeInFrames // frames
                     * 1000 // msec per second
                     / SAMPLE_RATE;  // samples per second
    return chunkCount;
}

void PulseEnhancer::mute(float *audioBuffer, unsigned int numberOfSamples) {
    SuperpoweredVolume(audioBuffer, audioBuffer, 0.0F, 0.0F, numberOfSamples);
}

void PulseEnhancer::createLimiter(unsigned int samplingRate) {
    mLimiter = new SuperpoweredLimiter(samplingRate);
    mLimiter->ceilingDb = -1.0F;
    mLimiter->thresholdDb = -1.0F;
    mLimiter->releaseSec = 0.20F;
    mLimiter->enable(true);
}

void PulseEnhancer::predictLimiter(float *buffer, SLuint32 frames) {
//    __android_log_print(ANDROID_LOG_INFO, "PulseEnhancer", "predictLimiter() -- buffer: %p -- frames: %u", buffer, frames);
    float peak = SuperpoweredPeak(buffer, 2 * frames);
    // TARGET_PEAK should always be >0.0f so we never try to divide by 0
    float targetVolume;
    if (peak > 0.0F) {
        targetVolume = TARGET_PEAK / peak;
    } else {
        targetVolume = mVolume;
    }
    __android_log_print(ANDROID_LOG_INFO, "PulseEnhancer",
                        "predictLimiter() -- isPulseDetected(): %d -- isPeakDetected(): %d -- openGateCount(): %d -- peak: %f -- targetVolume: %f",
                        mNoiseGate->isPulseDetected(),
                        mNoiseGate->isPeakDetected(),
                        mNoiseGate->openGateCount(),
                        peak,
                        targetVolume
    );
    if (mNoiseGate->isPulseDetected() && !mNoiseGate->isPeakDetected()) {
        targetVolume = targetVolume * static_cast<float>(mNoiseGate->openGateCount()) / 3.0F;
    }
    SuperpoweredVolume(buffer, buffer, mVolume, targetVolume, frames);
    mVolume = targetVolume;

}








