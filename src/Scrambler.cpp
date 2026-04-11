#include "plugin.hpp"
#include <algorithm>
#include <random>

struct Scrambler : Module
{
    enum ParamId
    {
        CLEAN_PARAM,
        SCRAMBLE_PARAM,
        CHUNK_PARAM,
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
        configParam(CLEAN_PARAM, 0.f, 3000.f, 200.f, "Clean samples");
        configParam(SCRAMBLE_PARAM, 0.f, 3000.f, 200.f, "Scramble samples");
        configParam(CHUNK_PARAM, 1.f, 1000.f, 1.f, "Chunk size");
        paramQuantities[CLEAN_PARAM]->snapEnabled = true;
        paramQuantities[SCRAMBLE_PARAM]->snapEnabled = true;
        paramQuantities[CHUNK_PARAM]->snapEnabled = true;
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
            if (collectIndex >= (int)collectBuffer.size())
            {
                int chunkSize = std::max(1, (int)params[CHUNK_PARAM].getValue());
                int bufSize = (int)collectBuffer.size();
                int numChunks = bufSize / chunkSize;
                int remainder = bufSize % chunkSize;

                // Build shuffled index list for chunks
                std::vector<int> chunkIndices(numChunks);
                for (int i = 0; i < numChunks; i++)
                    chunkIndices[i] = i;
                std::shuffle(chunkIndices.begin(), chunkIndices.end(), rng);

                // Reconstruct playback buffer with shuffled chunks
                playbackBuffer.resize(bufSize);
                int writePos = 0;
                for (int i = 0; i < numChunks; i++)
                {
                    int srcStart = chunkIndices[i] * chunkSize;
                    for (int j = 0; j < chunkSize; j++)
                        playbackBuffer[writePos++] = collectBuffer[srcStart + j];
                }
                // Append any remainder samples unshuffled
                for (int i = 0; i < remainder; i++)
                    playbackBuffer[writePos++] = collectBuffer[numChunks * chunkSize + i];

                playbackIndex = 0;
                inPlayback = true;
            }
        }
    }
};

#ifndef METAMODULE
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
#endif

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

        // Chunk size knob
        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(20, 75)), module, Scrambler::CHUNK_PARAM));

        // Audio in/out
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(8.5, 110.5)), module, Scrambler::IN_INPUT));
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(31.5, 110.5)), module, Scrambler::OUT_OUTPUT));
    }
};

Model *modelScrambler = createModel<Scrambler, ScramblerWidget>("Scrambler");