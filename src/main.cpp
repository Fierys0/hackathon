#include "core/globals.hpp"
#include "fumbo.hpp"

#include "states/demoDesktop.hpp"
#include <memory>

int main() {
  Fumbo::Engine::Instance().Init(1280, 720, "Fumbo OS", 60);
  Fumbo::Assets::AddAssetPack("data.fpk");

  OS::LoadGlobalAssets();

  Fumbo::Engine::Instance().Run(std::make_shared<DemoDesktop>());

  OS::UnloadGlobalAssets();

  return 0;
}
