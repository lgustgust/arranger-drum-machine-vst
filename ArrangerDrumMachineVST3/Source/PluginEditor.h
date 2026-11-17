#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class ArrangerDrumMachineAudioProcessorEditor : public juce::AudioProcessorEditor,
                                                private juce::Timer
{
public:
    explicit ArrangerDrumMachineAudioProcessorEditor(ArrangerDrumMachineAudioProcessor&);
    ~ArrangerDrumMachineAudioProcessorEditor() override = default;
    void paint(juce::Graphics&) override;
    void resized() override;
private:
    void timerCallback() override;
    void styleComboChanged();
    ArrangerDrumMachineAudioProcessor& processor;
    juce::ComboBox styleBox, variationBox;
    juce::Slider bpmSlider, kickSlider, snareSlider, hatSlider, percSlider, swingSlider, masterSlider;
    juce::ToggleButton runButton { "RUN" }, syncButton { "SYNC HOST" };
    juce::Label bpmLabel, styleLabel, variationLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> bpmAttach, kickAttach, snareAttach, hatAttach, percAttach, swingAttach, masterAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> styleAttach, variationAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> runAttach, syncAttach;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ArrangerDrumMachineAudioProcessorEditor)
};
