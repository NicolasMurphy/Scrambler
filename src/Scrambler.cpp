#include "plugin.hpp"
#include <algorithm>
#include <random>

struct Scrambler : Module
{
    enum ParamId
    {
        CLEAN_PARAM,
        SCRAMBLE_PARAM,
        PARAMS_LEN
    };
    enum InputId
    {
        IN_INPUT,
        INPUTS_LEN
    };
    enum OutputId
    {
        OUT_OUTPUT,
        OUTPUTS_LEN
    };

    std::vector<float> buffer;
    int position = 0;
    bool inClean = true;
    int bufferIndex = 0;
    std::mt19937 rng{std::random_device{}()};

    Scrambler()
    {
        config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN);
        configParam(CLEAN_PARAM, 0.f, 10000.f, 3000.f, "Clean samples");
        configParam(SCRAMBLE_PARAM, 0.f, 10000.f, 200.f, "Scramble samples");
        configInput(IN_INPUT, "Audio");
        configOutput(OUT_OUTPUT, "Audio");
    }

    void process(const ProcessArgs &args) override
    {
        int cleanLen = (int)params[CLEAN_PARAM].getValue();
        int scrambleLen = (int)params[SCRAMBLE_PARAM].getValue();
        float in = inputs[IN_INPUT].getVoltage();

        if (inClean)
        {
            outputs[OUT_OUTPUT].setVoltage(in);
            position++;
            if (position >= cleanLen)
            {
                position = 0;
                inClean = false;
                buffer.resize(scrambleLen);
                bufferIndex = 0;
            }
        }
        else
        {
            buffer[bufferIndex++] = in;
            if (bufferIndex >= scrambleLen)
            {
                std::shuffle(buffer.begin(), buffer.end(), rng);
                for (float s : buffer)
                {
                    outputs[OUT_OUTPUT].setVoltage(s);
                }
                inClean = true;
                position = 0;
            }
            else
            {
                outputs[OUT_OUTPUT].setVoltage(0.f);
            }
        }
    }
};

struct ScramblerWidget : ModuleWidget
{
    ScramblerWidget(Scrambler *module)
    {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/Scrambler.svg")));

        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(10, 20)), module, Scrambler::CLEAN_PARAM));
        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(10, 40)), module, Scrambler::SCRAMBLE_PARAM));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(10, 60)), module, Scrambler::IN_INPUT));
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(10, 80)), module, Scrambler::OUT_OUTPUT));
    }
};

Model *modelScrambler = createModel<Scrambler, ScramblerWidget>("Scrambler");