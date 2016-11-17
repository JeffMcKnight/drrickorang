//
// Created by JeffMcKnight on 11/16/16.
//

#include "PulseEnhancer.h"

/**
 * Constructor to initialize fields and constants with default values.
 * <b>Do not directly use this method to create an instance of {@link PulseEnhancer}.  Use the factory
 * method instead. </b>
 * FILTER_FREQUENCY_HZ should match Constant.LOOPBACK_FREQUENCY
 * FILTER_RESONANCE chosen to make feedback fairly stable
 */
PulseEnhancer::PulseEnhancer() : volume(1.0F),
                                 FILTER_FREQUENCY_HZ(19.0f * 1000.0F),
                                 FILTER_RESONANCE(1.0f),
                                 TARGET_PEAK(1.0f) {
}

/**
 * Factory method. This is the preferred method to create instances of {@link PulseEnhancer}.
 * It creates the object and also sets up the audio {@link #filter}
 */
PulseEnhancer *PulseEnhancer::create(int samplingRate) {
    PulseEnhancer *enhancer = new PulseEnhancer();
    enhancer->createFilter(static_cast<unsigned int>(samplingRate));
    return enhancer;
}
/**
 * Process the raw record buffer so we can do the latency measurement test in open air without the
 * loopback dongle.
 */
char *PulseEnhancer::processForOpenAir(SLuint32 bufSizeInFrames,
                                       SLuint32 channelCount,
                                       char *recordBuffer) {
    // Convert the raw buffer to stereo floating point;  pSles->rxBuffers stores the data recorded
    float *stereoBuffer = monoShortIntToStereoFloat(recordBuffer, bufSizeInFrames, channelCount);
    //  Filter the mic input to get a cleaner pulse and reduce runaway feedback (not exactly Larsen effect, but similar)
    filter->process(stereoBuffer, stereoBuffer, bufSizeInFrames);
    enhancePulse(stereoBuffer, bufSizeInFrames);
    // Convert buffer back to 16 bit mono
    char *buffer = stereoFloatToMonoShortInt(stereoBuffer, bufSizeInFrames, channelCount);
    delete[] stereoBuffer;
    return buffer;
}

/**
 * Boost the pulse and attenuate/mute ambient noise.
 */
void PulseEnhancer::enhancePulse(float *stereoBuffer, SLuint32 bufSizeInFrames) {
    // Quick and dirty Gain Limiter --
    float peak = SuperpoweredPeak(stereoBuffer, 2 * bufSizeInFrames);
    // TARGET_PEAK should always be >0.0f so we never try to divide by 0
    if (peak > TARGET_PEAK){
        float targetVolume = TARGET_PEAK / peak;
        if (volume > targetVolume){
            volume = targetVolume;
        }
    }
    SuperpoweredVolume(stereoBuffer, stereoBuffer, volume, volume, bufSizeInFrames);
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
    filter = new SuperpoweredFilter(SuperpoweredFilter_Resonant_Highpass, samplingRate);
    filter->setResonantParameters(FILTER_FREQUENCY_HZ, FILTER_RESONANCE);

    /** Set up bandpass filter with a bandwidth of 0.1 octaves */
//    filter = new SuperpoweredFilter(SuperpoweredFilter_Bandlimited_Bandpass, samplingRate);
//    float filterWidthInOctaves = 0.1f;
//    filter->setBandlimitedParameters(19.0f * 1000.0f, filterWidthInOctaves);

    filter->enable(true);
}
