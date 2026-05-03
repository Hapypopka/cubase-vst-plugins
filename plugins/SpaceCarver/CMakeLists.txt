juce_add_plugin(SpaceCarver
    COMPANY_NAME "IlmirVST"
    PLUGIN_MANUFACTURER_CODE Ilmr
    PLUGIN_CODE SpCv
    FORMATS VST3
    PRODUCT_NAME "SpaceCarver"
    VST3_CATEGORIES "Fx Dynamics"
    BUNDLE_ID "com.ilmirvst.spacecarver")

juce_generate_juce_header(SpaceCarver)

target_sources(SpaceCarver
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp)

target_compile_definitions(SpaceCarver
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(SpaceCarver
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
