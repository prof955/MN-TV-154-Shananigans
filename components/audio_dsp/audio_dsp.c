#include "audio_dsp.h"
#include <math.h>
#include <string.h>
#include "esp_log.h"
#include "driver/i2s_std.h"
#include "freertos/FreeRTOS.h"

static const char *TAG = "audio_dsp";

static i2s_chan_handle_t rx_handle = NULL;
static float hann_window[FFT_N];
static spectrum_data_t current_spectrum = {0};

// Precalculated FFT lookup tables
static uint16_t bit_reverse_table[FFT_N];
static float cos_table[FFT_N / 2];
static float sin_table[FFT_N / 2];

// Logarithmic frequency band boundary indices for 512-point FFT @ 22.05kHz
static const uint16_t band_boundaries[SPECTRUM_BANDS + 1] = {
    1, 2, 3, 4, 6, 8, 11, 15, 21, 29, 40, 55, 76, 105, 145, 200, 255
};

static uint16_t reverse_bits(uint16_t val, int bits) {
    uint16_t res = 0;
    for (int i = 0; i < bits; i++) {
        if (val & (1 << i)) {
            res |= (1 << (bits - 1 - i));
        }
    }
    return res;
}

static void init_fft_tables(void) {
    for (int i = 0; i < FFT_N; i++) {
        bit_reverse_table[i] = reverse_bits(i, 9); // 2^9 = 512
        hann_window[i] = 0.5f * (1.0f - cosf(2.0f * M_PI * i / (FFT_N - 1)));
    }
    for (int i = 0; i < FFT_N / 2; i++) {
        cos_table[i] = cosf(-2.0f * M_PI * i / FFT_N);
        sin_table[i] = sinf(-2.0f * M_PI * i / FFT_N);
    }
}

// In-tree Radix-2 Cooley-Tukey FFT implementation
static void fft_radix2(float *real, float *imag) {
    // 1. Bit reversal permutation
    for (int i = 0; i < FFT_N; i++) {
        int rev = bit_reverse_table[i];
        if (i < rev) {
            float temp_r = real[i];
            float temp_i = imag[i];
            real[i] = real[rev];
            imag[i] = imag[rev];
            real[rev] = temp_r;
            imag[rev] = temp_i;
        }
    }

    // 2. Cooley-Tukey computation
    for (int len = 2; len <= FFT_N; len <<= 1) {
        int half_len = len >> 1;
        int step = FFT_N / len;
        for (int i = 0; i < FFT_N; i += len) {
            for (int j = 0; j < half_len; j++) {
                int table_idx = j * step;
                float w_r = cos_table[table_idx];
                float w_i = sin_table[table_idx];

                int u_idx = i + j;
                int v_idx = u_idx + half_len;

                float u_r = real[u_idx];
                float u_i = imag[u_idx];

                float v_r = real[v_idx] * w_r - imag[v_idx] * w_i;
                float v_i = real[v_idx] * w_i + imag[v_idx] * w_r;

                real[u_idx] = u_r + v_r;
                imag[u_idx] = u_i + v_i;
                real[v_idx] = u_r - v_r;
                imag[v_idx] = u_i - v_i;
            }
        }
    }
}

esp_err_t audio_dsp_init(void)
{
    ESP_LOGI(TAG, "Initializing INMP441 I2S Digital Microphone & FFT...");

    init_fft_tables();

    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
    ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, NULL, &rx_handle));

    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(AUDIO_SAMPLE_RATE),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_32BIT, I2S_SLOT_MODE_MONO),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = PIN_I2S_BCLK,
            .ws   = PIN_I2S_WS,
            .dout = I2S_GPIO_UNUSED,
            .din  = PIN_I2S_SD,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv   = false,
            },
        },
    };
    std_cfg.slot_cfg.slot_mask = I2S_STD_SLOT_LEFT;

    // ESP-IDF v6.1 compatibility
    ESP_ERROR_CHECK(i2s_channel_init_std_mode(rx_handle, &std_cfg));
    ESP_ERROR_CHECK(i2s_channel_enable(rx_handle));

    memset(&current_spectrum, 0, sizeof(current_spectrum));
    ESP_LOGI(TAG, "I2S initialized successfully.");
    return ESP_OK;
}

void audio_dsp_process(spectrum_data_t *out_spectrum)
{
    static int32_t raw_i2s_buffer[FFT_N];
    static float real[FFT_N];
    static float imag[FFT_N];
    size_t bytes_read = 0;

    esp_err_t err = i2s_channel_read(rx_handle, raw_i2s_buffer, sizeof(raw_i2s_buffer), &bytes_read, pdMS_TO_TICKS(100));
    if (err != ESP_OK || bytes_read < sizeof(raw_i2s_buffer)) {
        if (out_spectrum) *out_spectrum = current_spectrum;
        return;
    }

    // 1. Convert 24-bit PCM inside 32-bit container & apply 6.0x Software Gain Boost
    float sum_sq = 0.0f;
    #define AUDIO_GAIN_BOOST 6.0f

    for (int i = 0; i < FFT_N; i++) {
        float sample = ((float)(raw_i2s_buffer[i] >> 8) / 8388608.0f) * AUDIO_GAIN_BOOST;
        current_spectrum.raw_audio_wave[i] = sample;
        sum_sq += sample * sample;
        real[i] = sample * hann_window[i];
        imag[i] = 0.0f;
    }

    // RMS Volume Calculation
    float rms = sqrtf(sum_sq / FFT_N);
    current_spectrum.rms_volume = rms * 100.0f * 3.5f;
    if (current_spectrum.rms_volume > 100.0f) current_spectrum.rms_volume = 100.0f;

    // Transient Clap Detection
    current_spectrum.clap_detected = (rms > 0.45f);

    // 2. Perform 512-point Real FFT
    fft_radix2(real, imag);

    // 3. Compute magnitudes and map into 16 Logarithmic Frequency Bands
    for (int b = 0; b < SPECTRUM_BANDS; b++) {
        uint16_t start_bin = band_boundaries[b];
        uint16_t end_bin = band_boundaries[b + 1];
        float max_mag = 0.0f;

        for (int k = start_bin; k < end_bin; k++) {
            float mag = sqrtf(real[k] * real[k] + imag[k] * imag[k]);
            if (mag > max_mag) max_mag = mag;
        }

        // High sensitivity band normalization (scaling threshold 3.5)
        float norm_val = (max_mag / 3.5f);
        if (norm_val > 1.0f) norm_val = 1.0f;

        // Dynamic smoothing
        current_spectrum.band_values[b] = current_spectrum.band_values[b] * 0.3f + norm_val * 0.7f;

        // Peak Hold and Exponential Decay logic
        if (current_spectrum.band_values[b] >= current_spectrum.band_peaks[b]) {
            current_spectrum.band_peaks[b] = current_spectrum.band_values[b];
            current_spectrum.peak_decay_counters[b] = 6;
        } else {
            if (current_spectrum.peak_decay_counters[b] > 0) {
                current_spectrum.peak_decay_counters[b]--;
            } else {
                current_spectrum.band_peaks[b] -= 0.05f;
                if (current_spectrum.band_peaks[b] < 0.0f) {
                    current_spectrum.band_peaks[b] = 0.0f;
                }
            }
        }
    }

    if (out_spectrum) {
        *out_spectrum = current_spectrum;
    }
}

void audio_dsp_reset_peaks(void)
{
    for (int i = 0; i < SPECTRUM_BANDS; i++) {
        current_spectrum.band_peaks[i] = 0.0f;
        current_spectrum.peak_decay_counters[i] = 0;
    }
}
