cmake_minimum_required(VERSION 3.22)
project(SpectralMorpher VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ─── JUCE ──────────────────────────────────────────────────────────────────
# Option A: JUCE as a git submodule at ./JUCE
# Option B: set JUCE_DIR to your system JUCE installation path
#   e.g. cmake -DJUCE_DIR=/path/to/JUCE ..
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/JUCE/CMakeLists.txt")
    add_subdirectory(JUCE)
elseif(DEFINED JUCE_DIR)
    add_subdirectory(${JUCE_DIR} JUCE)
else()
    message(FATAL_ERROR
        "JUCE not found. Either:\n"
        "  1. Run:  git submodule add https://github.com/juce-framework/JUCE.git JUCE\n"
        "  2. Pass: cmake -DJUCE_DIR=/path/to/JUCE ..\n")
endif()

# ─── Plugin target ─────────────────────────────────────────────────────────
juce_add_plugin(SpectralMorpher
    VERSION                     1.0.0
    COMPANY_NAME                "YourStudio"
    PLUGIN_MANUFACTURER_CODE    YSTD
    PLUGIN_CODE                 SPMO
    FORMATS                     VST3 AU Standalone
    PRODUCT_NAME                "Spectral Morpher"
    IS_SYNTH                    FALSE
    NEEDS_MIDI_INPUT            FALSE
    NEEDS_MIDI_OUTPUT           FALSE
    IS_MIDI_EFFECT              FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    VST3_CATEGORIES             "Fx" "Spectral"
    AU_MAIN_TYPE                kAudioUnitType_Effect
    COPY_PLUGIN_AFTER_BUILD     FALSE   # Set TRUE to auto-install after build
)

juce_generate_juce_header(SpectralMorpher)

target_sources(SpectralMorpher PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
    Source/SpectralEngine.cpp
    Source/SpectralEngine.h
)

target_compile_definitions(SpectralMorpher PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
)

target_link_libraries(SpectralMorpher PRIVATE
    juce::juce_audio_basics
    juce::juce_audio_devices
    juce::juce_audio_formats
    juce::juce_audio_plugin_client
    juce::juce_audio_processors
    juce::juce_audio_utils
    juce::juce_core
    juce::juce_data_structures
    juce::juce_dsp
    juce::juce_events
    juce::juce_graphics
    juce::juce_gui_basics
    juce::juce_gui_extra
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)
