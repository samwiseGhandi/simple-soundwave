#include <iostream>
#include <cmath>
#include <thread>
#include <chrono>
#include <atomic>
#include "NoiseMaker.h"

constexpr double SAMPLE_RATE = 44100.0;
constexpr double DURATION = 5.0;
constexpr double FREQUENCY = 440.0;
constexpr double AMPLITUDE = 0.3;

// using namespace std;

std::atomic<double> dFrequencyOut = 0.0;             //the note
double dOctaveBaseFrequency = 110.0; //A2       //freq of octave
double d12thRootOf2 = pow(2.0,1.0/12.0);        //assuming western 12 notes per octave

double makeSound(double dTime){
    double masterAmp = 0.5;
    double dOutPut = sin(dFrequencyOut * 2.0 * M_PI * dTime);
    return dOutPut * masterAmp; //To controle master volume
}

#if defined(__linux__)
#include <ao/ao.h>

void printKeyboard()
{
    std::string keyboard_layout = R"(
    |   |   | |   |   |   |   | |   | |   |   |   |   |   |
    |   | W | | E |   |   | T | | Y | | U |   |   | O |   |
    |   |___| |___|   |   |___| |___| |___|   |   |___|   |
    |     |     |     |     |     |     |     |     |     |
    |  A  |  S  |  D  |  F  |  G  |  H  |  J  |  K  |  L  |
    |_____|_____|_____|_____|_____|_____|_____|_____|_____|
    
    Use Z and X to adjust the octave.
    )";
        
    std::cout << "Sound playing..." << std::endl;

    std::cout << keyboard_layout << std::endl;
}

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
    ao_device *device = ao_open_live(ao_default_driver_id(), &format, nullptr);
    if (device == nullptr)
    {
        std::cerr << "Error: Failed to open audio device." << std::endl;
        return;
    }

    // Generate and play audio samples
    for (double t = 0.0; t < duration; t += 1.0 / SAMPLE_RATE)
    {
        double sample = amplitude * std::sin(2 * M_PI * frequency * t);
        int16_t sampleValue = static_cast<int16_t>(sample * INT16_MAX);
        ao_play(device, reinterpret_cast<char *>(&sampleValue), sizeof(int16_t));
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

    printKeyboard();

    // Play the soundd
    std::cout << "Sound playing..." << std::endl;
    playSoundWave(440.0, 0.3, 3.0);

    // Wait for the sound to finish playing
    std::this_thread::sleep_for(std::chrono::duration<double>(DURATION));

    std::cout << "Sound finished playing." << std::endl;
    return 0;
    
}
