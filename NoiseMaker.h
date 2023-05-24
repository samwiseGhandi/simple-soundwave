#include <iostream>
#include <cmath>
#include <fstream>
#include <vector>
#include <string>
#include <thread>
#include <atomic>
#include <condition_variable>
#include <alsa/asoundlib.h>
using namespace std;

const double PI = 2.0 * acos(0.0);

template<class T>
class NoiseMaker
{
public:
    NoiseMaker(unsigned int nSampleRate = 44100, unsigned int nChannels = 1, unsigned int nBlocks = 8, unsigned int nBlockSamples = 512)
    {
        Create(nSampleRate, nChannels, nBlocks, nBlockSamples);
    }

    ~NoiseMaker()
    {
        Destroy();
    }

    bool Create(unsigned int nSampleRate = 44100, unsigned int nChannels = 1, unsigned int nBlocks = 8, unsigned int nBlockSamples = 512)
    {
        m_bReady = false;
        m_nSampleRate = nSampleRate;
        m_nChannels = nChannels;
        m_nBlockCount = nBlocks;
        m_nBlockSamples = nBlockSamples;
        m_nBlockFree = m_nBlockCount;
        m_nBlockCurrent = 0;
        m_pBlockMemory = nullptr;

        m_userFunction = nullptr;

        m_pBlockMemory = new T[m_nBlockCount * m_nBlockSamples];
        if (m_pBlockMemory == nullptr)
            return Destroy();
        memset(m_pBlockMemory, 0, sizeof(T) * m_nBlockCount * m_nBlockSamples);

        if (snd_pcm_open(&m_alsaHandle, "default", SND_PCM_STREAM_PLAYBACK, 0) < 0)
        {
            cout << "Failed to open PCM device" << endl;
            return Destroy();
        }

        if (snd_pcm_set_params(m_alsaHandle, SND_PCM_FORMAT_S16_LE, SND_PCM_ACCESS_RW_INTERLEAVED, m_nChannels, m_nSampleRate, 1, 500000) < 0)
        {
            cout << "Failed to set PCM parameters" << endl;
            return Destroy();
        }

        m_bReady = true;

        m_thread = thread(&olcNoiseMaker::MainThread, this);

        unique_lock<mutex> lm(m_muxBlockNotZero);
        m_cvBlockNotZero.notify_one();

        return true;
    }

    bool Destroy()
    {
        if (m_bReady)
        {
            m_bReady = false;
            m_thread.join();
        }

        if (m_alsaHandle)
        {
            snd_pcm_close(m_alsaHandle);
            m_alsaHandle = nullptr;
        }

        if (m_pBlockMemory)
        {
            delete[] m_pBlockMemory;
            m_pBlockMemory = nullptr;
        }

        return true;
    }

    void Stop()
    {
        m_bReady = false;
        m_thread.join();
    }

    virtual double UserProcess(double dTime)
    {
        return 0.0;
    }

    double GetTime()
    {
        return m_dGlobalTime;
    }

public:
    void SetUserFunction(double(*func)(double))
    {
        m_userFunction = func;
    }

    double Clip(double dSample, double dMax)
    {
        if (dSample >= 0.0)
            return fmin(dSample, dMax);
        else
            return fmax(dSample, -dMax);
    }

private:
    static void AlsaCallback(snd_async_handler_t* handler)
    {
        olcNoiseMaker* this_ptr = static_cast<olcNoiseMaker*>(snd_async_handler_get_callback_private(handler));
        this_ptr->waveOutProc();
    }

    void waveOutProc()
    {
        m_nBlockFree++;
        unique_lock<mutex> lm(m_muxBlockNotZero);
        m_cvBlockNotZero.notify_one();
    }

    void MainThread()
    {
        m_dGlobalTime = 0.0;
        double dTimeStep = 1.0 / (double)m_nSampleRate;

        T nMaxSample = (T)pow(2, (sizeof(T) * 8) - 1) - 1;
        double dMaxSample = (double)nMaxSample;
        T nPreviousSample = 0;

        snd_pcm_hw_params_t* hw_params;
        snd_pcm_hw_params_alloca(&hw_params);
        snd_pcm_uframes_t frames = m_nBlockSamples;
        snd_pcm_sframes_t delay;

        while (m_bReady)
        {
            if (m_nBlockFree == 0)
            {
                unique_lock<mutex> lm(m_muxBlockNotZero);
                m_cvBlockNotZero.wait(lm);
            }

            m_nBlockFree--;

            T nNewSample = 0;
            int nCurrentBlock = m_nBlockCurrent * m_nBlockSamples;

            for (unsigned int n = 0; n < m_nBlockSamples; n++)
            {
                if (m_userFunction == nullptr)
                    nNewSample = (T)(Clip(UserProcess(m_dGlobalTime), 1.0) * dMaxSample);
                else
                    nNewSample = (T)(Clip(m_userFunction(m_dGlobalTime), 1.0) * dMaxSample);

                m_pBlockMemory[nCurrentBlock + n] = nNewSample;
                nPreviousSample = nNewSample;
                m_dGlobalTime = m_dGlobalTime + dTimeStep;
            }

            m_nBlockCurrent++;
            m_nBlockCurrent %= m_nBlockCount;

            if (snd_pcm_writei(m_alsaHandle, &m_pBlockMemory[nCurrentBlock], frames) < 0)
            {
                snd_pcm_recover(m_alsaHandle, snd_pcm_state(m_alsaHandle), 0);
                cout << "Failed to write audio to PCM device" << endl;
                m_bReady = false;
            }

            snd_pcm_delay(m_alsaHandle, &delay);
            if (delay > 0)
            {
                frames = delay;
            }
            else
            {
                frames = m_nBlockSamples;
            }
        }
    }

private:
    atomic<bool> m_bReady;
    thread m_thread;
    condition_variable m_cvBlockNotZero;
    mutex m_muxBlockNotZero;

    double m_dGlobalTime;
    unsigned int m_nSampleRate;
    unsigned int m_nChannels;
    unsigned int m_nBlockCount;
    unsigned int m_nBlockSamples;
    int m_nBlockCurrent;
    int m_nBlockFree;
    T* m_pBlockMemory;
    double(*m_userFunction)(double);

    snd_pcm_t* m_alsaHandle;
};

double MyFunction(double dTime)
{
    return sin(440.0 * 2.0 * PI * dTime);
}

int main()
{
    NoiseMaker<int16_t> noiseMaker;
    noiseMaker.SetUserFunction(MyFunction);

    cout << "Press Enter to exit..." << endl;
    cin.ignore();

    return 0;
}
