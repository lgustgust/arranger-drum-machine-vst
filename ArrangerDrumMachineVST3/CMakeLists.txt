cmake_minimum_required(VERSION 3.22)
project(ArrangerDrumMachine VERSION 1.0.0)

include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.6
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(ArrangerDrumMachine
    COMPANY_NAME "Arranger Audio"
    BUNDLE_ID "com.arrangeraudio.arrangerdrummachine"
    PLUGIN_MANUFACTURER_CODE ArAu
    PLUGIN_CODE ArD1
    FORMATS VST3 Standalone
    PRODUCT_NAME "Arranger Drum Machine"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
)

# Generate JuceHeader.h required by the source files.
juce_generate_juce_header(ArrangerDrumMachine)

target_sources(ArrangerDrumMachine PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_compile_features(ArrangerDrumMachine PRIVATE cxx_std_17)
target_compile_definitions(ArrangerDrumMachine PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)
target_link_libraries(ArrangerDrumMachine PRIVATE
    juce::juce_audio_utils
    juce::juce_audio_processors
    juce::juce_dsp
    PUBLIC juce::juce_recommended_config_flags
           juce::juce_recommended_lto_flags
           juce::juce_recommended_warning_flags)
