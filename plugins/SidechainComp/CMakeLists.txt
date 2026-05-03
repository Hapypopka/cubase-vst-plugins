juce_add_plugin(SidechainComp
    COMPANY_NAME "IlmirVST"
    PLUGIN_MANUFACTURER_CODE Ilmr
    PLUGIN_CODE Sdcp
    FORMATS VST3
    PRODUCT_NAME "Сайдчейн-компрессор (SidechainComp)"
    VST3_CATEGORIES "Fx Dynamics"
    BUNDLE_ID "com.ilmirvst.sidechaincomp")

juce_generate_juce_header(SidechainComp)

target_sources(SidechainComp
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp)

target_compile_definitions(SidechainComp
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(SidechainComp
    PRIVATE
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
