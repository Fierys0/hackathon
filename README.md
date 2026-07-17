# 🛡️ Sentinels

> **GarudaHacks 7.0** — A disaster-response crisis management simulator built in C++ with raylib.

Sentinels puts you in the role of an emergency operations officer managing multi-hazard disasters across three shifts. You monitor live telemetry, allocate rescue teams, issue warnings, open shelters, and communicate with AI-driven field contacts — all inside a retro Windows 95-style desktop environment.

---

## 📋 Table of Contents

- [Features](#-features)
- [Prerequisites](#-prerequisites)
- [Building on Linux](#-building-on-linux)
- [Building for Windows (cross-compile)](#-building-for-windows-cross-compile)
- [Running the Game](#-running-the-game)
- [API Key Setup (Gemini AI)](#-api-key-setup-gemini-ai)
- [Project Structure](#-project-structure)
- [Tech Stack](#-tech-stack)
- [How to Play](#-how-to-play)

---

## ✨ Features

- **Windows 95/98 aesthetic** desktop OS simulation — taskbar, draggable windows, context menus, system tray, notifications.
- **Three disaster types**: Flood, Wildfire, Earthquake — each with unique telemetry metrics, threat progression, and SOP guidance.
- **Turn-based decision system**: Each shift has four operational windows (08:00 → 11:00 → 14:00 → 17:00). Choose actions each turn: issue warnings, deploy rescue teams, open shelters, close roads, request air support, or evacuate residents.
- **AI-powered features** (requires Gemini API key):
  - **Dynamic Threat Reports** — real-time AI-generated situational assessments every window.
  - **Field Radio chatbot** — converse with AI-driven field contacts (Hendro, Budi, Surono) for ground-level telemetry advice.
  - **After-Action Reports** — AI-generated shift debriefs with ratings, outcomes, and recommendations.
- **Decision Log** — review all your actions and their outcomes across all shifts.
- **Campaign progression** — survive three shifts to complete the campaign.
- **Asset packing** — all game assets are packed into a single `data.fpk` binary for fast loading.

---

## 🔧 Prerequisites

### Linux

| Dependency | Why |
|---|---|
| `cmake` >= 3.16 | Build system |
| `g++` with C++17 | Compiler |
| `libcurl-dev` | AI features (Gemini API calls) |
| `libGL`, `libX11`, `libXrandr`, `libXinerama`, `libXcursor`, `libXi` | raylib display |

```bash
# Ubuntu / Debian
sudo apt install cmake g++ libcurl4-openssl-dev \
    libgl1-mesa-dev libx11-dev libxrandr-dev \
    libxinerama-dev libxcursor-dev libxi-dev
```

### Windows (cross-compile from Linux)

```bash
sudo apt install mingw-w64 cmake
```

> The Windows build uses a **bundled static curl** from `lib/windows/curl/`. No system curl is needed on the Windows target machine.

---

## 🐧 Building on Linux

```bash
# 1. Clone with submodules (engine and raylib)
git clone --recurse-submodules https://github.com/your-org/sentinels.git
cd sentinels

# 2. Configure
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

# 3. Build
cmake --build build --parallel $(nproc)

# 4. Output is in build/hackathon/
ls build/hackathon/
```

Run the executable from its directory so relative paths resolve correctly:

```bash
cd build/hackathon
./hackathon
```

---

## 🪟 Building for Windows (cross-compile)

A convenience script is provided:

```bash
chmod +x build_windows.sh
./build_windows.sh
```

This runs CMake with the MinGW toolchain at `cmake/windows-mingw-toolchain.cmake` and produces output in `build_windows/hackathon/`. Copy the **entire** `build_windows/hackathon/` folder to the Windows machine to run.

> **Note:** The Windows build links curl statically — no extra DLLs are needed.

---

## 🚀 Running the Game

After building, the output directory (`build/hackathon/`) contains:

```
hackathon(.exe)
data.fpk              <- packed game assets
config/
  api_key.txt         <- your Gemini API key (see below)
curl-ca-bundle.crt    <- (Windows only) TLS certificate bundle
```

**Always run the executable from inside its directory** so relative paths resolve correctly:

```bash
cd build/hackathon
./hackathon
```

---

## 🔑 API Key Setup (Gemini AI)

The AI features (dynamic reports, field radio, after-action reports) require a **Google Gemini API key**.

### Step 1 — Get a key

1. Go to [Google AI Studio](https://aistudio.google.com/app/apikey).
2. Sign in with a Google account.
3. Click **"Create API key"**.
4. Copy the generated key.

### Step 2 — Place it in the config file

Create (or edit) the file `config/api_key.txt` in the **project root**:

```
config/api_key.txt
```

Paste your key as a **single line** with no spaces and no quotes:

```
AIzaSyXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
```

> The file must not contain the placeholder text `YOUR_GEMINI_API_KEY_HERE` and must not start with `#`.

### Step 3 — Build (or copy manually)

CMake automatically copies `config/` next to the built executable after every build. If `config/` doesn't exist at configure time, an empty directory is created at the output — drop your key file there instead.

### Where the key is read at runtime

The game searches for the key file in this order:

1. `config/api_key.txt` — next to the executable **(primary)**
2. `../config/api_key.txt`
3. `../../config/api_key.txt`
4. `build/hackathon/config/api_key.txt`

If no valid key is found, **AI features are silently disabled** — the game is fully playable without them.

### Keeping your key private

`config/api_key.txt` is already in `.gitignore`. **Never commit your real key to git.**

---

## 📁 Project Structure

```
sentinels/
├── assets/                   # Game assets (textures, fonts, audio, icons)
│   ├── fonts/
│   ├── icons/
│   ├── images/
│   └── music/
├── cmake/                    # CMake toolchain files (MinGW cross-compile)
├── config/
│   └── api_key.txt           # Gemini API key (gitignored)
├── engine/                   # Fumbo engine (internal framework on raylib)
│   ├── fumbo/                # Engine subsystems (audio, 2D graphics, UI, shaders)
│   ├── fumbo.cpp / fumbo.hpp # Engine entry point
│   ├── lib/raylib/           # raylib (git submodule)
│   └── tools/                # Asset packer tool
├── lib/
│   └── windows/curl/         # Bundled static curl for Windows builds
├── src/
│   ├── core/                 # OS simulation layer
│   │   ├── globals.*         # Shared assets (GlobalFont)
│   │   ├── os_desktop.*      # Desktop environment
│   │   ├── os_window.*       # Draggable windows
│   │   ├── os_taskbar.*      # Taskbar & system tray
│   │   ├── os_notification.* # Toast notifications
│   │   └── os_context_menu.* # Right-click context menus
│   ├── game/                 # Game logic
│   │   ├── game_manager.*    # Orchestrates game state + scenario
│   │   ├── game_state.*      # Player resources, actions, shift stats
│   │   ├── scenario.*        # Scripted disaster scenarios & time windows
│   │   ├── simulation.*      # Turn execution, action processing, outcomes
│   │   ├── threat_center.*   # Telemetry data model
│   │   ├── comms.*           # Field communications data model
│   │   └── ai_service.*      # Gemini API integration (reports + chat)
│   ├── states/
│   │   ├── titlescreen.*     # Title screen & settings modal
│   │   └── demoDesktop.*     # Main gameplay desktop state
│   └── main.cpp              # Entry point
├── build_windows.sh          # Windows cross-compile helper script
└── CMakeLists.txt            # Build configuration
```

---

## 🛠️ Tech Stack

| Component | Technology |
|---|---|
| Language | C++17 |
| Graphics & windowing | [raylib](https://www.raylib.com/) |
| Engine framework | Fumbo (custom, built on raylib) |
| AI | Google Gemini API (`gemini-2.0-flash`) |
| HTTP client | libcurl |
| Build system | CMake 3.16+ |
| Asset pipeline | Custom `.fpk` binary pack format |

---

## 🎮 How to Play

1. **Launch the game** — you are placed in the Sentinels OS retro desktop.
2. **Double-click Threat Center** to monitor live disaster telemetry for the current time window.
3. **Double-click Field Comms** to read incoming reports from field units.
4. **Double-click Mitigation Hub** to queue your response actions for this window.
5. **Click "End Time Window"** in the status bar to execute your decisions and advance to the next time slot.
6. After four windows (08:00 → 11:00 → 14:00 → 17:00), **click "End Shift"** to complete the shift and receive an after-action report.
7. **Survive three shifts** to complete the campaign.

> **Tip:** Issue early warnings and open shelters before a threat escalates — it significantly improves your people-saved count and public trust score.
>
> **Tip:** With a Gemini API key, use the **Field Radio** app to ask field contacts for real-time situation assessments that adapt to your decisions.

---

*Built for GarudaHacks 7.0 · 2026*
