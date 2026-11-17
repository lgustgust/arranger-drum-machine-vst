#include "PluginProcessor.h"
#include "PluginEditor.h"

const std::array<juce::String, 8> ArrangerDrumMachineAudioProcessor::styleNames = {
    "Pop Rock", "Disco 80s", "Bossa Nova", "Hip Hop", "Reggae", "Ballad", "Funk", "Samba"
};
const std::array<juce::String, 4> ArrangerDrumMachineAudioProcessor::variationNames = {
    "Main A", "Main B", "Fill In 1", "Fill In 2"
};

// Exact 16-step bit masks recovered from Arranger_Drum_Machine_v7.fmx.
// Order in each row: Kick, Snare, Closed Hat, Percussion. Bit 15 is step 1.
uint32_t ArrangerDrumMachineAudioProcessor::patterns[8][4][4] = {
    {{33410,2056,43690,2},{35474,2072,65535,64},{34952,2655,43690,1024},{32896,4095,21845,32768}},
    {{34952,2056,43690,8738},{34968,2120,65535,8738},{34952,3391,43690,20480},{43690,21845,34952,255}},
    {{33410,0,43690,37449},{33666,514,65535,37961},{34952,62,43690,18724},{33924,1455,21845,43690}},
    {{33154,2056,43690,32},{34194,2072,61166,1028},{32896,2175,43690,512},{39321,3855,21845,51}},
    {{128,128,43690,8738},{640,132,65535,17476},{136,127,43690,34952},{2056,22015,43690,9252}},
    {{32800,2056,43690,4},{33316,2072,65535,256},{34952,639,43690,2048},{32768,255,21845,16384}},
    {{33345,2088,65535,8224},{34385,2346,65535,9282},{34952,7163,43690,16448},{37265,28399,65535,34952}},
    {{33410,10570,65535,17044},{34695,28086,65535,38069},{34952,16255,43690,43690},{43690,65535,21845,52428}}
};

juce::AudioProcessorValueTreeState::ParameterLayout ArrangerDrumMachineAudioProcessor::createParameterLayout()
{
    using P = juce::AudioParameterFloat;
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;
    p.push_back(std::make_unique<juce::AudioParameterInt>("bpm", "BPM", 40, 240, 120));
    p.push_back(std::make_unique<juce::AudioParameterChoice>("style", "Style", juce::StringArray{"Pop Rock","Disco 80s","Bossa Nova","Hip Hop","Reggae","Ballad","Funk","Samba"}, 0));
    p.push_back(std::make_unique<juce::AudioParameterChoice>("variation", "Variation", juce::StringArray{"Main A","Main B","Fill In 1","Fill In 2"}, 0));
    p.push_back(std::make_unique<juce::AudioParameterBool>("syncHost", "Sync to Host", true));
    p.push_back(std::make_unique<juce::AudioParameterBool>("run", "Run", true));
    p.push_back(std::make_unique<P>("kickVol", "Kick", juce::NormalisableRange<float>(0.0f,100.0f,0.1f),85.0f));
    p.push_back(std::make_unique<P>("snareVol", "Snare", juce::NormalisableRange<float>(0.0f,100.0f,0.1f),80.0f));
    p.push_back(std::make_unique<P>("hatVol", "Hi-Hat", juce::NormalisableRange<float>(0.0f,100.0f,0.1f),75.0f));
    p.push_back(std::make_unique<P>("percVol", "Percussion", juce::NormalisableRange<float>(0.0f,100.0f,0.1f),70.0f));
    p.push_back(std::make_unique<P>("swing", "Swing", juce::NormalisableRange<float>(0.0f,100.0f,0.1f),0.0f));
    p.push_back(std::make_unique<P>("volume", "Master", juce::NormalisableRange<float>(-60.0f,12.0f,0.1f),-3.0f));
    return {p.begin(), p.end()};
}

ArrangerDrumMachineAudioProcessor::ArrangerDrumMachineAudioProcessor()
    : AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "PARAMETERS", createParameterLayout()) {}

void ArrangerDrumMachineAudioProcessor::prepareToPlay(double sr, int)
{
    currentSampleRate = sr > 0.0 ? sr : 44100.0;
    freePhase = 0.0;
    lastAbsoluteStep = std::numeric_limits<int64_t>::min();
    for (auto& v : voices) v.active = false;
}

bool ArrangerDrumMachineAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::mono()
        || layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void ArrangerDrumMachineAudioProcessor::trigger(int type, float velocity, int style)
{
    auto* chosen = &voices[0];
    for (auto& v : voices) { if (!v.active) { chosen = &v; break; } if (v.age > chosen->age) chosen = &v; }
    *chosen = DrumVoice{};
    chosen->active = true;
    chosen->type = type;
    chosen->style = style;
    chosen->velocity = juce::jlimit(0.0f, 1.5f, velocity);
    chosen->noiseState = static_cast<uint32_t>(random.nextInt()) | 1u;
    if (type == 0) chosen->duration = 0.42;
    else if (type == 1) chosen->duration = 0.24;
    else if (type == 2) chosen->duration = 0.11;
    else chosen->duration = (style == 2 ? 0.16 : 0.22);
}

float ArrangerDrumMachineAudioProcessor::renderVoice(DrumVoice& v)
{
    if (!v.active) return 0.0f;
    const double t = v.age;
    const double sr = currentSampleRate;
    auto noise = [&]() -> float {
        v.noiseState ^= v.noiseState << 13; v.noiseState ^= v.noiseState >> 17; v.noiseState ^= v.noiseState << 5;
        return (static_cast<float>(v.noiseState & 0xffffu) / 32767.5f) - 1.0f;
    };
    float out = 0.0f;
    if (v.type == 0) { // synthesized kick: fast pitch drop + damped sine + short click
        const double f = 46.0 + 112.0 * std::exp(-t * 34.0);
        v.phase += juce::MathConstants<double>::twoPi * f / sr;
        const float env = static_cast<float>(std::exp(-t * 10.5));
        const float click = (t < 0.004 ? noise() * static_cast<float>(1.0 - t / 0.004) * 0.12f : 0.0f);
        out = static_cast<float>(std::sin(v.phase)) * env * 0.95f + click;
    } else if (v.type == 1) { // snare: filtered-ish noise + tuned body
        v.phase += juce::MathConstants<double>::twoPi * 185.0 / sr;
        const float envN = static_cast<float>(std::exp(-t * 20.0));
        const float envB = static_cast<float>(std::exp(-t * 26.0));
        const float n = noise();
        const float hp = n - v.previousNoise * 0.72f; v.previousNoise = n;
        out = hp * envN * 0.58f + static_cast<float>(std::sin(v.phase)) * envB * 0.28f;
    } else if (v.type == 2) { // closed hi-hat: high-passed noise
        const float n = noise();
        const float hp = n - v.previousNoise * 0.92f; v.previousNoise = n;
        out = hp * static_cast<float>(std::exp(-t * 42.0)) * 0.48f;
    } else { // style-aware synthesized hand percussion
        if (v.style == 2) { // Bossa Nova: short sidestick / rim-click character
            v.phase += juce::MathConstants<double>::twoPi * 230.0 / sr;
            const float env = static_cast<float>(std::exp(-t * 34.0));
            out = (noise() * 0.48f + static_cast<float>(std::sin(v.phase)) * 0.16f) * env;
        } else if (v.style == 7) { // Samba: bright claves / agogo-like transient
            v.phase += juce::MathConstants<double>::twoPi * 1780.0 / sr;
            const float env = static_cast<float>(std::exp(-t * 22.0));
            out = (static_cast<float>(std::sin(v.phase)) * 0.48f
                 + static_cast<float>(std::sin(v.phase * 1.31)) * 0.30f
                 + noise() * 0.08f) * env;
        } else { // other styles: compact cowbell / tom-like voice
            v.phase += juce::MathConstants<double>::twoPi * 410.0 / sr;
            const float env = static_cast<float>(std::exp(-t * 15.0));
            out = (static_cast<float>(std::sin(v.phase)) * 0.52f
                 + static_cast<float>(std::sin(v.phase * (565.0 / 410.0))) * 0.32f
                 + noise() * 0.12f) * env;
        }
    }
    v.age += 1.0 / sr;
    if (v.age >= v.duration) v.active = false;
    return out * v.velocity;
}

void ArrangerDrumMachineAudioProcessor::triggerStep(int step, int style, int variation,
    float kick, float snare, float hat, float perc)
{
    const uint32_t bit = 1u << (15 - (step & 15));
    const auto& mask = patterns[juce::jlimit(0,7,style)][juce::jlimit(0,3,variation)];
    if ((mask[0] & bit) && kick > 0.0001f) trigger(0, kick * 1.05f, style);
    if ((mask[1] & bit) && snare > 0.0001f) trigger(1, snare, style);
    if ((mask[2] & bit) && hat > 0.0001f) trigger(2, hat * 0.9f, style);
    if ((mask[3] & bit) && perc > 0.0001f) trigger(3, perc, style);
}

void ArrangerDrumMachineAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    midi.clear();
    const int n = buffer.getNumSamples();
    buffer.clear();
    const int style = static_cast<int>(apvts.getRawParameterValue("style")->load());
    const int variation = static_cast<int>(apvts.getRawParameterValue("variation")->load());
    const bool run = apvts.getRawParameterValue("run")->load() > 0.5f;
    const bool sync = apvts.getRawParameterValue("syncHost")->load() > 0.5f;
    const double manualBpm = apvts.getRawParameterValue("bpm")->load();
    const float kick = apvts.getRawParameterValue("kickVol")->load() * 0.01f;
    const float snare = apvts.getRawParameterValue("snareVol")->load() * 0.01f;
    const float hat = apvts.getRawParameterValue("hatVol")->load() * 0.01f;
    const float perc = apvts.getRawParameterValue("percVol")->load() * 0.01f;
    const double swing = apvts.getRawParameterValue("swing")->load() * 0.005;
    const float master = juce::Decibels::decibelsToGain(apvts.getRawParameterValue("volume")->load());

    double bpm = manualBpm;
    double ppqStart = 0.0;
    bool hostPlaying = false;
    if (sync) {
        if (auto* ph = getPlayHead()) {
            if (auto pos = ph->getPosition()) {
                bpm = pos->getBpm().orFallback(manualBpm);
                ppqStart = pos->getPpqPosition().orFallback(0.0);
                hostPlaying = pos->getIsPlaying();
            }
        }
    }
    bpm = juce::jlimit(40.0, 240.0, bpm);
    const bool active = run && (!sync || hostPlaying);
    transportRunning.store(active);
    const double inc = bpm / (60.0 * currentSampleRate) * 4.0;
    int lastStepForUi = displayedStep.load();
    for (int i = 0; i < n; ++i) {
        if (active) {
            double phase;
            if (sync) phase = (ppqStart + (static_cast<double>(i) * bpm / (60.0 * currentSampleRate))) * 4.0;
            else { freePhase += inc; phase = freePhase; }
            const int64_t absStep = static_cast<int64_t>(std::floor(phase));
            const int step = static_cast<int>((absStep % 16 + 16) % 16);
            const double threshold = static_cast<double>(absStep) + ((step & 1) ? swing : 0.0);
            if (phase >= threshold && absStep != lastAbsoluteStep) {
                triggerStep(step, style, variation, kick, snare, hat, perc);
                lastAbsoluteStep = absStep;
                lastStepForUi = step;
            }
        }
        float sample = 0.0f;
        for (auto& v : voices) sample += renderVoice(v);
        sample *= master * 0.72f;
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch) buffer.setSample(ch, i, sample);
    }
    displayedStep.store(lastStepForUi);
}

void ArrangerDrumMachineAudioProcessor::getStateInformation(juce::MemoryBlock& dest)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, dest);
}
void ArrangerDrumMachineAudioProcessor::setStateInformation(const void* data, int size)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, size));
    if (xml && xml->hasTagName(apvts.state.getType())) apvts.replaceState(juce::ValueTree::fromXml(*xml));
}
juce::AudioProcessorEditor* ArrangerDrumMachineAudioProcessor::createEditor() { return new ArrangerDrumMachineAudioProcessorEditor(*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new ArrangerDrumMachineAudioProcessor(); }
