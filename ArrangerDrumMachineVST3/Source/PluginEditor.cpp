#include "PluginEditor.h"

namespace {
constexpr juce::uint32 bg = 0xff101317;
constexpr juce::uint32 panel = 0xff1a2026;
constexpr juce::uint32 amber = 0xffffb547;
constexpr juce::uint32 green = 0xff45d6a0;
void styleSlider(juce::Slider& s, const juce::String& suffix = {}) {
    s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 64, 18);
    s.setTextValueSuffix(suffix);
    s.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(amber));
    s.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(0xff414a53));
    s.setColour(juce::Slider::thumbColourId, juce::Colour(0xffffd28a));
    s.setColour(juce::Slider::textBoxTextColourId, juce::Colours::white);
    s.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(panel));
    s.setColour(juce::Slider::textBoxOutlineColourId, juce::Colour(0xff343d46));
}
void styleCombo(juce::ComboBox& c) {
    c.setColour(juce::ComboBox::backgroundColourId, juce::Colour(panel));
    c.setColour(juce::ComboBox::textColourId, juce::Colours::white);
    c.setColour(juce::ComboBox::outlineColourId, juce::Colour(0xff46515c));
    c.setColour(juce::ComboBox::arrowColourId, juce::Colour(amber));
}
}

ArrangerDrumMachineAudioProcessorEditor::ArrangerDrumMachineAudioProcessorEditor(ArrangerDrumMachineAudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    setSize(820, 500);
    auto& state = processor.apvts;
    for (int i=0;i<8;++i) styleBox.addItem(ArrangerDrumMachineAudioProcessor::styleNames[(size_t)i], i+1);
    for (int i=0;i<4;++i) variationBox.addItem(ArrangerDrumMachineAudioProcessor::variationNames[(size_t)i], i+1);
    styleCombo(styleBox); styleCombo(variationBox);
    addAndMakeVisible(styleBox); addAndMakeVisible(variationBox);
    styleAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(state,"style",styleBox);
    variationAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(state,"variation",variationBox);

    auto setup = [this](juce::Slider& s, const juce::String& suffix = {}) { styleSlider(s,suffix); addAndMakeVisible(s); };
    setup(bpmSlider," BPM"); setup(kickSlider," %"); setup(snareSlider," %"); setup(hatSlider," %"); setup(percSlider," %"); setup(swingSlider," %"); setup(masterSlider," dB");
    bpmSlider.setRange(40,240,1); bpmSlider.setDoubleClickReturnValue(true,120);
    bpmAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(state,"bpm",bpmSlider);
    kickAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(state,"kickVol",kickSlider);
    snareAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(state,"snareVol",snareSlider);
    hatAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(state,"hatVol",hatSlider);
    percAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(state,"percVol",percSlider);
    swingAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(state,"swing",swingSlider);
    masterAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(state,"volume",masterSlider);

    for (auto* b : {&runButton,&syncButton}) {
        b->setColour(juce::ToggleButton::textColourId,juce::Colours::white);
        b->setColour(juce::ToggleButton::tickColourId,juce::Colour(green));
        b->setColour(juce::ToggleButton::tickDisabledColourId,juce::Colour(0xff59636d));
        addAndMakeVisible(*b);
    }
    runAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(state,"run",runButton);
    syncAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(state,"syncHost",syncButton);
    startTimerHz(24);
}

void ArrangerDrumMachineAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(bg));
    g.setColour(juce::Colour(amber)); g.fillRoundedRectangle(18,16,4,42,2.0f);
    g.setColour(juce::Colours::white); g.setFont(juce::Font(juce::FontOptions(24.0f,juce::Font::bold)));
    g.drawText("ARRANGER DRUM MACHINE",34,14,500,30,juce::Justification::centredLeft);
    g.setColour(juce::Colour(0xff9aa5af)); g.setFont(juce::Font(11.0f));
    g.drawText("SYNTH DRUM ENGINE  •  16-STEP ARRANGER",35,42,500,18,juce::Justification::centredLeft);

    g.setColour(juce::Colour(panel)); g.fillRoundedRectangle(18,78,getWidth()-36,108,10.0f);
    g.setColour(juce::Colour(0xff303942)); g.drawRoundedRectangle(18,78,getWidth()-36,108,10.0f,1.0f);
    g.setColour(juce::Colour(0xffaab4bd)); g.setFont(juce::Font(10.0f,juce::Font::bold));
    g.drawText("ARRANGEMENT",34,86,140,16,juce::Justification::centredLeft);
    g.drawText("16-STEP PATTERN",34,130,150,16,juce::Justification::centredLeft);

    const int x0=34, y0=151, cellW=(getWidth()-68)/16;
    const int active = processor.getDisplayedStep();
    const bool playing = processor.getTransportRunning();
    for (int i=0;i<16;++i) {
        const auto r=juce::Rectangle<float>((float)(x0+i*cellW+2),(float)y0,(float)(cellW-4),22.0f);
        const bool on = playing && i==active;
        g.setColour(on?juce::Colour(green):juce::Colour(0xff303942));
        g.fillRoundedRectangle(r,4.0f);
        g.setColour(on?juce::Colour(0xff101820):juce::Colour(0xffaab4bd));
        g.setFont(juce::Font(10.0f,juce::Font::bold));
        g.drawText(juce::String(i+1),r.toNearestInt(),juce::Justification::centred);
    }

    g.setColour(juce::Colour(panel)); g.fillRoundedRectangle(18,202,getWidth()-36,250,10.0f);
    g.setColour(juce::Colour(0xff303942)); g.drawRoundedRectangle(18,202,getWidth()-36,250,10.0f,1.0f);
    g.setColour(juce::Colour(0xffaab4bd)); g.setFont(juce::Font(10.0f,juce::Font::bold));
    g.drawText("DRUM MIXER",34,211,150,16,juce::Justification::centredLeft);
    g.setColour(juce::Colour(0xff74818c)); g.setFont(juce::Font(10.0f));
    g.drawText("All drum voices are generated inside this plugin — no external drum instrument required.",34,466,getWidth()-68,18,juce::Justification::centredLeft);
}

void ArrangerDrumMachineAudioProcessorEditor::resized()
{
    styleBox.setBounds(180,103,205,30);
    variationBox.setBounds(180,145,205,28);
    runButton.setBounds(420,103,110,28);
    syncButton.setBounds(540,103,140,28);
    bpmSlider.setBounds(690,91,100,88);
    const int top=248, w=94, gap=14, start=34;
    kickSlider.setBounds(start,top,w,155);
    snareSlider.setBounds(start+(w+gap),top,w,155);
    hatSlider.setBounds(start+2*(w+gap),top,w,155);
    percSlider.setBounds(start+3*(w+gap),top,w,155);
    swingSlider.setBounds(start+4*(w+gap),top,w,155);
    masterSlider.setBounds(start+5*(w+gap),top,w,155);
}
void ArrangerDrumMachineAudioProcessorEditor::timerCallback() { repaint(18,78,getWidth()-36,108); }
