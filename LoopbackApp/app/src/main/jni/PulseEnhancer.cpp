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
PulseEnhancer::PulseEnhancer(int sampleRate) : SAMPLE_RATE(sampleRate),
                                               SAMPLES_MONO_ONE_MSEC((unsigned int) (sampleRate / 1000)),
                                               SAMPLES_STEREO_ONE_MSEC(2*SAMPLES_MONO_ONE_MSEC),
                                               mVolume(1.0F),
                                               mTimeElapsed(0),
                                               FILTER_FREQUENCY_HZ(19.0f * 1000.0F),
                                               FILTER_RESONANCE(2.0f),
                                               TARGET_PEAK(1.0f) {
    __android_log_print(ANDROID_LOG_INFO, "PulseEnhancer", "::create -- SAMPLE_RATE: %d -- SAMPLES_MONO_ONE_MSEC: %d -- SAMPLES_STEREO_ONE_MSEC: %d", SAMPLE_RATE, SAMPLES_MONO_ONE_MSEC, SAMPLES_STEREO_ONE_MSEC);
}

/**
 * Factory method. This is the preferred method to create instances of {@link PulseEnhancer}.
 * It creates the object and also sets up the audio {@link #filter}
 */
PulseEnhancer *PulseEnhancer::create(int samplingRate) {
    PulseEnhancer *enhancer = new PulseEnhancer(samplingRate);
    enhancer->createFilter(static_cast<unsigned int>(samplingRate));
    enhancer->mNoiseGate = new NoiseGate(samplingRate, 50, 1.5F, 1);
    return enhancer;
}
/**
 * Process the raw record buffer so we can do the latency measurement test in open air without the
 * loopback dongle.
 */
char *PulseEnhancer::processForOpenAir(SLuint32 bufSizeInFrames,
                                       SLuint32 channelCount,
                                       char *recordBuffer) {
//    __android_log_print(ANDROID_LOG_INFO, "PulseEnhancer", "processForOpenAir() -- stereoBuffer: %p -- bufSizeInFrames: %u -- channelCount: %u", recordBuffer, bufSizeInFrames, channelCount);
    // Convert the raw buffer to stereo floating point;  pSles->rxBuffers stores the data recorded
    unsigned int bufferSizeInSamples = channelCount * bufSizeInFrames;
    float *stereoBuffer = monoShortIntToStereoFloat(recordBuffer, bufSizeInFrames, channelCount);
    //  Filter the mic input to get a cleaner pulse and reduce runaway feedback (not exactly Larsen effect, but similar)
    mFilter->process(stereoBuffer, stereoBuffer, bufSizeInFrames);
    // Apply a basic noise gate to remove ambient noise
    mNoiseGate->process(stereoBuffer, bufSizeInFrames, channelCount);
    enhancePulse(stereoBuffer, bufSizeInFrames);
    if (mTimeElapsed < WARMUP_PERIOD_MSEC){
        mute(stereoBuffer, bufferSizeInSamples);
    }
    // Convert buffer back to 16 bit mono
    char *buffer = stereoFloatToMonoShortInt(stereoBuffer, bufSizeInFrames, channelCount);
    mTimeElapsed += bufferSizeInMsec(bufSizeInFrames);
//    __android_log_print(ANDROID_LOG_INFO, "PulseEnhancer", "processForOpenAir() -- mTimeElapsed : %i ", mTimeElapsed);
    return buffer;
}

/**
 * Boost the pulse and attenuate/mute ambient noise.
 */
void PulseEnhancer::enhancePulse(float *stereoBuffer, SLuint32 bufSizeInFrames) {
//    __android_log_print(ANDROID_LOG_INFO, "PulseEnhancer", "enhancePulse() -- stereoBuffer: %p -- bufSizeInFrames: %u", stereoBuffer, bufSizeInFrames);
    // Quick and dirty Gain Limiter --
    float peak = SuperpoweredPeak(stereoBuffer, 2 * bufSizeInFrames);
    // TARGET_PEAK should always be >0.0f so we never try to divide by 0
    if (peak > TARGET_PEAK){
        float targetVolume = TARGET_PEAK / peak;
        if (mVolume > targetVolume){
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
char *PulseEnhancer::stereoFloatToMonoShortInt(float *stereoBuffer,
                                               SLuint32 bufSizeInFrames,
                                               SLuint32 channelCount) {
//    __android_log_print(ANDROID_LOG_INFO, "PulseEnhancer", "stereoFloatToMonoShortInt() -- stereoBuffer: %p -- bufSizeInFrames: %u -- channelCount: %u", stereoBuffer, bufSizeInFrames, channelCount);
    void *buffer = new float[bufSizeInFrames];
    // Deinterleave back to a mono audio frame
    float *right = new float[bufSizeInFrames];
    float *left = new float[bufSizeInFrames];
    SuperpoweredDeInterleave(stereoBuffer, left, right, bufSizeInFrames);
    // Convert floating point linear PCM back to 16bit linear PCM
    SuperpoweredFloatToShortInt(left, (short *) buffer, bufSizeInFrames, channelCount);
    // Release audio frame memory resources
    delete[] left;
    delete[] right;
    return (char *) buffer;
}

/**
 * Convert mono 16 bit linear PCM buffer to stereo floating point buffer because most Superpowered
 * methods expect stereo float frames
 * TODO: do this without using so many buffer resources
*/
float *PulseEnhancer::monoShortIntToStereoFloat(const char *monoShortIntBuffer,
                                                SLuint32 bufSizeInFrames,
                                                SLuint32 channelCount) {
    float *stereoFloatBuffer = new float[2 * bufSizeInFrames];
    float *monoFloatBuffer = new float[bufSizeInFrames];
    // Convert mono 16bit buffer to mono floating point buffer
    SuperpoweredShortIntToFloat((short int *) monoShortIntBuffer,
                                monoFloatBuffer,
                                bufSizeInFrames,
                                channelCount);
    // Convert mono float buffer to stereo float buffer
    SuperpoweredInterleave(monoFloatBuffer, monoFloatBuffer, stereoFloatBuffer, bufSizeInFrames);
    delete[] monoFloatBuffer;
    return stereoFloatBuffer;
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




