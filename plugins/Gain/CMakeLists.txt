juce_add_plugin(Gain
    COMPANY_NAME "IlmirVST"
    PLUGIN_MANUFACTURER_CODE Ilmr
    PLUGIN_CODE Gain
    FORMATS VST3
    PRODUCT_NAME "Gain"
    VST3_CATEGORIES "Fx Tools"
    BUNDLE_ID "com.ilmirvst.gain")

juce_generate_juce_header(Gain)

target_sources(Gain
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp)

target_compile_definitions(Gain
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(Gain
    PRIVATE
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
