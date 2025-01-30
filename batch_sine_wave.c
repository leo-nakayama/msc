#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sndfile.h>
#include <string.h>

#define SAMPLE_RATE 44100  // Standard audio sample rate
#define AMPLITUDE 32760    // Max amplitude for 16-bit PCM
#define MAX_LINE_LENGTH 1024
#define MAX_RHYTHM_SEGMENTS 100

// Function to generate a sine wave
void generate_sine_wave(short *buffer, double frequency, double duration, int sample_rate) {
    int num_samples = (int)(duration * sample_rate);
    for (int i = 0; i < num_samples; i++) {
        buffer[i] = (short)(AMPLITUDE * sin(2 * M_PI * frequency * i / sample_rate));
    }
}

// Function to generate silence
void generate_silence(short *buffer, double duration, int sample_rate) {
    int num_samples = (int)(duration * sample_rate);
    for (int i = 0; i < num_samples; i++) {
        buffer[i] = 0;  // Silence (zero amplitude)
    }
}

// Function to process a single row from the CSV file
int process_csv_row(char *line, short **output_buffer, int *total_samples) {
    double frequency;
    double rhythm[MAX_RHYTHM_SEGMENTS];
    int num_segments = 0;

    // Tokenize the CSV row
    char *token = strtok(line, ",");
    if (token == NULL) return 0;

    // Read frequency
    frequency = atof(token);

    // Read rhythm pattern
    while ((token = strtok(NULL, ",")) != NULL && num_segments < MAX_RHYTHM_SEGMENTS) {
        rhythm[num_segments++] = atof(token);
    }

    // Calculate total samples needed
    *total_samples = 0;
    for (int i = 0; i < num_segments; i++) {
        *total_samples += (int)(rhythm[i] * SAMPLE_RATE);
    }

    // Allocate memory for this segment
    *output_buffer = (short *)malloc(*total_samples * sizeof(short));
    if (*output_buffer == NULL) {
        printf("Memory allocation failed.\n");
        return 0;
    }

    // Generate waveform based on the rhythm pattern
    int buffer_index = 0;
    for (int i = 0; i < num_segments; i++) {
        if (i % 2 == 0) {  // Sound
            generate_sine_wave(&(*output_buffer)[buffer_index], frequency, rhythm[i], SAMPLE_RATE);
        } else {  // Silence
            generate_silence(&(*output_buffer)[buffer_index], rhythm[i], SAMPLE_RATE);
        }
        buffer_index += (int)(rhythm[i] * SAMPLE_RATE);
    }

    return 1;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <csv_file>\n", argv[0]);
        return 1;
    }

    char *csv_filename = argv[1];
    FILE *csv_file = fopen(csv_filename, "r");
    if (!csv_file) {
        printf("Error: Cannot open CSV file %s\n", csv_filename);
        return 1;
    }

    // Prepare WAV file output
    SF_INFO sf_info;
    sf_info.samplerate = SAMPLE_RATE;
    sf_info.channels = 1;  // Mono
    sf_info.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;

    SNDFILE *sndfile = sf_open("batch_output.wav", SFM_WRITE, &sf_info);
    if (!sndfile) {
        printf("Error: Cannot open output WAV file.\n");
        fclose(csv_file);
        return 1;
    }

    char line[MAX_LINE_LENGTH];
    while (fgets(line, sizeof(line), csv_file)) {
        short *buffer;
        int total_samples;

        // Process the current CSV row
        if (process_csv_row(line, &buffer, &total_samples)) {
            // Write to WAV file
            sf_write_short(sndfile, buffer, total_samples);
            free(buffer);
        }
    }

    // Cleanup
    sf_close(sndfile);
    fclose(csv_file);

    printf("WAV file 'batch_output.wav' created successfully.\n");
    return 0;
}
