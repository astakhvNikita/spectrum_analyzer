#include <iostream>

#include <SFML/Audio/SoundBufferRecorder.hpp>
#include <SFML/Graphics.hpp>

#include "dsp.h"

#define LOG_SCALE

#define FFT_LENGTH          (1024)
#define FFT_LENGTH_LOG      (10)
#define SMA_FILT_ORDER_M    (1)
#define SMA_FILT_ORDER      (10 * SMA_FILT_ORDER_M)
#define SMA_FILT_PEAK_ORDER (38 * SMA_FILT_ORDER_M)

sf::Mutex mutex;

std::vector<std::complex<double>> fftOut;

class CustomRecorder : public sf::SoundRecorder
{
public:
    bool onStart() override
    {
        setProcessingInterval(sf::milliseconds(10));

        return true;
    }

    bool onProcessSamples(const sf::Int16* samples, std::size_t sampleCount) override
    {
        std::cout << "onProcessSamples: sampleCount = " << sampleCount << std::endl;

        while (sampleCount--)
        {
            sampleBuf.push_back(*samples++);
            if (sampleBuf.size() != FFT_LENGTH)
                continue;

            if (fftIn.size() != sampleBuf.size())
                fftIn.resize(sampleBuf.size());
            if (fftOut.size() != sampleBuf.size())
                fftOut.resize(sampleBuf.size());

            for (int i = 0; i < sampleBuf.size(); i++)
            {
                fftIn[i].real(sampleBuf[i]);
                fftIn[i].imag(0.0);
            }

            mutex.lock();
            dsp::DSP::fft(fftIn, fftOut);
            mutex.unlock();
            sampleBuf.clear();

            std::cout << "onProcessSamples: fft result updated" << std::endl;
        }

        return true;
    }

private:
    std::vector<int16_t> sampleBuf;

    std::vector<std::complex<double>> fftIn;
};

int main()
{
#ifdef LOG_SCALE
    sf::RenderWindow window(sf::VideoMode(FFT_LENGTH_LOG * 32, 240),
        "Spectrum Analyzer");
#else
    sf::RenderWindow window(sf::VideoMode(FFT_LENGTH * 2, 240),
        "Spectrum Analyzer");
#endif
    sf::RectangleShape mean, peak;
    std::vector<int16_t> fft(FFT_LENGTH);
    std::vector<int16_t> fftPeak(FFT_LENGTH);
    std::vector<std::vector<uint16_t>> fftHistory(FFT_LENGTH);
    std::vector<std::vector<uint16_t>> fftPeakHistory(FFT_LENGTH);

    sf::Vector2u windowSize = window.getSize();
    window.setFramerateLimit(60);
    window.setVerticalSyncEnabled(true);

    mean.setFillColor(sf::Color::Green);
    peak.setFillColor(sf::Color::Red);

    CustomRecorder recorder;
    if (recorder.isAvailable())
    {
        std::vector<std::string> devices =
            sf::SoundBufferRecorder::getAvailableDevices();

        window.setTitle("Spectrum Analyzer: " + devices[0]);

        recorder.start();
    }

    for (int i = 0; i < fftHistory.size(); i++)
        fftHistory[i].resize(SMA_FILT_ORDER + 1);
    for (int i = 0; i < fftPeakHistory.size(); i++)
        fftPeakHistory[i].resize(SMA_FILT_PEAK_ORDER + 1);

    while (window.isOpen())
    {
        sf::Event event;
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
                window.close();
        }

        window.clear();

        mutex.lock();
        if (fftOut.size() == FFT_LENGTH)
        {
            for (int i = 0; i < FFT_LENGTH; i++)
            {
                int16_t point = 20 * std::log10(std::abs(fftOut[i]) /
                    (FFT_LENGTH / 2) / INT16_MAX) + 60;
                if (point < 0)
                    point = 0;
                point *= 4;
                fft[i] = dsp::DSP::sma_filt(point, SMA_FILT_ORDER,
                    fftHistory[i]);
                fftPeak[i] = dsp::DSP::sma_filt(point, SMA_FILT_PEAK_ORDER,
                    fftPeakHistory[i], true);
            }
        }
        mutex.unlock();
#ifdef LOG_SCALE
        for (int i = 1, x = 0; i <= fft.size() / 2; i = i * 2, x++)
        {
            if (i == fft.size() / 2)
                i = 20000 / ((recorder.getSampleRate() / 2) / (fft.size() / 2));
            peak.setPosition(sf::Vector2f(x * (windowSize.x / FFT_LENGTH_LOG),
                windowSize.y - fftPeak[i]));
            peak.setSize(sf::Vector2f(windowSize.x / FFT_LENGTH_LOG - 1, 10));
            window.draw(peak);

            mean.setPosition(sf::Vector2f(x * (windowSize.x / FFT_LENGTH_LOG),
                windowSize.y - fft[i]));
            mean.setSize(sf::Vector2f(windowSize.x / FFT_LENGTH_LOG - 1,
                fft[i]));
            window.draw(mean);
        }
#else
        for (int i = 0; i < fft.size() / 2; i++)
        {
            peak.setPosition(sf::Vector2f(i * (windowSize.x / (FFT_LENGTH / 2)),
                windowSize.y - fftPeak[i]));
            peak.setSize(sf::Vector2f(windowSize.x / (FFT_LENGTH / 2) - 1,
                windowSize.x / (FFT_LENGTH / 2) - 1));
            window.draw(peak);

            mean.setPosition(sf::Vector2f(i * (windowSize.x / (FFT_LENGTH / 2)),
                windowSize.y - fft[i]));
            mean.setSize(sf::Vector2f(windowSize.x / (FFT_LENGTH / 2) - 1,
                fft[i]));
            window.draw(mean);
        }
#endif

        window.display();

        sf::sleep(sf::Time(sf::milliseconds(10)));
    }

    return 0;
}
