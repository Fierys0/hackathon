#include "globals.hpp"

namespace OS {

Font GlobalFont{};

void LoadGlobalAssets() {
  // Load default font: replace with a custom font from assets if desired
  // e.g. GlobalFont = Fumbo::Assets::LoadFont("assets/fonts/MyFont.ttf", 96);
  GlobalFont = GetFontDefault();
}

void UnloadGlobalAssets() {
  // Unload custom fonts here if loaded
  // e.g. UnloadFont(GlobalFont);
}

} // namespace OS
