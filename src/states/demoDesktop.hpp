#pragma once
#include "../core/os_desktop.hpp"
#include "fumbo.hpp"
#include <vector>

// DemoDesktop: Demo state showing OS simulation features

class DemoDesktop : public IGameState {
public:
  void Init() override;
  void Cleanup() override;
  void Update() override;
  void DrawClean() override;
  void DrawDirty() override;

private:
  struct DecisionLogEntry {
    std::string timestamp;
    std::string title;
    std::string disaster;
    std::string sector;
    std::string status;
    std::string cost;
    std::string outcome;
  };

  std::shared_ptr<OS::OSDesktop> m_desktop;

  // Game state variables for status bar
  int m_day;
  int m_budget;
  int m_publicTrust;
  std::string m_cityStatus;

  std::vector<DecisionLogEntry> m_decisionLog;
  int m_selectedDecisionLogEntry;
  int m_decisionLogScroll;

  // Setup helpers (called from Init)
  void SetupDesktopIcons();
  void SetupStartMenu();
  void SetupContextMenu();
  void SetupSystemTray();
  void DrawStatusBar();
  void AddDecisionLogEntry(const std::string &title,
                           const std::string &disaster,
                           const std::string &sector,
                           const std::string &status,
                           const std::string &cost,
                           const std::string &outcome);

  // Window content builders
  static void DrawTerminalContent(Rectangle area);
  static void DrawSettingsContent(Rectangle area);
  static void DrawFileManagerContent(Rectangle area);
  static void DrawNotesContent(Rectangle area);
  static void DrawAboutContent(Rectangle area);
  static void DrawThreatCenterContent(Rectangle area);
  static void DrawCommsContent(Rectangle area);
  void DrawMitigationHub(Rectangle area);
  void DrawDecisionLog(Rectangle area);
};
