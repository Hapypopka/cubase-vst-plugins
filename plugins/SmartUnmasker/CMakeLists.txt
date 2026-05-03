juce_add_plugin(SmartUnmasker
    COMPANY_NAME "IlmirVST"
    PLUGIN_MANUFACTURER_CODE Ilmr
    PLUGIN_CODE Smun
    FORMATS VST3
    PRODUCT_NAME "Анмаскер (SmartUnmasker)"
    VST3_CATEGORIES "Fx Dynamics"
    BUNDLE_ID "com.ilmirvst.smartunmasker")

juce_generate_juce_header(SmartUnmasker)

target_sources(SmartUnmasker
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/DSP/FFTProcessor.cpp
        Source/DSP/SpectralMask.cpp
        Source/DSP/OverlapAdd.cpp)

target_include_directories(SmartUnmasker
    PRIVATE
        Source)

target_compile_definitions(SmartUnmasker
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(SmartUnmasker
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
