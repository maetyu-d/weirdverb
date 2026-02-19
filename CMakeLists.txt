cmake_minimum_required(VERSION 3.20)
project(weirdVERB LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(verb_dsp src/WeirdConvolutionReverb.cpp)
target_include_directories(verb_dsp PUBLIC include)
target_compile_options(verb_dsp PRIVATE -Wall -Wextra -Wpedantic)

add_executable(verb_suite_demo src/main.cpp)
target_link_libraries(verb_suite_demo PRIVATE verb_dsp)

set(JUCE_DIR "/Users/md/JUCE" CACHE PATH "Path to JUCE root")

if(EXISTS "${JUCE_DIR}/CMakeLists.txt")
    add_subdirectory("${JUCE_DIR}" "JUCE")

    juce_add_plugin(weirdVERB
        VERSION "0.1.0"
        COMPANY_NAME "MD"
        COMPANY_WEBSITE "https://example.com"
        COMPANY_EMAIL "none@example.com"
        BUNDLE_ID "com.md.weirdverb"
        IS_SYNTH FALSE
        NEEDS_MIDI_INPUT FALSE
        NEEDS_MIDI_OUTPUT FALSE
        IS_MIDI_EFFECT FALSE
        EDITOR_WANTS_KEYBOARD_FOCUS FALSE
        COPY_PLUGIN_AFTER_BUILD FALSE
        PLUGIN_MANUFACTURER_CODE MdVS
        PLUGIN_CODE WvRb
        FORMATS AU VST3
        PRODUCT_NAME "weirdVERB")

    juce_generate_juce_header(weirdVERB)

    target_sources(weirdVERB PRIVATE
        src/PluginEditor.cpp
        src/PluginEditor.h
        src/PluginProcessor.cpp
        src/PluginProcessor.h
    )

    target_link_libraries(weirdVERB PRIVATE
        verb_dsp
        juce::juce_audio_utils
        juce::juce_dsp
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

    target_compile_definitions(weirdVERB PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0)
else()
    message(WARNING "JUCE not found at ${JUCE_DIR}. Plugin target skipped; demo target still available.")
endif()
