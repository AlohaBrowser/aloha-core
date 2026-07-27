## About bromium
bromium это кастомизированный android_webview в котором реализованы различные фичи которых не хватает в default webview 

## Bromium features
- работа с мультимедиа такие как скачивание видео, запись стримов, проигрывание видео в фоне
- Блокировка рекламы на основе Adblock Plus от eyeo
- Реализация приватного режима и минимизация трекинговой информации

## Сборка Bromium
Для сборки bromium используется @aloha/build/build_webview.py с параметром -a arm64
После этого для создания aar вызывается @aloha/build/make_aar.py