#pragma once
#include <JuceHeader.h>
#include <array>
#include <atomic>

class ArrangerDrumMachineAudioProcessor : public juce::AudioProcessor
{
public:
    ArrangerDrumMachineAudioProcessor();
    ~ArrangerDrumMachineAudioProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "Arranger Drum Machine"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 2.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;

    juce::AudioProcessorValueTreeState apvts;
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    int getDisplayedStep() const { return displayedStep.load(); }
    bool getTransportRunning() const { return transportRunning.load(); }
    static const std::array<juce::String, 8> styleNames;
    static const std::array<juce::String, 4> variationNames;

private:
    struct DrumVoice
    {
        bool active = false;
        int type = 0; // 0 kick, 1 snare, 2 hat, 3 percussion
        int style = 0;
        double phase = 0.0;
        double age = 0.0;
        double duration = 1.0;
        float amplitude = 1.0f;
        float velocity = 1.0f;
        float previousNoise = 0.0f;
        uint32_t noiseState = 0x1234567u;
    };
    void trigger(int type, float velocity, int style);
    float renderVoice(DrumVoice& v);
    void triggerStep(int step, int style, int variation,
                     float kick, float snare, float hat, float perc);
    static uint32_t patterns[8][4][4];

    double currentSampleRate = 44100.0;
    double freePhase = 0.0;
    int64_t lastAbsoluteStep = std::numeric_limits<int64_t>::min();
    std::array<DrumVoice, 32> voices{};
    std::atomic<int> displayedStep{0};
    std::atomic<bool> transportRunning{false};
    juce::Random random;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ArrangerDrumMachineAudioProcessor)
};
