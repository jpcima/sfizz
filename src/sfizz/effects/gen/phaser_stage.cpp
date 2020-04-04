/* ------------------------------------------------------------
name: "phaser_stage"
Code generated with Faust 2.20.2 (https://faust.grame.fr)
Compilation options: -lang cpp -inpl -scal -ftz 0
------------------------------------------------------------ */

#ifndef __faustPhaserStage_H__
#define __faustPhaserStage_H__

#ifndef FAUSTFLOAT
#define FAUSTFLOAT float
#endif

#include <algorithm>
#include <cmath>
#include <math.h>

static float faustPhaserStage_faustpower2_f(float value)
{
    return (value * value);
}

#ifndef FAUSTCLASS
#define FAUSTCLASS faustPhaserStage
#endif

#ifdef __APPLE__
#define exp10f __exp10f
#define exp10 __exp10
#endif

class faustPhaserStage {

private:
    int fSampleRate;
    float fConst0;
    float fConst1;
    float fConst2;
    float fRec2[3];
    float fConst3;
    float fRec1[3];
    float fRec0[2];

public:
    int getNumInputs()
    {
        return 4;
    }
    int getNumOutputs()
    {
        return 1;
    }
    int getInputRate(int channel)
    {
        int rate;
        switch ((channel)) {
        case 0: {
            rate = 1;
            break;
        }
        case 1: {
            rate = 1;
            break;
        }
        case 2: {
            rate = 1;
            break;
        }
        case 3: {
            rate = 1;
            break;
        }
        default: {
            rate = -1;
            break;
        }
        }
        return rate;
    }
    int getOutputRate(int channel)
    {
        int rate;
        switch ((channel)) {
        case 0: {
            rate = 1;
            break;
        }
        default: {
            rate = -1;
            break;
        }
        }
        return rate;
    }

    static void classInit(int sample_rate)
    {
    }

    void instanceConstants(int sample_rate)
    {
        fSampleRate = sample_rate;
        fConst0 = std::min<float>(192000.0f, std::max<float>(1.0f, float(fSampleRate)));
        fConst1 = (24.3473434f / fConst0);
        fConst2 = (6.28318548f / fConst0);
        fConst3 = (121.736717f / fConst0);
    }

    void instanceResetUserInterface()
    {
    }

    void instanceClear()
    {
        for (int l0 = 0; (l0 < 3); l0 = (l0 + 1)) {
            fRec2[l0] = 0.0f;
        }
        for (int l1 = 0; (l1 < 3); l1 = (l1 + 1)) {
            fRec1[l1] = 0.0f;
        }
        for (int l2 = 0; (l2 < 2); l2 = (l2 + 1)) {
            fRec0[l2] = 0.0f;
        }
    }

    void init(int sample_rate)
    {
        classInit(sample_rate);
        instanceInit(sample_rate);
    }
    void instanceInit(int sample_rate)
    {
        instanceConstants(sample_rate);
        instanceResetUserInterface();
        instanceClear();
    }

    faustPhaserStage* clone()
    {
        return new faustPhaserStage();
    }

    int getSampleRate()
    {
        return fSampleRate;
    }

    void compute(int count, FAUSTFLOAT** inputs, FAUSTFLOAT** outputs)
    {
        FAUSTFLOAT* input0 = inputs[0];
        FAUSTFLOAT* input1 = inputs[1];
        FAUSTFLOAT* input2 = inputs[2];
        FAUSTFLOAT* input3 = inputs[3];
        FAUSTFLOAT* output0 = outputs[0];
        for (int i = 0; (i < count); i = (i + 1)) {
            float fTemp0 = float(input0[i]);
            float fTemp1 = float(input1[i]);
            float fTemp2 = float(input2[i]);
            float fTemp3 = float(input3[i]);
            float fTemp4 = (fConst1 * fTemp1);
            float fTemp5 = faustPhaserStage_faustpower2_f((1.0f - fTemp4));
            float fTemp6 = faustPhaserStage_faustpower2_f((fTemp4 + 1.0f));
            float fTemp7 = ((fTemp5 / fTemp6) + 1.0f);
            float fTemp8 = (fTemp0 * fTemp1);
            float fTemp9 = (fRec2[1] * (0.0f - (fTemp7 * std::cos((fConst2 * std::max<float>(0.0f, std::min<float>(3100.0f, ((31.0f * fTemp8) + 1600.0f))))))));
            fRec2[0] = ((fTemp3 + (fRec0[1] * std::min<float>(0.999000013f, (0.00999999978f * fTemp2)))) - (fTemp9 + ((fTemp5 * fRec2[2]) / fTemp6)));
            float fTemp10 = (fConst3 * fTemp1);
            float fTemp11 = faustPhaserStage_faustpower2_f((1.0f - fTemp10));
            float fTemp12 = faustPhaserStage_faustpower2_f((fTemp10 + 1.0f));
            float fTemp13 = ((fTemp11 / fTemp12) + 1.0f);
            float fTemp14 = (fRec1[1] * (0.0f - (fTemp13 * std::cos((fConst2 * std::max<float>(0.0f, std::min<float>(13400.0f, ((155.0f * fTemp8) + 8300.0f))))))));
            fRec1[0] = ((fTemp9 + (0.5f * (fTemp7 * (fRec2[0] + fRec2[2])))) - (fTemp14 + ((fTemp11 * fRec1[2]) / fTemp12)));
            fRec0[0] = (fTemp14 + (0.5f * (fTemp13 * (fRec1[0] + fRec1[2]))));
            output0[i] = FAUSTFLOAT(fRec0[0]);
            fRec2[2] = fRec2[1];
            fRec2[1] = fRec2[0];
            fRec1[2] = fRec1[1];
            fRec1[1] = fRec1[0];
            fRec0[1] = fRec0[0];
        }
    }
};

#endif
