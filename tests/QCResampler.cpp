#include "sfizz/MathHelpers.h"
#include <sndfile.hh>
#include <soxr.h>
#include <memory>
#include <algorithm>
#include <cstdlib>
#include <cmath>

struct InterleavedAudio {
    std::unique_ptr<float[]> data;
    size_t frames = 0;
    unsigned channels = 0;
};

struct MonoAudio {
    std::unique_ptr<float[]> data;
    size_t frames = 0;
};

static constexpr size_t excessInputFrames = 8; // extra zeros for interpolating

static MonoAudio resampleSoxVhq(const MonoAudio& input, double oldRate, double newRate);
static MonoAudio resampleSfizzLinear(const MonoAudio& input, double oldRate, double newRate);
static MonoAudio resampleSfizzHermite(const MonoAudio& input, double oldRate, double newRate);

static double signalNoiseRatio(const MonoAudio& testSig, const MonoAudio& refSig);

int main(int argc, char* argv[])
{
    if (argc != 3) {
        fprintf(stderr, "Usage: qc_resampler <audio-file> <resample-ratio>\n");
        return 1;
    }

    const char *filePath = argv[1];
    double ratio = std::atof(argv[2]);

    if (ratio <= 0.0) {
        fprintf(stderr, "The resampling ratio is invalid.\n");
        return 1;
    }

    InterleavedAudio fileAudio;
    double fileRate = 0;

    {
        SndfileHandle sndFile(filePath);
        if (!sndFile) {
            fprintf(stderr, "The sound file cannot be read.\n");
            return 1;
        }
        fileAudio.frames = sndFile.frames();
        fileAudio.channels = sndFile.channels();
        fileAudio.data.reset(new float[fileAudio.channels * fileAudio.frames + excessInputFrames]{});
        fileRate = sndFile.samplerate();
        sndFile.readf(fileAudio.data.get(), fileAudio.frames);
    }

    for (unsigned c = 0; c < fileAudio.channels; ++c) {
        printf("* Channel %d\n", c + 1);

        MonoAudio channel;
        channel.frames = fileAudio.frames;
        channel.data.reset(new float[fileAudio.frames + excessInputFrames]{});

        for (size_t i = 0; i < fileAudio.frames; ++i)
            channel.data[i] = fileAudio.data[i * fileAudio.channels + c];

        const MonoAudio refResampledAudio = resampleSoxVhq(channel, fileRate, fileRate * ratio);
        const MonoAudio linearAudio = resampleSfizzLinear(channel, fileRate, fileRate * ratio);
        const MonoAudio hermiteAudio = resampleSfizzHermite(channel, fileRate, fileRate * ratio);

#if 0
        const double resampledRate = fileRate * ratio;

        auto writeSoundFile = [resampledRate](const std::string& filename, const MonoAudio& audio) {
            SndfileHandle sndFile(
                (filename + ".aiff").c_str(),
                SFM_WRITE, SF_FORMAT_AIFF|SF_FORMAT_FLOAT,
                1, resampledRate);
            sndFile.writef(audio.data.get(), audio.frames);
        };

        writeSoundFile("reference" + std::to_string(c + 1), refResampledAudio);
        writeSoundFile("linear" + std::to_string(c + 1), linearAudio);
        writeSoundFile("hermite" + std::to_string(c + 1), hermiteAudio);
#endif

        /// SNR
        printf("SNR linear: %f dB RMS\n", mag2db(signalNoiseRatio(linearAudio, refResampledAudio)));
        printf("SNR hermite: %f dB RMS\n", mag2db(signalNoiseRatio(hermiteAudio, refResampledAudio)));
    }

    return 0;
}

static MonoAudio resampleSoxVhq(const MonoAudio& input, double oldRate, double newRate)
{
    const double ratio = newRate / oldRate;

    MonoAudio output;
    output.frames = static_cast<size_t>(std::ceil(input.frames * ratio));
    output.data.reset(new float[output.frames]{});

    soxr_io_spec_t io_spec = soxr_io_spec(SOXR_FLOAT32_I, SOXR_FLOAT32_I);
    soxr_quality_spec_t quality_spec = soxr_quality_spec(SOXR_VHQ, 0);
    soxr_runtime_spec_t runtime_spec = soxr_runtime_spec(2);

    size_t idone = 0;
    size_t odone = 0;
    soxr_error_t err = soxr_oneshot(
        oldRate, newRate, 1,
        input.data.get(), input.frames, &idone,
        output.data.get(), output.frames, &odone,
        &io_spec, &quality_spec, &runtime_spec);

    if (err)
        throw std::runtime_error("SoX resampler error");

    return output;
}

static MonoAudio resampleSfizzLinear(const MonoAudio& input, double oldRate, double newRate)
{
    const double ratio = newRate / oldRate;

    MonoAudio output;
    output.frames = static_cast<size_t>(std::ceil(input.frames * ratio));
    output.data.reset(new float[output.frames]{});

    for (uint32_t iOut = 0; iOut < output.frames; ++iOut) {
        float posIn = iOut * static_cast<float>(1.0 / ratio);
        unsigned dec = static_cast<int>(posIn);
        float frac = posIn - dec;
        output.data[iOut] = linearInterpolation(&input.data[dec], frac);
    }

    return output;
}

static MonoAudio resampleSfizzHermite(const MonoAudio& input, double oldRate, double newRate)
{
    const double ratio = newRate / oldRate;

    MonoAudio output;
    output.frames = static_cast<size_t>(std::ceil(input.frames * ratio));
    output.data.reset(new float[output.frames]{});

    // latency compensation against SoX resampler (approximate)
    constexpr size_t shift = 1;
    std::unique_ptr<float[]> shiftedIn { new float[input.frames + shift]{} };
    std::copy(&input.data[0], &input.data[input.frames], &shiftedIn[shift]);

    for (uint32_t iOut = 0; iOut < output.frames; ++iOut) {
        float posIn = iOut * static_cast<float>(1.0 / ratio);
        unsigned dec = static_cast<int>(posIn);
        float frac = posIn - dec;
        output.data[iOut] = hermiteInterpolation(&shiftedIn[dec], frac);
    }

    return output;
}

static double signalNoiseRatio(const MonoAudio& testSig, const MonoAudio& refSig)
{
    // cf. https://www.earlevel.com/main/2018/09/22/wavetable-signal-to-noise-ratio/

    size_t frames = std::min(testSig.frames, refSig.frames);

    double sigPower = 0.0;
    double errPower = 0.0;

    for (size_t i = 0; i < frames; ++i) {
        double sig = testSig.data[i];
        double err = refSig.data[i] - sig;
        sigPower += sig * sig;
        errPower += err * err;
    }

    sigPower = std::sqrt(sigPower / frames);
    errPower = std::sqrt(errPower / frames);

    return sigPower / errPower;
}
