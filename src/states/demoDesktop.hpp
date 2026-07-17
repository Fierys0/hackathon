#pragma once
#include "../core/os_desktop.hpp"
#include "fumbo.hpp"
#include "raylib.h"
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
  int m_shiftNumber;
  int m_currentTimeWindowIndex;
  std::string m_currentTimeLabel;
  bool m_shiftSummaryVisible;
  bool m_shiftBriefingVisible;

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
                           const std::string &sector, const std::string &status,
                           const std::string &cost, const std::string &outcome);
  std::string GetCurrentTimeLabel() const;
  void AdvanceTimeWindow();

  // Window content builders
  static void DrawTerminalContent(Rectangle area);
  static void DrawSettingsContent(Rectangle area);
  static void DrawFileManagerContent(Rectangle area);
  void DrawNotesContent(Rectangle area);
  static void DrawAboutContent(Rectangle area);
  void DrawThreatCenterContent(Rectangle area);
  void DrawCommsContent(Rectangle area);
  void DrawMitigationHub(Rectangle area);
  void DrawDecisionLog(Rectangle area);

  // Texture & UI elements
  Texture2D m_wallpaper;
  Texture2D m_bgIconTex;
  Texture2D m_threatIconTex;
  Texture2D m_commsIconTex;
  Texture2D m_mitigationIconTex;
  Texture2D m_bookmarkIconTex;

  Fumbo::UI::Textbox m_notesTextbox;
  int m_notesWindowId;
};
