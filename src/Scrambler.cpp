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
        CVINC_INPUT,
        CVINS_INPUT,
        INPUTS_LEN
    };
    enum OutputId
    {
        OUT_OUTPUT,
        OUTPUTS_LEN
    };

    std::vector<float> collectBuffer;
    std::vector<float> playbackBuffer;
    int position = 0;
    bool inClean = true;
    int collectIndex = 0;
    int playbackIndex = 0;
    bool inPlayback = false;
    std::mt19937 rng{std::random_device{}()};

    Scrambler()
    {
        config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN);
        configParam(CLEAN_PARAM, 0.f, 10000.f, 3000.f, "Clean samples");
        configParam(SCRAMBLE_PARAM, 0.f, 10000.f, 200.f, "Scramble samples");
        configInput(IN_INPUT, "Audio");
        configOutput(OUT_OUTPUT, "Audio");
        configInput(CVINC_INPUT, "Clean CV");
        configInput(CVINS_INPUT, "Scramble CV");
    }

    void process(const ProcessArgs &args) override
    {
        float cleanVal = params[CLEAN_PARAM].getValue();
        if (inputs[CVINC_INPUT].isConnected())
            cleanVal += inputs[CVINC_INPUT].getVoltage() * 1000.f;
        int cleanLen = std::max(1, (int)cleanVal);

        float scrambleVal = params[SCRAMBLE_PARAM].getValue();
        if (inputs[CVINS_INPUT].isConnected())
            scrambleVal += inputs[CVINS_INPUT].getVoltage() * 1000.f;
        int scrambleLen = std::max(1, (int)scrambleVal);
        float in = inputs[IN_INPUT].getVoltage();

        if (inPlayback)
        {
            outputs[OUT_OUTPUT].setVoltage(playbackBuffer[playbackIndex++]);
            if (playbackIndex >= (int)playbackBuffer.size())
            {
                inPlayback = false;
                inClean = true;
                position = 0;
            }
            return;
        }

        if (inClean)
        {
            outputs[OUT_OUTPUT].setVoltage(in);
            position++;
            if (position >= cleanLen)
            {
                position = 0;
                inClean = false;
                collectBuffer.resize(scrambleLen);
                collectIndex = 0;
            }
        }
        else
        {
            collectBuffer[collectIndex++] = in;
            outputs[OUT_OUTPUT].setVoltage(0.f);
            if (collectIndex >= scrambleLen)
            {
                playbackBuffer = collectBuffer;
                std::shuffle(playbackBuffer.begin(), playbackBuffer.end(), rng);
                playbackIndex = 0;
                inPlayback = true;
            }
        }
    }
};

static void addLabel(rack::widget::Widget *parent, const char *text, float x, float y, float width)
{
    rack::ui::Label *label = new rack::ui::Label;
    label->box.pos = rack::math::Vec(x, y);
    label->box.size = rack::math::Vec(width, 20);
    label->text = text;
    label->color = nvgRGB(255, 255, 255);
    label->fontSize = 11;
    parent->addChild(label);
}

struct ScramblerWidget : ModuleWidget
{
    ScramblerWidget(Scrambler *module)
    {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/Scrambler.svg")));

        // Knobs
        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(10, 30.5)), module, Scrambler::CLEAN_PARAM));
        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(30, 30.5)), module, Scrambler::SCRAMBLE_PARAM));

        // CV inputs
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(9.9, 48.5)), module, Scrambler::CVINC_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(30.4, 48.5)), module, Scrambler::CVINS_INPUT));

        // Audio in/out
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(8.5, 110.5)), module, Scrambler::IN_INPUT));
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(31.5, 110.5)), module, Scrambler::OUT_OUTPUT));
    }
};

Model *modelScrambler = createModel<Scrambler, ScramblerWidget>("Scrambler");