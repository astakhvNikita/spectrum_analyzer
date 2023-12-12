#include <iostream>

#include <SFML/Audio/SoundBufferRecorder.hpp>
#include <SFML/Graphics.hpp>

#include <fftw3.h>

#include "dsp.h"

#define FFT_LENGTH      (256)
#define SMA_FILT_ORDER  (15)

sf::Mutex mutex;

std::vector<int16_t> fft(256);
std::vector<std::vector<uint16_t>> filtHistory(256);

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

            dsp::DSP::fft(fftIn, fftOut);
            mutex.lock();
            for (int i = 0; i < sampleBuf.size(); i++)
            {
                fft[i] = 20 * std::log10(std::abs(fftOut[i]) /
                    (sampleBuf.size() / 2) / INT16_MAX) + 60;
                if (fft[i] < 0)
                    fft[i] = 0;
                fft[i] *= 4;
                fft[i] = dsp::DSP::sma_filt(fft[i], SMA_FILT_ORDER,
                    filtHistory[i]);
            }
            mutex.unlock();
            sampleBuf.clear();

            std::cout << "onProcessSamples: fft result updated" << std::endl;
        }

        return true;
    }

private:
    std::vector<int16_t> sampleBuf;

    std::vector<std::complex<double>> fftIn;
    std::vector<std::complex<double>> fftOut;
};

int main()
{
    sf::RenderWindow window(sf::VideoMode(FFT_LENGTH * 2, 240),
        "Spectrum Analyzer");
    sf::RectangleShape line;

    sf::Vector2u windowSize = window.getSize();
    window.setFramerateLimit(60);
    window.setVerticalSyncEnabled(true);

    line.setFillColor(sf::Color::Green);

    CustomRecorder recorder;
    if (recorder.isAvailable())
    {
        std::vector<std::string> devices =
            sf::SoundBufferRecorder::getAvailableDevices();

        window.setTitle("Spectrum Analyzer: " + devices[0]);

        recorder.start();
    }

    for (int i = 0; i < filtHistory.size(); i++)
        filtHistory[i].resize(SMA_FILT_ORDER + 1);

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
        for (int i = 0; i < fft.size() / 2; i++)
        {
            line.setPosition(sf::Vector2f(i * (windowSize.x / (FFT_LENGTH / 2)),
                windowSize.y - fft[i]));
            line.setSize(sf::Vector2f(windowSize.x / (FFT_LENGTH / 2) - 1, fft[i]));
            window.draw(line);
        }
        mutex.unlock();

        window.display();

        sf::sleep(sf::Time(sf::milliseconds(10)));
    }

    return 0;
}
