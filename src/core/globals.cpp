#include "globals.hpp"

namespace OS {

Font GlobalFont{};

void LoadGlobalAssets() {
  GlobalFont = Fumbo::Assets::LoadFont("assets/fonts/W95F.otf", 24);
  if (GlobalFont.texture.id == 0) {
    GlobalFont = GetFontDefault();
  }
}

void UnloadGlobalAssets() {
  if (GlobalFont.texture.id != 0 && GlobalFont.texture.id != GetFontDefault().texture.id) {
    UnloadFont(GlobalFont);
  }
}

} // namespace OS
