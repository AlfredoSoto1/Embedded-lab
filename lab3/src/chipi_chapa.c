#define _GNU_SOURCE
#include <gpiod.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <errno.h>
#include <string.h>
#include <math.h>
#include <stdint.h>

#define CHIP "/dev/gpiochip4"
#define LED_TEST 21

#define WAV_FILE "/home/alfredo/Desktop/repositories/Embedded-lab/Chipi.wav"

// WAV file header structure
typedef struct {
    char riff[4];              // "RIFF"
    uint32_t file_size;
    char wave[4];              // "WAVE"
    char fmt[4];               // "fmt "
    uint32_t fmt_size;
    uint16_t audio_format;
    uint16_t num_channels;
    uint32_t sample_rate;
    uint32_t byte_rate;
    uint16_t block_align;
    uint16_t bits_per_sample;
} __attribute__((packed)) WavHeader;

typedef struct {
    char id[4];                // "data"
    uint32_t size;
} __attribute__((packed)) DataChunkHeader;

// Global variables for signal handler
static struct gpiod_line *led = NULL;
static uint8_t *audio_buffer = NULL;
static size_t current_sample = 0;
static size_t total_samples = 0;
static int bytes_per_sample = 0;
static int num_channels = 0;
static int bits_per_sample = 0;
static volatile int playback_complete = 0;

// Timer interrupt handler for audio playback
void timer_handler(int sig, siginfo_t *si, void *uc) {
    (void)sig;
    (void)si;
    (void)uc;
    
    if (led && audio_buffer && current_sample < total_samples) {
        size_t byte_offset = current_sample * bytes_per_sample * num_channels;
        int16_t sample = 0;
        
        // Read sample (convert to 16-bit signed)
        if (bits_per_sample == 8) {
            // 8-bit samples are unsigned (0-255)
            sample = (audio_buffer[byte_offset] - 128) << 8;
        } else if (bits_per_sample == 16) {
            // 16-bit samples are signed
            sample = *(int16_t*)(&audio_buffer[byte_offset]);
        }
        
        // Simple 1-bit output: high if sample > 0, low otherwise
        int output = (sample > 0) ? 1 : 0;
        gpiod_line_set_value(led, output);
        
        current_sample++;
    } else {
        playback_complete = 1;
    }
}

int main(void) {
    struct gpiod_chip *chip = gpiod_chip_open(CHIP);
    if (!chip) {
        perror("gpiod_chip_open");
        return 1;
    }

    // Set up the LED line as output test
    led = gpiod_chip_get_line(chip, LED_TEST);
    if (!led) {
        perror("gpiod_chip_get_line");
        gpiod_chip_close(chip);
        return 1;
    }

    if (gpiod_line_request_output(led, "led", 0) < 0) { 
        perror("led"); 
        return 1; 
    }
    
    // Open WAV file
    FILE *wav_file = fopen(WAV_FILE, "rb");
    if (!wav_file) {
        perror("fopen wav file");
        return 1;
    }
    
    // Read WAV header
    WavHeader header;
    if (fread(&header, sizeof(WavHeader), 1, wav_file) != 1) {
        perror("fread header");
        fclose(wav_file);
        return 1;
    }
    
    printf("WAV Info:\n");
    printf("  Channels: %d\n", header.num_channels);
    printf("  Sample Rate: %d Hz\n", header.sample_rate);
    printf("  Bits per Sample: %d\n", header.bits_per_sample);
    
    // Find data chunk
    DataChunkHeader data_chunk;
    while (fread(&data_chunk, sizeof(DataChunkHeader), 1, wav_file) == 1) {
        if (strncmp(data_chunk.id, "data", 4) == 0) {
            break;
        }
        // Skip unknown chunk
        fseek(wav_file, data_chunk.size, SEEK_CUR);
    }
    
    printf("  Data Size: %d bytes\n", data_chunk.size);
    
    // Store audio parameters in global variables
    bytes_per_sample = header.bits_per_sample / 8;
    num_channels = header.num_channels;
    bits_per_sample = header.bits_per_sample;
    total_samples = data_chunk.size / (bytes_per_sample * num_channels);
    
    // Read audio data into buffer
    audio_buffer = malloc(data_chunk.size);
    if (!audio_buffer) {
        perror("malloc");
        fclose(wav_file);
        return 1;
    }
    
    size_t bytes_read = fread(audio_buffer, 1, data_chunk.size, wav_file);
    fclose(wav_file);
    
    if (bytes_read != data_chunk.size) {
        fprintf(stderr, "Failed to read complete audio data\n");
        free(audio_buffer);
        return 1;
    }
    
    printf("Playing audio using timer interrupts...\n");
    
    // Set up signal handler for timer
    struct sigaction sa;
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = timer_handler;
    sigemptyset(&sa.sa_mask);
    
    if (sigaction(SIGRTMIN, &sa, NULL) == -1) {
        perror("sigaction");
        free(audio_buffer);
        return 1;
    }
    
    // Create a POSIX timer
    timer_t timerid;
    struct sigevent sev;
    
    sev.sigev_notify = SIGEV_SIGNAL;
    sev.sigev_signo = SIGRTMIN;
    sev.sigev_value.sival_ptr = &timerid;
    
    if (timer_create(CLOCK_MONOTONIC, &sev, &timerid) == -1) {
        perror("timer_create");
        free(audio_buffer);
        return 1;
    }
    
    // Configure timer for sample rate
    long interval_ns = 1000000000L / header.sample_rate;
    
    struct itimerspec timer_spec;
    timer_spec.it_value.tv_sec = 0;
    timer_spec.it_value.tv_nsec = interval_ns;  // Initial expiration
    timer_spec.it_interval.tv_sec = 0;
    timer_spec.it_interval.tv_nsec = interval_ns;  // Periodic interval
    
    if (timer_settime(timerid, 0, &timer_spec, NULL) == -1) {
        perror("timer_settime");
        timer_delete(timerid);
        free(audio_buffer);
        return 1;
    }
    
    // Wait for playback to complete
    while (!playback_complete) {
        pause();  // Wait for signals
    }
    
    printf("Playback complete!\n");
    
    timer_delete(timerid);
    free(audio_buffer);
    gpiod_line_release(led);
    gpiod_chip_close(chip);
    return 0;
}
