#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define SAMPLE_RATE 48000
#define CHANNELS 2
#define DELAY_MS 300                 // Delay in milliseconds
#define MAX_DELAY_SAMPLES (SAMPLE_RATE * CHANNELS * DELAY_MS / 1000)
#define BUFFER_SIZE (MAX_DELAY_SAMPLES * 2)  // Make it a bit larger for safety

ma_int16 delay_buffer[BUFFER_SIZE];
size_t buffer_size = BUFFER_SIZE;
size_t write_index = 0;
size_t read_index = 0;
size_t delay_in_samples = MAX_DELAY_SAMPLES;

void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
    if (pDevice->capture.format != ma_format_s16 || pDevice->playback.format != ma_format_s16) {
        return;
    }

    ma_int16* pIn = (ma_int16*)pInput;
    ma_int16* pOut = (ma_int16*)pOutput;

    float pitchFactor = 0.6f;
    float slowDown = 0.5f;
    float wobbleDepth = 0.2f;
    float wobbleRate = 3.0f;
    float sampleRate = (float)SAMPLE_RATE;
    float volume = 0.95f;

    static float phase = 0.0f;

    size_t samplesToProcess = frameCount * CHANNELS;

    // Write input to circular buffer
    for (size_t i = 0; i < samplesToProcess; i++) {
        delay_buffer[write_index] = pIn[i];
        write_index = (write_index + 1) % buffer_size;
    }

    read_index = (write_index + buffer_size - delay_in_samples) % buffer_size;

    for (size_t i = 0; i < samplesToProcess; ++i) {
        // Simulate slow read for pitch shift
        size_t slowIndex = (size_t)((float)i * slowDown / pitchFactor);
        size_t readPos = (read_index + slowIndex) % buffer_size;

        ma_int16 delayed = delay_buffer[readPos];
        float sample = (float)delayed / 32768.0f;

        // Add wobble effect
        sample *= 1.0f + wobbleDepth * sinf(phase);

        // Soft clipping
        if (sample > 0.8f) sample = 0.8f + (sample - 0.8f) * 0.1f;
        if (sample < -0.8f) sample = -0.8f + (sample + 0.8f) * 0.1f;

        sample *= volume;

        // Clamp and convert to int16
        if (sample > 1.0f) sample = 1.0f;
        if (sample < -1.0f) sample = -1.0f;

        pOut[i] = (ma_int16)(sample * 32768.0f);

        // Advance wobble phase
        phase += 2.0f * 3.14159265f * wobbleRate / sampleRate;
        if (phase >= 2.0f * 3.14159265f) phase -= 2.0f * 3.14159265f;
    }
}

#ifdef __EMSCRIPTEN__
void main_loop__em() {}
#endif

int main(int argc, char **argv) {
    ma_result result;
    ma_device_config deviceConfig;
    ma_device device;

    deviceConfig = ma_device_config_init(ma_device_type_duplex);
    deviceConfig.capture.format = ma_format_s16;
    deviceConfig.capture.channels = CHANNELS;
    deviceConfig.playback.format = ma_format_s16;
    deviceConfig.playback.channels = CHANNELS;
    deviceConfig.dataCallback = data_callback;

    result = ma_device_init(NULL, &deviceConfig, &device);
    if (result != MA_SUCCESS) {
        printf("Failed to initialize device: %d\n", result);
        return result;
    }

    result = ma_device_start(&device);
    if (result != MA_SUCCESS) {
        printf("Failed to start device: %d\n", result);
        ma_device_uninit(&device);
        return result;
    }

#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop(main_loop__em, 0, 1);
#else
    printf("Press Enter to quit...\n");
    getchar();
    ma_device_uninit(&device);
#endif

    (void)argc;
    (void)argv;
    return 0;
}
