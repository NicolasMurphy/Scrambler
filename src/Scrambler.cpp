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
        CVINCH_INPUT,
        INPUTS_LEN
    };
    enum OutputId
    {
        OUT_OUTPUT,
        OUTPUTS_LEN
    };

    std::vector<float> collectBuffer;
    std::vector<float> playbackBuffer;
    std::vector<int> chunkIndices;
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
        configParam(CHUNK_PARAM, 1.f, 200.f, 1.f, "Chunk size");
        paramQuantities[CLEAN_PARAM]->snapEnabled = true;
        paramQuantities[SCRAMBLE_PARAM]->snapEnabled = true;
        paramQuantities[CHUNK_PARAM]->snapEnabled = true;
        configInput(IN_INPUT, "Audio");
        configOutput(OUT_OUTPUT, "Audio");
        configInput(CVINC_INPUT, "Clean CV");
        configInput(CVINS_INPUT, "Scramble CV");
        configInput(CVINCH_INPUT, "Chunk CV");
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
                float chunkVal = params[CHUNK_PARAM].getValue();
                if (inputs[CVINCH_INPUT].isConnected())
                    chunkVal += inputs[CVINCH_INPUT].getVoltage() * 1000.f;
                int chunkSize = std::max(1, (int)chunkVal);
                int bufSize = (int)collectBuffer.size();
                int numChunks = bufSize / chunkSize;
                int remainder = bufSize % chunkSize;

                // Build shuffled index list for chunks
                chunkIndices.resize(numChunks);
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


struct ScramblerWidget : ModuleWidget
{
    ScramblerWidget(Scrambler *module)
    {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/Scrambler.svg")));

        // Group 1: Clean
        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(20, 22.5)), module, Scrambler::CLEAN_PARAM));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(20, 37)), module, Scrambler::CVINC_INPUT));

        // Group 2: Scramble
        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(20, 53.5)), module, Scrambler::SCRAMBLE_PARAM));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(20, 67.7)), module, Scrambler::CVINS_INPUT));

        // Group 3: Chunk
        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(20, 84)), module, Scrambler::CHUNK_PARAM));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(20, 98.5)), module, Scrambler::CVINCH_INPUT));

        // Audio in/out
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(8.5, 110.5)), module, Scrambler::IN_INPUT));
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(31.5, 110.5)), module, Scrambler::OUT_OUTPUT));
    }
};

Model *modelScrambler = createModel<Scrambler, ScramblerWidget>("Scrambler");