#include "core/globals.hpp"
#include "fumbo.hpp"
#include "states/titlescreen.hpp"
#include <memory>

int main() {
  Fumbo::Engine::Instance().Init(1280, 720, "Sentinel", 60);
  Fumbo::Assets::AddAssetPack("data.fpk");

  OS::LoadGlobalAssets();

  Fumbo::Engine::Instance().Run(std::make_shared<TitleScreen>());

  OS::UnloadGlobalAssets();

  return 0;
}
