# Cubase VST3 Plugins

VST3 плагины автоматически собираемые из C++ кода (JUCE).

## Скачать готовые плагины

Все собранные плагины — на странице [Releases](../../releases).

Каждая сборка содержит .zip архивы для каждого плагина. Распакуй и положи `.vst3` папку в:
- **Windows**: `C:\Program Files\Common Files\VST3\`
- Перезапусти Cubase / FL Studio / любой DAW — плагин появится в списке.

## Как добавить плагин (для Claude)

Структура одного плагина:

```
plugins/
└── MyPlugin/
    ├── CMakeLists.txt        # JUCE plugin config
    └── Source/
        ├── PluginProcessor.h
        ├── PluginProcessor.cpp
        ├── PluginEditor.h
        └── PluginEditor.cpp
```

Минимальный `plugins/<Name>/CMakeLists.txt`:

```cmake
juce_add_plugin(<Name>
    COMPANY_NAME "IlmirVST"
    PLUGIN_MANUFACTURER_CODE Ilmr
    PLUGIN_CODE <4charCode>
    FORMATS VST3
    PRODUCT_NAME "<Name>"
    VST3_CATEGORIES "Fx")

juce_generate_juce_header(<Name>)

target_sources(<Name>
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp)

target_compile_definitions(<Name>
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(<Name>
    PRIVATE
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
```

`PLUGIN_MANUFACTURER_CODE` — ровно 4 буквы, `PLUGIN_CODE` — ровно 4 уникальных символа на каждый плагин.

После добавления нового плагина в `plugins/<Name>/` он автоматически подхватывается корневым `CMakeLists.txt` (`file(GLOB)`).

## Сборка локально (опционально, для отладки)

```bash
git clone --depth 1 --branch 7.0.12 https://github.com/juce-framework/JUCE.git
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel
```

Готовые `.vst3` будут в `build/plugins/<Name>/<Name>_artefacts/Release/VST3/`.

## CI

При каждом push в `main` — GitHub Actions собирает все плагины на Windows-runner и публикует Release с .zip архивами.
