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

    void playSoundWave(double frequency, double amplitude, double duration)
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

#elif defined(_WIN32)
    #include <Windows.h>

    void playSoundWave(double frequency, double amplitude, double duration)
    {
        // Generate audio samples
        std::vector<short> samples;
        for (double t = 0.0; t < DURATION; t += 1.0 / SAMPLE_RATE)
        {
            double sample = AMPLITUDE * std::sin(2 * M_PI * FREQUENCY * t);
            samples.push_back(static_cast<short>(sample * SHRT_MAX));
        }

        // Play audio
        PlaySound(reinterpret_cast<LPCWSTR>(&samples[0]), NULL, SND_MEMORY | SND_ASYNC);
    }

#elif defined(__APPLE__)
    #include <CoreAudio/CoreAudio.h>

    void playSoundWave(double frequency, double amplitude, double duration)
    {
        // Generate audio samples
        std::vector<float> samples;
        for (double t = 0.0; t < DURATION; t += 1.0 / SAMPLE_RATE)
        {
            double sample = AMPLITUDE * std::sin(2 * M_PI * FREQUENCY * t);
            samples.push_back(static_cast<float>(sample));
        }

        // Get the default audio output device
        AudioDeviceID outputDeviceID;
        UInt32 size = sizeof(outputDeviceID);
        AudioObjectPropertyAddress propertyAddress = { kAudioHardwarePropertyDefaultOutputDevice,
                                                       kAudioObjectPropertyScopeGlobal,
                                                       kAudioObjectPropertyElementMaster };
        AudioObjectGetPropertyData(kAudioObjectSystemObject, &propertyAddress, 0, nullptr, &size, &outputDeviceID);

        // Set up audio stream description
        AudioStreamBasicDescription streamFormat;
        streamFormat.mSampleRate = SAMPLE_RATE;
        streamFormat.mFormatID = kAudioFormatLinearPCM;
        streamFormat.mFormatFlags = kAudioFormatFlagsNativeFloatPacked;
        streamFormat.mBytesPerPacket = sizeof(float);
        streamFormat.mFramesPerPacket = 1;
        streamFormat.mBytesPerFrame = sizeof(float);
        streamFormat.mChannelsPerFrame = 1;
        streamFormat.mBitsPerChannel = sizeof(float) * 8;
        streamFormat.mReserved = 0;

        // Open the default output device for playback
        AudioDeviceIOProcID procID;
        AudioDeviceOpen(outputDeviceID, 0, &streamFormat, 0, nullptr, &procID);

        // Start audio playback
        AudioDeviceStart(outputDeviceID, procID);

        // Write audio samples to the output device
        for (const auto& sample : samples)
        {
            AudioDeviceWrite(outputDeviceID, 0, nullptr, &streamFormat, 1, &sample);
        }

        // Stop audio playback and close the output device
        AudioDeviceStop(outputDeviceID, procID);
        AudioDeviceClose(outputDeviceID);
    }

#else
    #error "Unsupported platform"

#endif

int main()
{
    // Play the sound
    std::cout << "Sound playing..." << std::endl;
    playSoundWave(440.0, 0.3, 3.0);

    // Wait for the sound to finish playing
    std::this_thread::sleep_for(std::chrono::duration<double>(DURATION));

    std::cout << "Sound finished playing." << std::endl;
    return 0;
}