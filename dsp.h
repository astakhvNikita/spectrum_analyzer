#pragma once

#include <complex>
#include <vector>

namespace dsp
{
    static class DSP
    {
    public:
        DSP() = default;
        ~DSP() = default;

        static int dft(std::vector<std::complex<double>>& in,
            std::vector<std::complex<double>>& out);
        static int fft(std::vector<std::complex<double>>& in,
            std::vector<std::complex<double>>& out, bool useWindow = false);

        static int idft(std::vector<std::complex<double>>& in,
            std::vector<std::complex<double>> &out) { return 0; };
        static int ifft(std::vector<std::complex<double>>& in,
            std::vector<std::complex<double>> &out) { return 0; };

        static uint16_t sma_filt(uint16_t in, int order,
            std::vector<uint16_t>& history, bool peak = false);

    private:
        static std::vector<std::complex<double>> _fft(
            std::vector<std::complex<double>>& in);
        static std::complex<double> _wnk(int k, int N);
        static void _blackmanWindow(std::vector<std::complex<double>>& inOut);
        static void _hammingWindow(std::vector<std::complex<double>>& inOut);
    };
}
