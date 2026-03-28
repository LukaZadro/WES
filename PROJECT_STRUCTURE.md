# WES Project Structure & Submodule Guide

## 📋 Overview

This is an **ESP32 GUI Application** using **SquareLine Studio** for UI design, **LVGL 8.3** for graphics, and **ESP-IDF 5.5.3** for embedded firmware.

### Key Dependencies (Git Submodules)

| Component | Location | Version | Purpose |
|-----------|----------|---------|---------|
| **LVGL** | `components/lvgl/` | v8.3.4 | Lightweight embedded graphics library |
| **LVGL ESP32 Drivers** | `components/lvgl_esp32_drivers/` | develop/lvgl_7.11.0_idf_5.0 | Display (ILI9341) & Touch (XPT2046) controller drivers |

**Verify submodules are initialized:**
```powershell
cd C:\wes\WES
git submodule status
```

Expected output:
```
 2c0162b457e32da50268127575c0c2b95ab29bc1 components/lvgl (v8.3.4)
 335d444e450c6669a9f8f91c350def5dfaf53788 components/lvgl_esp32_drivers
```

---

## 📂 Directory Hierarchy

```
C:\wes\WES/
├── CMakeLists.txt                           # ESP-IDF root build config
├── sdkconfig                                # Device-specific config (ILI9341, XPT2046, GPIO pins)
├── sdkconfig.defaults                       # Default config template
├── README.md                                # User-facing quick start guide
├── PROJECT_STRUCTURE.md                     # This file
│
├── main/
│   ├── app_main.c                           # ESP-IDF entry point
│   ├── gui.c                                # GUI task init + LVGL event loop
│   ├── gui.h
│   └── CMakeLists.txt
│
├── components/
│   ├── lvgl/                                # ⭐ SUBMODULE (auto-synced)
│   │   └── (LVGL core graphics library)
│   │
│   ├── lvgl_esp32_drivers/                  # ⭐ SUBMODULE (auto-synced)
│   │   ├── lvgl_tft/ili9341.c               # Display driver (320x240 RGB565)
│   │   ├── lvgl_touch/xpt2046.c             # Touch driver
│   │   └── lvgl_helpers.h                   # DMA & buffer config
│   │
│   ├── ui_app/                              # ⭐ UI BUILD ORCHESTRATOR
│   │   ├── CMakeLists.txt                   # Smart folder selector + GLOB
│   │   ├── ui_app.c                         # UI init wrapper
│   │   ├── ui_app.h
│   │   │
│   │   └── squareline/                      # ⭐ MAIN EXPORT FOLDER (preferred)
│   │       ├── CMakeLists.txt               # Auto-discovers all .c files
│   │       ├── ui.c                         # SquareLine-generated UI init
│   │       ├── ui.h
│   │       ├── ui_events.c                  # ⭐ EVENT HANDLERS (student code here)
│   │       ├── ui_events.h
│   │       ├── ui_helpers.c
│   │       ├── ui_helpers.h
│   │       │
│   │       ├── screens/
│   │       │   ├── ui_Home_Scr.c            # Auto-generated screen 1
│   │       │   ├── ui_Color_Scr.c           # Auto-generated screen 2
│   │       │   ├── ui_Settings_Scr.c        # Auto-generated screen 3
│   │       │   └── ... (new exports appear here)
│   │       │
│   │       ├── images/
│   │       │   ├── ui_img_home_icon_png.c   # Auto-generated image
│   │       │   ├── ui_img_colorwheel_icon_png.c
│   │       │   └── ... (new exports appear here)
│   │       │
│   │       ├── components/
│   │       │   └── ui_comp_hook.c           # Auto-generated component
│   │       │
│   │       ├── fonts/                       # Font files (if any)
│   │       ├── SQUARELINE.md                # SquareLine project exported info
│   │       └── project/                     # ⭐ LEGACY EXPORT FOLDER (fallback only)
│   │           ├── ui.sll                   # SquareLine project file
│   │           ├── ui.spj                   # SquareLine project settings
│   │           └── CMakeLists.txt           # Fallback build (auto-discovered)
│   │
│   └── wifi_station/                        # WiFi utility component
│       └── (WiFi setup for OTA updates, etc.)
│
├── patches/
│   └── lvgl_esp32_drivers_8-3.patch         # LVGL 8.3 API compatibility patch
│       (Applied on first build for driver compatibility)
│
├── scripts/
│   ├── apply_patch.ps1                      # PowerShell patch application
│   └── apply_patch.sh                       # Bash patch application
│
└── doc/
    └── BL_Dev_Kit/                          # Hardware documentation


```

---

## 🔄 Build System Architecture

### CMake Build Flow

```
idf.py build
  └─> CMakeLists.txt (root)
      └─> components/ui_app/CMakeLists.txt
          ├─> Detects if ui.c exists in squareline/ (preferred)
          │   └─> components/ui_app/squareline/CMakeLists.txt
          │       ├─> Glob all .c files recursively
          │       └─> Compile + link as "ui" library
          │
          └─> OR fallback to squareline/project/ if needed
              └─> components/ui_app/squareline/project/CMakeLists.txt
                  ├─> Glob all .c files recursively
                  └─> Compile + link as "ui" library
          
          └─> Link ui library with:
              ├─> lvgl (core graphics)
              └─> lvgl_esp32_drivers (display + touch drivers)

      └─> main/CMakeLists.txt
          └─> Compile app_main.c + gui.c
              └─> Link with ui app, FreeRTOS, ESP-IDF components
```

### Auto-Discovery Mechanism

Both `squareline/CMakeLists.txt` and `squareline/project/CMakeLists.txt` use:

```cmake
file(GLOB_RECURSE SOURCES "${CMAKE_CURRENT_LIST_DIR}/*.c")
list(FILTER SOURCES EXCLUDE REGEX ".*/(backup|cache)/.*\\.c$")
add_library(ui ${SOURCES})
```

**What this means:**
- ✅ New `.c` files in `screens/`, `images/`, `components/`, or root are automatically compiled
- ✅ Student can export from SquareLine without manually editing CMakeLists.txt
- ✅ Backup files ignored automatically
- ⚠️ Requires `idf.py reconfigure` after adding new files (not auto-detected in script mode)

---

## 📱 Hardware Configuration

### Display Controller: **ILI9341**
- **Resolution**: 320 × 240 pixels
- **Color Depth**: 16-bit RGB565 with byte-swap (`CONFIG_LV_COLOR_16_SWAP=y`)
- **Bus**: SPI (via GPIO pins in `sdkconfig`)

### Touch Controller: **XPT2046**
- **Sensing**: Resistive touch
- **Bus**: SPI (via GPIO pins in `sdkconfig`)

### Config Location
All hardware pins and controller settings are in `sdkconfig`:
- `CONFIG_LV_TFT_DISPLAY_CONTROLLER_ILI9341=y`
- `CONFIG_LV_TOUCH_CONTROLLER_XPT2046=y`
- `CONFIG_LV_HOR_RES_MAX=320`
- `CONFIG_LV_VER_RES_MAX=240`
- `CONFIG_LV_COLOR_DEPTH_16=y`
- `CONFIG_LV_COLOR_16_SWAP=y`

To modify GPIO pins or baudrates, use:
```powershell
idf.py menuconfig
```

---

## 🔧 Build Sequence

### First-Time Setup
```powershell
cd C:\wes\WES
git submodule update --init --recursive     # Clone LVGL, drivers
idf.py set-target esp32
idf.py build                                 # Applies patch automatically
idf.py -p COM3 flash                        # Flash to device (replace COM3 with your port)
idf.py monitor                              # View boot logs
```

### After SquareLine Export
```powershell
cd C:\wes\WES
# Export from SquareLine to C:\wes\WES\components\ui_app\squareline\
idf.py reconfigure                          # Rebuild CMake cache (required for new files)
idf.py build
idf.py -p COM3 flash
idf.py monitor
```

### Clean Rebuild
```powershell
idf.py fullclean                            # Remove all build artifacts
idf.py reconfigure
idf.py build
```

---

## 🎨 SquareLine Export Workflow

### Expected SquareLine Export Path
When exporting from SquareLine Studio, set the export path to:
```
C:\wes\WES\components\ui_app\squareline\
```

**This will generate:**
- `screens/ui_*.c` (screen callbacks)
- `images/ui_img_*.c` (image assets)
- `components/ui_comp_*.c` (custom components)
- `fonts/` (if custom fonts used)
- `ui.c`, `ui.h` (main UI init)
- `ui_events.c`, `ui_events.h` (event handler stubs)

All files are auto-discovered and compiled by CMake.

### Event Handler Pattern
Students add logic to `ui_events.c`:
```c
void ui_event_dark_mode_on(lv_event_t * e) {
    // Student event handler code here
}
```

---

## 📦 Submodule Management

### Update Submodules to Latest Remote
```powershell
git submodule update --remote --merge
git add components/lvgl components/lvgl_esp32_drivers
git commit -m "Updated LVGL submodules to latest"
```

### Check Submodule Status
```powershell
git status                                  # Shows submodule diff
git submodule status                        # Shows commit hashes
```

### Reinitialize Missing Submodules
```powershell
git submodule update --init --recursive
```

---

## ⚙️ Key Configuration Files

| File | Purpose |
|------|---------|
| `CMakeLists.txt` | Root ESP-IDF build config; includes all components |
| `sdkconfig` | Device-specific settings (GUI resolution, pins, controllers) |
| `sdkconfig.defaults` | Template for `sdkconfig`; committed to Git |
| `components/ui_app/CMakeLists.txt` | UI app build orchestrator; smart folder detection |
| `.gitmodules` | Submodule links; defines LVGL and driver source repos |
| `patches/lvgl_esp32_drivers_8-3.patch` | Compatibility patch for LVGL 8.3 API |

---

## 🛠️ Troubleshooting

### Q: "ui.c not found" CMake error
**A:** Export from SquareLine to `C:\wes\WES\components\ui_app\squareline\` (not `squareline/project/`).

### Q: New screens/images don't compile
**A:** Run `idf.py reconfigure` before `idf.py build`.

### Q: Submodule shows wrong commit hash
**A:** Run `git submodule update --init --recursive` to sync to pinned versions.

### Q: IntelliSense errors in VS Code ("cannot find lv_conf.h")
**A:** False positive. Project uses `CONFIG_LV_CONF_SKIP=y`. Errors don't affect build. Close/reopen file to refresh.

### Q: Display/touch not responding
**A:** Check `sdkconfig` GPIO pins match your hardware. Use `idf.py menuconfig` to adjust.

---

## 📚 References

- **LVGL Docs**: https://docs.lvgl.io/8.3/
- **SquareLine Studio**: https://squareline.io/
- **ESP-IDF**: https://docs.espressif.com/projects/esp-idf/en/v5.5.3/
- **ILI9341 Datasheet**: Display controller specs
- **XPT2046 Datasheet**: Touch controller specs

---

**Last Updated:** March 2026 | **ESP-IDF Version:** 5.5.3 | **LVGL Version:** 8.3.4

