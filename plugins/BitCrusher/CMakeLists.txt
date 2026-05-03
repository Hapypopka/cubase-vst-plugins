juce_add_plugin(BitCrusher
    COMPANY_NAME "IlmirVST"
    PLUGIN_MANUFACTURER_CODE Ilmr
    PLUGIN_CODE Btcr
    FORMATS VST3
    PRODUCT_NAME "Бит-кранчер (BitCrusher)"
    VST3_CATEGORIES "Fx Distortion"
    BUNDLE_ID "com.ilmirvst.bitcrusher")

juce_generate_juce_header(BitCrusher)

target_sources(BitCrusher
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp)

target_compile_definitions(BitCrusher
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(BitCrusher
    PRIVATE
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
