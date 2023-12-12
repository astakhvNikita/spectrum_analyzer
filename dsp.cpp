#include "dsp.h"

#define _USE_MATH_DEFINES
#include <math.h>

namespace dsp
{
    int DSP::dft(std::vector<std::complex<double>>& in,
        std::vector<std::complex<double>>& out)
    {
        int N = in.size();
        std::complex<double> power{};

        if (in.size() != out.size())
            return -1;

        for (int k = 0; k < N; k++)
        {
            out[k] = std::complex<double>( 0.0, 0.0 );
            for (int n = 0; n < N; n++)
            {
                power = std::complex<double>(0, -2 * M_PI * n * k / N);
                out[k] += in[n] * std::exp(power);
            }
        }

        return N;
    }

    int DSP::fft(std::vector<std::complex<double>>& in,
        std::vector<std::complex<double>>& out, bool useWindow)
    {
        if (in.size() != out.size())
            return -1;

        if (useWindow)
            _hammingWindow(in);
        out = _fft(in);

        return in.size();
    }

    uint16_t DSP::sma_filt(uint16_t in, int order, std::vector<uint16_t>& history)
    {
        int i;
        uint32_t sum;

        for (i = order - 1; i > 0; i--)
            history[i] = history[i - 1];
        history[0] = in;

        sum = 0;
        for (i = 0; i < order; i++)
            sum += history[i];
        sum = sum * 100 / (order * 100);

        history[order] = sum;

        return (uint16_t)sum;
    }

    std::vector<std::complex<double>> DSP::_fft(
        std::vector<std::complex<double>>& in)
    {
        int N = in.size();
        std::vector<std::complex<double>> X{};

        if (N == 2)
        {
            X.resize(2);
            X[0] = in[0] + in[1];
            X[1] = in[0] - in[1];
        }
        else
        {
            std::vector<std::complex<double>> xEven(N / 2);
            std::vector<std::complex<double>> xOdd(N / 2);
            for (int n = 0; n < N / 2; n++)
            {
                xEven[n] = in[2 * n];
                xOdd[n] = in[2 * n + 1];
            }

            std::vector<std::complex<double>> XEven = _fft(xEven);
            std::vector<std::complex<double>> XOdd = _fft(xOdd);
            X = std::vector<std::complex<double>>(N);
            for (int k = 0; k < N / 2; k++)
            {
                X[k] = XEven[k] + _wnk(k, N) * XOdd[k];
                X[k + N / 2] = XEven[k] - _wnk(k, N) * XOdd[k];
            }
        }

        return X;
    }

    std::complex<double> DSP::_wnk(int k, int N)
    {
        if (k % N == 0)
            return 1;

        return std::complex<double>(std::cos(-2 * M_PI * k / N),
            std::sin(-2 * M_PI * k / N));
    }

    void DSP::_blackmanWindow(std::vector<std::complex<double>>& inOut)
    {
        int N = inOut.size() - 1;

        for (int i = 0; i <= N; i++)
            inOut[i] = inOut[i] * (0.42 - 0.5 * std::cos(2 * M_PI * i / N) +
                0.08 * std::cos(4 * M_PI * i / N));
    }

    void DSP::_hammingWindow(std::vector<std::complex<double>>& inOut)
    {
        int N = inOut.size() - 1;

        for (int i = 0; i <= N; i++)
            inOut[i] = inOut[i] * (0.54 - 0.46 * std::cos(2 * M_PI * i / N));
    }
}
