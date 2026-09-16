# Vectorworks Tools

Отдельный хаб надстроек, хелперов и плагинов для **Vectorworks**.
Не часть монорепозитория Volna: сюда кладём только модули Vectorworks и простую страницу каталога.

Репозиторий: [github.com/pridurok-goxa/vectorworks-tools](https://github.com/pridurok-goxa/vectorworks-tools)

## Структура

```text
vectorworks-tools/
  README.md
  modules/vectorworks-launcher-en/   # исходники C++/Win32 launcher
  website/                           # статическая страница каталога
  dist/                              # готовые файлы для скачивания
```

## Каталог на сайте

Самая простая статика: `website/index.html` + `website/styles.css`.
Тяжёлого фреймворка нет.

Открыть локально (из корня проекта, любой свободный порт):

```bash
cd vectorworks-tools
python3 -m http.server 8876
```

Затем в браузере: `http://127.0.0.1:8876/website/`

Скачивание идёт из `dist/` (ссылка `../dist/...` работает и с `file://`, и с сервером из корня проекта).
Не поднимайте сервер из папки `website/` — тогда файл в `dist/` будет недоступен.

Публичный домен и HTTPS не настроены: для этого нужны DNS и явное согласие.

## Как добавить новый модуль на сайт

1. Положите исходники в `modules/<имя-модуля>/`.
2. Положите готовый файл для скачивания в `dist/` (например `dist/MyPlugin.zip`).
3. Скопируйте карточку в `website/index.html` (секция «Модули») и заполните:
   - название;
   - короткое описание;
   - ссылку «Скачать» на файл в `dist/`;
   - ссылку на GitHub (репозиторий, папка модуля или релиз).
4. Если модуль ещё не готов — оставьте карточку в секции «Скоро» без ссылки на скачивание.

## VectorworksLauncherEN

Native Win32 helper: переключает раскладку на English (US) и запускает Vectorworks.
**.NET Runtime не нужен.**

Готовый файл: `dist/VectorworksLauncherEN.exe`.

### Сборка launcher

На Ubuntu / Linux (cross-compile через MinGW):

```bash
sudo aptitude install mingw-w64 g++-mingw-w64-x86-64
cd modules/vectorworks-launcher-en
chmod +x build-mingw.sh
./build-mingw.sh
```

Скрипт кладёт exe в `modules/vectorworks-launcher-en/build/` и копирует его в `dist/`.

Подробности алгоритма и сборки на Windows — в `modules/vectorworks-launcher-en/README.md`.

## GitHub

Публичный репозиторий: [github.com/pridurok-goxa/vectorworks-tools](https://github.com/pridurok-goxa/vectorworks-tools).
`origin` уже настроен. Обычная публикация правок: `git push origin main`.

Публичный домен, GitHub Pages и HTTPS не включены — для этого нужны DNS и явное согласие.
