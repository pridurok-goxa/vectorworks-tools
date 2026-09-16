# Vectorworks Launcher EN (native Win32)

Небольшой Windows-helper **без .NET Runtime**: перед запуском Vectorworks переключает системную раскладку на **English (US)** (KLID `00000409`) и запускает `Vectorworks20xx.exe`.

Итоговый файл: **`VectorworksLauncherEN.exe`**. Готовая копия для каталога лежит в [`../../dist/`](../../dist/).

Каталог модулей: [vectorworks-tools](https://github.com/pridurok-goxa/vectorworks-tools).

## Для пользователя (Windows)

1. Скопируйте `VectorworksLauncherEN.exe` в папку установки Vectorworks (рядом с `Vectorworks20xx.exe`) **или** в любое удобное место.
2. Запустите helper двойным щелчком.
3. Helper:
   - попытается включить английскую раскладку;
   - найдёт Vectorworks в своей папке, затем в `Program Files` / `Program Files (x86)`;
   - если не найдёт — предложит выбрать `.exe` вручную;
   - запустит Vectorworks с рабочей папкой каталога установки.
4. Если английская раскладка недоступна в Windows — helper покажет предупреждение и не продолжит.
5. Если переключить раскладку не удалось — появится диалог **Продолжить / Отмена**.

### Повторная раскладка после старта Vectorworks

После появления главного окна Vectorworks helper **ещё раз** отправляет окну запрос смены раскладки (`WM_INPUTLANGCHANGEREQUEST`). Это **запрос Windows**, а не принудительная смена раскладки в чужом процессе: ОС может проигнорировать его, если политика ввода или фокус окна не позволяют. Цель — компенсировать per-app раскладку, которую Vectorworks иногда восстанавливает при получении фокуса.

### Требования

- Windows 10/11 (x64).
- В системе должна быть установлена раскладка **English (United States)**.
- **Не требуется** .NET Desktop Runtime.

## Для разработчика

### Структура

```text
modules/vectorworks-launcher-en/
  assets/Vectorworks.ico
  resources/VectorworksLauncherEN.rc
  src/
    main.cpp
    keyboard_layout.cpp
    vectorworks_discovery.cpp
    user_prompts.cpp
  build-mingw.sh
  README.md
```

### Сборка на Ubuntu (cross-compile)

```bash
sudo aptitude install mingw-w64 g++-mingw-w64-x86-64
cd modules/vectorworks-launcher-en
chmod +x build-mingw.sh
./build-mingw.sh
```

Артефакт: `build/VectorworksLauncherEN.exe`. Скрипт также копирует его в `dist/` в корне хаба.

Флаги: `-mwindows` (без консоли), `-municode`, `-static-libgcc -static-libstdc++` для меньшей зависимости от DLL MinGW на целевой машине.

### Сборка на Windows (опционально)

Можно собрать тем же исходником через MSYS2/MinGW-w64 или Visual Studio с аналогичными WinAPI-зависимостями (`user32`, `comdlg32`, `shell32`).

### Алгоритм поиска Vectorworks

1. Папка helper (`GetModuleFileNameW`), маска `Vectorworks*.exe`.
2. `Program Files` / `Program Files (x86)`: каталоги `Vectorworks *`, `Vectorworks * EN`, `VW*`.
3. Приоритет: `Vectorworks20??.exe` с большим годом в имени, затем `LastWriteTime`.
4. Чёрный список: Install Manager, Uninstall, error handler, package manager, proxy/helpers и прочие служебные `.exe` (см. `vectorworks_discovery.cpp`).

### Переключение раскладки

Слои (до запуска и повторно в окно Vectorworks):

- `LoadKeyboardLayoutW("00000409")`, `ActivateKeyboardLayout`
- `AttachThreadInput` + активация на потоке целевого окна
- `PostMessageW(WM_INPUTLANGCHANGEREQUEST)`
- fallback hotkeys: Alt+Shift, Ctrl+Shift, Win+Space через `SendInput`

### Иконка и trademark

Иконка `assets/Vectorworks.ico` перенесена из прежнего helper в Volna (источник: Wikimedia Commons). Логотип Vectorworks может быть товарным знаком правообладателя; для внутреннего helper у установленного ПО это допустимо, для публичной публикации — проверьте права отдельно.
