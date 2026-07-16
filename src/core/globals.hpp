#pragma once
#include "fumbo.hpp"
#include <string>

// Global assets and styles shared across all states

namespace OS {

// Global font
extern Font GlobalFont;

// Load/unload global assets (call from main)
void LoadGlobalAssets();
void UnloadGlobalAssets();

} // namespace OS
