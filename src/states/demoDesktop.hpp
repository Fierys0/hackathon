#pragma once
#include "../core/os_desktop.hpp"
#include "fumbo.hpp"

// DemoDesktop: Demo state showing OS simulation features

class DemoDesktop : public IGameState {
public:
  void Init() override;
  void Cleanup() override;
  void Update() override;
  void DrawClean() override;
  void DrawDirty() override;

private:
  std::shared_ptr<OS::OSDesktop> m_desktop;

  // Setup helpers (called from Init)
  void SetupDesktopIcons();
  void SetupStartMenu();
  void SetupContextMenu();
  void SetupSystemTray();

  // Window content builders
  static void DrawTerminalContent(Rectangle area);
  static void DrawSettingsContent(Rectangle area);
  static void DrawFileManagerContent(Rectangle area);
  static void DrawNotesContent(Rectangle area);
  static void DrawAboutContent(Rectangle area);
};
