#include <iostream>
#include <cmath>
#include <thread>
#include <chrono>

constexpr double SAMPLE_RATE = 44100.0;
constexpr double DURATION = 5.0;
constexpr double FREQUENCY = 440.0;
constexpr double AMPLITUDE = 0.3;

#if defined(__linux__)
    #include <ao/ao.h>

    void playSoundWave()
    {
        // Initialize audio output
        ao_initialize();

        // Set up audio format
        ao_sample_format format;
        format.bits = 16;
        format.channels = 1;
        format.rate = static_cast<int>(SAMPLE_RATE);
        format.byte_format = AO_FMT_LITTLE;

        // Open default driver for playback
        ao_device* device = ao_open_live(ao_default_driver_id(), &format, nullptr);
        if (device == nullptr)
        {
            std::cerr << "Error: Failed to open audio device." << std::endl;
            return;
        }

        // Generate and play audio samples
        for (double t = 0.0; t < DURATION; t += 1.0 / SAMPLE_RATE)
        {
            double sample = AMPLITUDE * std::sin(2 * M_PI * FREQUENCY * t);
            int16_t sampleValue = static_cast<int16_t>(sample * INT16_MAX);
            ao_play(device, reinterpret_cast<char*>(&sampleValue), sizeof(int16_t));
        }

        // Close the audio device
        ao_close(device);

        // Shutdown audio output
        ao_shutdown();
    }

#else
    #error "Unsupported platform"

#endif

int main()
{
    // Play the sound
    std::cout << "Sound playing..." << std::endl;
    playSoundWave();

    // Wait for the sound to finish playing
    std::this_thread::sleep_for(std::chrono::duration<double>(DURATION));

    std::cout << "Sound finished playing." << std::endl;
    return 0;
}
