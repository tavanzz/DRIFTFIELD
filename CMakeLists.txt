cmake_minimum_required(VERSION 3.22)

project(Driftfield VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Su macOS: build universale (Apple Silicon + Intel)
if(APPLE)
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "" FORCE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "" FORCE)
endif()

# JUCE: usa una copia locale se passi -DJUCE_DIR=..., altrimenti la scarica da GitHub
if(DEFINED JUCE_DIR)
    add_subdirectory(${JUCE_DIR} JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.4
        GIT_SHALLOW ON)
    FetchContent_MakeAvailable(JUCE)
endif()

juce_add_plugin(Driftfield
    COMPANY_NAME "Driftfield Audio"
    PLUGIN_MANUFACTURER_CODE Dfau
    PLUGIN_CODE Drft
    BUNDLE_ID com.driftfieldaudio.driftfield
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "Driftfield"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    VST3_CATEGORIES Fx Reverb Delay)

target_sources(Driftfield PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_include_directories(Driftfield PRIVATE Source)

target_compile_definitions(Driftfield PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(Driftfield
    PRIVATE
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

# Test offline del DSP (C++ puro, non richiede JUCE)
option(DRIFTFIELD_BUILD_TESTS "Build the offline DSP render test" OFF)
if(DRIFTFIELD_BUILD_TESTS)
    add_executable(dsp_render tests/dsp_render.cpp)
    target_include_directories(dsp_render PRIVATE Source)

    juce_add_console_app(plugin_smoke PRODUCT_NAME "plugin_smoke")
    target_sources(plugin_smoke PRIVATE tests/plugin_smoke.cpp Source/PluginProcessor.cpp Source/PluginEditor.cpp)
    target_include_directories(plugin_smoke PRIVATE Source)
    target_compile_definitions(plugin_smoke PRIVATE
        JucePlugin_Name="Driftfield" JucePlugin_VersionString="0.1.0"
        JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_STANDALONE_APPLICATION=1)
    target_link_libraries(plugin_smoke PRIVATE juce::juce_audio_utils juce::juce_recommended_config_flags)
endif()
