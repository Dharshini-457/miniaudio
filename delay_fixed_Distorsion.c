#define MINIAUDIO_IMPLEMENTATION
// Make sure these definitions appear BEFORE including miniaudio.h
#include "miniaudio.h"
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
 
// Circular buffer for 6-second delay
#define SAMPLE_RATE 48000 // Assuming 48 kHz sample rate
#define CHANNELS 2        // Stereo
#define FORMAT_SIZE 2     // 16-bit samples (2 bytes per sample)
#define DELAY_SECONDS 6   // 6-second delay
#define BUFFER_SIZE (SAMPLE_RATE * CHANNELS * FORMAT_SIZE * DELAY_SECONDS)
static char delay_buffer[BUFFER_SIZE];
static size_t write_index = 0;
static size_t read_index = 0; // Initialize read_index to 0
#ifdef __EMSCRIPTEN__
void main_loop__em() {}
#endif

void data_callback(ma_device *pDevice, void *pOutput, const void *pInput,ma_uint32 frameCount) {
    if (pDevice->capture.format != pDevice->playback.format ||pDevice->capture.channels != pDevice->playback.channels) {
        return;
    }

    size_t bytes_per_frame = ma_get_bytes_per_frame(pDevice->capture.format, pDevice->capture.channels);
    size_t bytes_to_process = frameCount * bytes_per_frame;

    // Calculate the delay in bytes
    size_t delay_in_bytes = SAMPLE_RATE * CHANNELS * FORMAT_SIZE * DELAY_SECONDS;

    // Write input (recorded) data to the circular buffer
    for (size_t i = 0; i < bytes_to_process; i++) {
        delay_buffer[write_index] = ((char *)pInput)[i];
        write_index = (write_index + 1) % BUFFER_SIZE;
    }

    // Ensure read_index lags behind write_index by the delay size
    read_index = (write_index + BUFFER_SIZE - delay_in_bytes) % BUFFER_SIZE;

    // Read delayed data from the circular buffer into the output
    for (size_t i = 0; i < bytes_to_process; i++) {
        ((char *)pOutput)[i] = delay_buffer[read_index];
        read_index = (read_index + 1) % BUFFER_SIZE;
    }
}

int main(int argc, char **argv) {
    ma_result result;
    ma_device_config deviceConfig;
    ma_device device;

    deviceConfig = ma_device_config_init(ma_device_type_duplex);
    deviceConfig.capture.pDeviceID = NULL;
    deviceConfig.capture.format = ma_format_s16;
    deviceConfig.capture.channels = 2;
    deviceConfig.capture.shareMode = ma_share_mode_shared ;
    deviceConfig.playback.pDeviceID = NULL;
    deviceConfig.playback.format = ma_format_s16;
    deviceConfig.playback.channels = 2;
    deviceConfig.dataCallback = data_callback;
    result = ma_device_init(NULL, &deviceConfig, &device);
    if (result != MA_SUCCESS) {
        return result;
    }

#ifdef __EMSCRIPTEN__
    getchar();
#endif
    ma_device_start(&device);

#ifdef __EMSCRIPTEN__
    emscripten_set_main_ loop(main_loop__em, 0, 1);
#else
    printf("Press Enter to quit...\n");
    getchar();
#endif

    ma_device_uninit(&device);

    (void)argc;
    (void)argv;
    return 0;
 }