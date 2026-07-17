#include "demoDesktop.hpp"
#include "../game/game_manager.hpp"
#include "../game/simulation.hpp"
#include "../core/globals.hpp"
#include "fumbo.hpp"
#include <algorithm>
#include <cstdio>
#include <ctime>
#include <string>

static int s_selectedCommsCard = -1;    // Tracks selected card in Comms UI
static int s_mitigationTab = 0;         // 0=Flood, 1=Wildfire, 2=Volcano
static bool s_queuedActions[3][5] = {}; // [tab][action] queued state
static int s_hoveredMitAction = -1;     // Currently hovered action index
static bool s_showEndShiftConfirm = false;
// IGameState interface: delegates to OSDesktop

void DemoDesktop::Init() {
  m_desktop = std::make_shared<OS::OSDesktop>();
  m_desktop->SetFont(OS::GlobalFont);

  // Initialize status bar game state
  m_day = 1;
  m_budget = 50000;
  m_publicTrust = 85;
  m_cityStatus = "ALERT";
  m_shiftNumber = 1;
  m_currentTimeWindowIndex = 0;
  m_currentTimeLabel = "08:00";
  m_shiftSummaryVisible = false;
  m_shiftBriefingVisible = false;
  m_decisionLog.clear();
  m_selectedDecisionLogEntry = -1;
  m_decisionLogScroll = 0;
  s_showEndShiftConfirm = false;

  // Load Windows 95/98 icons
  m_bgIconTex = Fumbo::Assets::LoadTexture("assets/icons/bgicons.png");
  m_threatIconTex = Fumbo::Assets::LoadTexture("assets/icons/threat.png");
  m_commsIconTex = Fumbo::Assets::LoadTexture("assets/icons/comms.png");
  m_mitigationIconTex =
      Fumbo::Assets::LoadTexture("assets/icons/mitigation.png");
  m_bookmarkIconTex = Fumbo::Assets::LoadTexture("assets/icons/bookmark.png");

  // Setup Notepad-like textbox
  m_notesWindowId = -1;
  m_notesTextbox = Fumbo::UI::Textbox({0, 0, 100, 100}, OS::GlobalFont, 14);
  m_notesTextbox.SetMultiline(true);
  m_notesTextbox.SetBackgroundColor(WHITE);
  m_notesTextbox.SetTextColor(BLACK);
  m_notesTextbox.SetOutlineColor({128, 128, 128, 255}, {10, 36, 106, 255});
  m_notesTextbox.SetPadding({6.0f, 6.0f});
  m_notesTextbox.SetText(
      "- Buy groceries\n- Finish the OS simulation\n- Submit hackathon "
      "project\n- Learn more C++\n- Sleep more :)");

  // Apply classic window styles
  OS::WindowStyle winStyle;
  winStyle.titleBarColor = {128, 128, 128, 255};    // Unfocused classic gray
  winStyle.titleBarFocusedColor = {0, 0, 128, 255}; // Focused blue
  winStyle.titleTextColor = WHITE;
  winStyle.bodyColor = {192, 192, 192, 255};   // Classic light gray body
  winStyle.borderColor = {128, 128, 128, 255}; // Silver border
  winStyle.borderThickness = 2.0f;
  winStyle.cornerRoundness = 0.0f; // Square corners
  winStyle.enableShadow = false;   // No modern shadow
  m_desktop->SetWindowStyle(winStyle);

  // Apply classic taskbar styles
  OS::TaskbarStyle taskStyle;
  taskStyle.backgroundColor = {192, 192, 192, 255};
  taskStyle.borderColor = {128, 128, 128, 255};
  taskStyle.itemColor = {192, 192, 192, 255};
  taskStyle.itemHoverColor = {210, 210, 210, 255};
  taskStyle.itemActiveColor = {160, 160, 160, 255}; // Sunken classic gray
  taskStyle.textColor = BLACK;
  taskStyle.startButtonColor = {192, 192, 192, 255};
  taskStyle.startButtonHoverColor = {210, 210, 210, 255};
  taskStyle.startMenuBg = {192, 192, 192, 255};
  taskStyle.startMenuBorder = {128, 128, 128, 255};
  taskStyle.startMenuItemHover = {0, 0, 128, 255};
  m_desktop->SetTaskbarStyle(taskStyle);

  // Load Texture
  m_wallpaper = Fumbo::Assets::LoadTexture("assets/images/background.png");

  // Setup all OS components
  SetupDesktopIcons();
  SetupStartMenu();
  SetupContextMenu();
  SetupSystemTray();

  // Initialize the desktop
  m_desktop->Init();

  // Welcome notification
  m_desktop->Notify("Welcome",
                    "Fumbo OS is ready. Double-click an icon to start.");

  m_desktop->SetWallpaper(m_wallpaper);
}

std::string DemoDesktop::GetCurrentTimeLabel() const { return m_currentTimeLabel; }

void DemoDesktop::AdvanceTimeWindow() {
  if (m_currentTimeWindowIndex < 3) {
    m_currentTimeWindowIndex++;
  }

  const char *times[] = {"08:00", "11:00", "14:00", "17:00"};
  m_currentTimeLabel = times[m_currentTimeWindowIndex];

  if (m_currentTimeWindowIndex == 1) {
    m_cityStatus = "WATCH";
    m_publicTrust = std::max(0, m_publicTrust - 2);
  } else if (m_currentTimeWindowIndex == 2) {
    m_cityStatus = "CRITICAL";
    m_publicTrust = std::max(0, m_publicTrust - 4);
    m_budget = std::max(0, m_budget - 4000);
  } else if (m_currentTimeWindowIndex == 3) {
    m_cityStatus = "EVACUATE";
    m_publicTrust = std::max(0, m_publicTrust - 6);
    m_budget = std::max(0, m_budget - 6000);
  }
}

void DemoDesktop::Cleanup() {
  if (m_desktop) {
    m_desktop->Cleanup();
    m_desktop.reset();
  }
}

void DemoDesktop::Update() {
  if (m_desktop) {
    m_desktop->Update();

    // Update notes textbox if Notes window is open and visible
    auto *notesWin = m_desktop->GetWindowManager().GetWindow(m_notesWindowId);
    if (notesWin && notesWin->IsVisible()) {
      m_notesTextbox.SetFocused(notesWin->IsFocused());
      m_notesTextbox.Update();
    }
  }
}

void DemoDesktop::DrawClean() {
  if (m_desktop)
    m_desktop->DrawClean();
}

void DemoDesktop::DrawDirty() {
  DrawStatusBar();

  if (m_desktop)
    m_desktop->DrawDirty();
}

// Setup Helpers

void DemoDesktop::SetupDesktopIcons() {
  // Threat Center icon
  m_desktop->AddDesktopIcon(
      "Threat Center", m_threatIconTex, m_bgIconTex, [this]() {
        m_desktop->OpenWindow(
            "Threat Center", {100, 80, 750, 450},
            [this](Rectangle area) { DrawThreatCenterContent(area); },
            m_threatIconTex);
        m_desktop->Notify("Threat Center",
                          "Threat Monitoring System activated at " +
                              GetCurrentTimeLabel() + ".");
      });

  // Comms icon
  m_desktop->AddDesktopIcon("Comms", m_commsIconTex, m_bgIconTex, [this]() {
    m_desktop->OpenWindow(
        "Comms", {100, 100, 850, 540},
        [this](Rectangle area) { DrawCommsContent(area); }, m_commsIconTex);
    m_desktop->Notify("Comms & Media",
                      "Comms activated at " + GetCurrentTimeLabel() + ".");
  });

  // Mitigation Hub icon
  m_desktop->AddDesktopIcon(
      "Mitigation Hub", m_mitigationIconTex, m_bgIconTex, [this]() {
        m_desktop->OpenWindow(
            "Mitigation Hub", {100, 100, 500, 350},
            [this](Rectangle area) { DrawMitigationHub(area); },
            m_mitigationIconTex);
        m_desktop->Notify("Mitigation Hub",
                          "Operational window active at " +
                              GetCurrentTimeLabel() + ".");
      });

  // Decision Log icon
  m_desktop->AddDesktopIcon(
      "Decision Log", m_bookmarkIconTex, m_bgIconTex, [this]() {
        m_desktop->OpenWindow(
            "Decision Log", {100, 100, 500, 350},
            [this](Rectangle area) { DrawDecisionLog(area); },
            m_bookmarkIconTex);
        m_desktop->Notify("Decision Log",
                          "Decision Log opened at " + GetCurrentTimeLabel() +
                              ".");
      });

  // Notes icon
  m_desktop->AddDesktopIcon("Notes", m_bookmarkIconTex, m_bgIconTex, [this]() {
    m_notesWindowId = m_desktop->OpenWindow(
        "Notes", {300, 140, 380, 280},
        [this](Rectangle area) { DrawNotesContent(area); }, m_bookmarkIconTex);
  });
}

void DemoDesktop::SetupStartMenu() {
  m_desktop->AddStartMenuItem("Terminal", [this]() {
    m_desktop->OpenWindow("Terminal", {200, 100, 500, 350},
                          DrawTerminalContent);
  });

  m_desktop->AddStartMenuItem("File Manager", [this]() {
    m_desktop->OpenWindow("File Manager", {180, 90, 520, 380},
                          DrawFileManagerContent);
    m_desktop->Notify("File Manager", "Opening file manager...");
  });

  m_desktop->AddStartMenuItem("Settings", [this]() {
    m_desktop->OpenWindow("Settings", {250, 120, 450, 320},
                          DrawSettingsContent);
    m_desktop->Notify("Settings", "Opening system settings...");
  });

  m_desktop->AddStartMenuItem("About", [this]() {
    m_desktop->OpenWindow("About Fumbo OS", {350, 200, 350, 180},
                          DrawAboutContent);
  });
}

void DemoDesktop::SetupContextMenu() {
  auto &ctxMenu = m_desktop->GetDesktopContextMenu();

  ctxMenu.AddItem("New Window", [this]() {
    static int count = 1;
    std::string title = "Window " + std::to_string(count++);
    m_desktop->OpenWindow(
        title, {150.0f + count * 20.0f, 80.0f + count * 15.0f, 420.0f, 280.0f},
        [title](Rectangle area) {
          Fumbo::Graphic2D::DrawText(
              "Content of " + title, {area.x + 15.0f, area.y + 15.0f},
              GetFontDefault(), 14, {200, 200, 230, 255});
          Fumbo::Graphic2D::DrawText(
              "Drag the title bar to move", {area.x + 15.0f, area.y + 40.0f},
              GetFontDefault(), 12, {150, 150, 180, 255});
          Fumbo::Graphic2D::DrawText(
              "Drag edges to resize", {area.x + 15.0f, area.y + 58.0f},
              GetFontDefault(), 12, {150, 150, 180, 255});
        });
  });

  ctxMenu.AddItem("Send Notification", [this]() {
    m_desktop->Notify("Hello!",
                      "This is a test notification from the desktop.");
  });

  ctxMenu.AddSeparator();

  ctxMenu.AddItem("About Fumbo OS", [this]() {
    m_desktop->OpenWindow("About", {350, 200, 350, 200}, DrawAboutContent);
  });
}

void DemoDesktop::SetupSystemTray() {
  m_desktop->AddTrayItem(
      "clock",
      [this](Rectangle area) {
        std::string dateText = "07/17/2026";
        Font font = GetFontDefault();

        Vector2 timeSize = MeasureTextEx(font, GetCurrentTimeLabel().c_str(), 10, 1.0f);
        Vector2 dateSize = MeasureTextEx(font, dateText.c_str(), 8, 1.0f);

        float totalHeight = timeSize.y + 2.0f + dateSize.y;
        float startY = area.y + (area.height - totalHeight) * 0.5f;

        Fumbo::Graphic2D::DrawText(GetCurrentTimeLabel(),
                                   {area.x + (area.width - timeSize.x) * 0.5f,
                                    startY},
                                   font, 10, BLACK);
        Fumbo::Graphic2D::DrawText(dateText,
                                   {area.x + (area.width - dateSize.x) * 0.5f,
                                    startY + timeSize.y + 2.0f},
                                   font, 8, BLACK);
      },
      nullptr, 58.0f);
}

// Window Content Drawers (static: reusable from anywhere)

void DemoDesktop::DrawTerminalContent(Rectangle area) {
  Fumbo::Graphic2D::DrawRectangleRec(area, {15, 15, 25, 255});

  float y = area.y + 10.0f;
  float x = area.x + 10.0f;
  Font font = GetFontDefault();

  Fumbo::Graphic2D::DrawText("user@fumbo-os:~$", {x, y}, font, 12,
                             {80, 220, 80, 255});
  Fumbo::Graphic2D::DrawText("echo \"Hello, World!\"", {x + 130.0f, y}, font,
                             12, {200, 200, 220, 255});
  y += 18.0f;
  Fumbo::Graphic2D::DrawText("Hello, World!", {x, y}, font, 12,
                             {200, 200, 220, 255});
  y += 18.0f;
  Fumbo::Graphic2D::DrawText("user@fumbo-os:~$ _", {x, y}, font, 12,
                             {80, 220, 80, 255});
}

void DemoDesktop::DrawSettingsContent(Rectangle area) {
  float y = area.y + 15.0f;
  float x = area.x + 15.0f;
  Font font = GetFontDefault();

  Fumbo::Graphic2D::DrawText("System Settings", {x, y}, font, 15, WHITE);
  y += 30.0f;

  const char *settings[] = {"Display", "Sound", "Network", "Appearance",
                            "Privacy"};
  for (int i = 0; i < 5; i++) {
    Rectangle itemRect = {x, y, area.width - 30.0f, 30.0f};
    Color bg = (i % 2 == 0) ? Color{40, 40, 60, 200} : Color{35, 35, 52, 200};
    Fumbo::Graphic2D::DrawRectangleRounded(itemRect, 0.1f, 4, bg);
    Fumbo::Graphic2D::DrawText(settings[i], {x + 12.0f, y + 7.0f}, font, 12,
                               {200, 200, 230, 255});
    Fumbo::Graphic2D::DrawText(">", {x + area.width - 50.0f, y + 7.0f}, font,
                               12, {120, 120, 160, 255});
    y += 34.0f;
  }
}

void DemoDesktop::DrawFileManagerContent(Rectangle area) {
  float y = area.y + 10.0f;
  float x = area.x + 10.0f;
  Font font = GetFontDefault();

  // Path bar
  Rectangle pathBar = {x, y, area.width - 20.0f, 24.0f};
  Fumbo::Graphic2D::DrawRectangleRounded(pathBar, 0.15f, 4, {40, 40, 60, 220});
  Fumbo::Graphic2D::DrawText("/home/user/Documents", {x + 8.0f, y + 5.0f}, font,
                             11, {180, 180, 210, 255});
  y += 32.0f;

  // File list
  const char *files[] = {"Documents/", "Pictures/", "Music/",
                         "readme.txt", "notes.md",  "config.json"};
  const char *sizes[] = {"--", "--", "--", "4.2 KB", "1.1 KB", "256 B"};

  for (int i = 0; i < 6; i++) {
    Rectangle itemRect = {x, y, area.width - 20.0f, 26.0f};
    Color bg = (i % 2 == 0) ? Color{38, 38, 56, 200} : Color{34, 34, 50, 200};
    Fumbo::Graphic2D::DrawRectangleRec(itemRect, bg);

    Color dotColor =
        (i < 3) ? Color{80, 160, 240, 255} : Color{180, 180, 200, 255};
    Fumbo::Graphic2D::DrawCircleV({x + 12.0f, y + 13.0f}, 4.0f, dotColor);
    Fumbo::Graphic2D::DrawText(files[i], {x + 24.0f, y + 6.0f}, font, 11,
                               {210, 210, 235, 255});
    Fumbo::Graphic2D::DrawText(sizes[i], {x + area.width - 80.0f, y + 6.0f},
                               font, 10, {130, 130, 160, 255});
    y += 26.0f;
  }
}

void DemoDesktop::DrawNotesContent(Rectangle area) {
  Fumbo::Graphic2D::DrawRectangleRec(area, {192, 192, 192, 255});
  m_notesTextbox.SetBounds(
      {area.x + 2.0f, area.y + 2.0f, area.width - 4.0f, area.height - 4.0f});
  m_notesTextbox.Draw();
}

void DemoDesktop::DrawAboutContent(Rectangle area) {
  Font font = GetFontDefault();
  Fumbo::Graphic2D::DrawText("Fumbo OS Simulation",
                             {area.x + 15.0f, area.y + 15.0f}, font, 16, WHITE);
  Fumbo::Graphic2D::DrawText("Built with Fumbo Engine",
                             {area.x + 15.0f, area.y + 40.0f}, font, 12,
                             {180, 180, 200, 255});
  Fumbo::Graphic2D::DrawText("GarudaHacks 2026",
                             {area.x + 15.0f, area.y + 58.0f}, font, 12,
                             {140, 140, 180, 255});
}

void DemoDesktop::DrawThreatCenterContent(Rectangle area) {
  // 1. Draw solid dark background for this app window
  Fumbo::Graphic2D::DrawRectangleRec(area, {20, 24, 33, 255});

  float padding = 10.0f;
  Font font = GetFontDefault();

  // 2. Define Layout columns (58% left map, 42% right SOP)
  float leftWidth = area.width * 0.58f;
  float rightWidth = area.width * 0.42f;

  float leftX = area.x + padding;
  float leftY = area.y + padding;
  float leftW = leftWidth - (padding * 1.5f);
  float leftH = area.height - (padding * 2.0f);

  // Draw header for Map section
  Fumbo::Graphic2D::DrawText("REGIONAL MONITORING MAP",
                             {leftX + 5.0f, leftY + 5.0f}, font, 13,
                             {0, 230, 118, 255});
  Fumbo::Graphic2D::DrawText("CURRENT TIME: " + GetCurrentTimeLabel(),
                             {leftX + 5.0f, leftY + 20.0f}, font, 10,
                             {255, 215, 0, 255});

  // Draw simulated map box
  Rectangle mapBox = {leftX, leftY + 25.0f, leftW, leftH - 25.0f};
  Fumbo::Graphic2D::DrawRectangleRounded(mapBox, 0.05f, 4, {28, 33, 46, 255});
  Fumbo::Graphic2D::DrawRectangleRoundedLinesEx(mapBox, 0.05f, 4, 1.0f,
                                                {50, 60, 80, 255});

  // Draw map grid lines
  for (int gx = 1; gx < 5; gx++) {
    float gridX = mapBox.x + (mapBox.width / 5) * gx;
    Fumbo::Graphic2D::DrawLineEx({gridX, mapBox.y},
                                 {gridX, mapBox.y + mapBox.height}, 1.0f,
                                 {38, 44, 60, 150});
  }
  for (int gy = 1; gy < 4; gy++) {
    float gridY = mapBox.y + (mapBox.height / 4) * gy;
    Fumbo::Graphic2D::DrawLineEx({mapBox.x, gridY},
                                 {mapBox.x + mapBox.width, gridY}, 1.0f,
                                 {38, 44, 60, 150});
  }

  // Draw 3 sectors and threat monitoring sensors
  // Sector A (North): Flood Monitoring Zone
  Vector2 secACenter = {mapBox.x + mapBox.width * 0.5f,
                        mapBox.y + mapBox.height * 0.28f};
  Fumbo::Graphic2D::DrawCircleV(secACenter, 7.0f,
                                {41, 121, 255, 255}); // Blue for water
  Fumbo::Graphic2D::DrawText("SEC-A [RIVER]",
                             {secACenter.x - 38.0f, secACenter.y - 18.0f}, font,
                             10, {150, 180, 220, 255});

  // Sector B (South-West): Seismic Monitoring Zone
  Vector2 secBCenter = {mapBox.x + mapBox.width * 0.25f,
                        mapBox.y + mapBox.height * 0.72f};
  Fumbo::Graphic2D::DrawCircleV(secBCenter, 7.0f,
                                {255, 145, 0, 255}); // Orange for seismic
  Fumbo::Graphic2D::DrawText("SEC-B [SEISMIC]",
                             {secBCenter.x - 45.0f, secBCenter.y - 18.0f}, font,
                             10, {220, 180, 150, 255});

  // Sector C (South-East): Wildfire Danger Zone
  Vector2 secCCenter = {mapBox.x + mapBox.width * 0.75f,
                        mapBox.y + mapBox.height * 0.68f};
  Fumbo::Graphic2D::DrawCircleV(secCCenter, 7.0f,
                                {255, 23, 68, 255}); // Red for fire danger
  Fumbo::Graphic2D::DrawText("SEC-C [WILDFIRE]",
                             {secCCenter.x - 48.0f, secCCenter.y - 18.0f}, font,
                             10, {255, 150, 150, 255});

  // 3. Draw Vertical Divider
  float dividerX = area.x + leftWidth;
  Fumbo::Graphic2D::DrawLineEx({dividerX, area.y + padding},
                               {dividerX, area.y + area.height - padding}, 1.0f,
                               {60, 68, 90, 255});

  // Right Column Content: SOP Rules Reference
  float rightX = dividerX + (padding * 0.5f);
  float rightY = area.y + padding;
  float rightW = rightWidth - (padding * 1.5f);
  float rightH = area.height - (padding * 2.0f);

  // Draw header for SOP Reference Drawer
  Fumbo::Graphic2D::DrawText("MONITORING PROTOCOLS",
                             {rightX + 5.0f, rightY + 5.0f}, font, 13,
                             {255, 215, 0, 255});

  // Draw a content frame for SOP text
  Rectangle sopBox = {rightX, rightY + 25.0f, rightW, rightH - 25.0f};
  Fumbo::Graphic2D::DrawRectangleRounded(sopBox, 0.05f, 4, {23, 27, 38, 255});
  Fumbo::Graphic2D::DrawRectangleRoundedLinesEx(sopBox, 0.05f, 4, 1.0f,
                                                {50, 60, 80, 255});

  // Draw actual text guidelines inside SOP Drawer
  float textY = sopBox.y + 12.0f;
  float textX = sopBox.x + 12.0f;

  Fumbo::Graphic2D::DrawText("RULE-01: FLOODING LEVEL", {textX, textY}, font,
                             11, {140, 180, 255, 255});
  textY += 15.0f;
  Fumbo::Graphic2D::DrawText("- Trigger if River Depth > 4.5m",
                             {textX + 8.0f, textY}, font, 10,
                             {180, 180, 210, 255});
  textY += 12.0f;
  Fumbo::Graphic2D::DrawText("  OR Rainfall rate > 80mm/h",
                             {textX + 8.0f, textY}, font, 10,
                             {180, 180, 210, 255});
  textY += 22.0f;

  Fumbo::Graphic2D::DrawText("RULE-02: WILDFIRE DANGER", {textX, textY}, font,
                             11, {255, 140, 140, 255});
  textY += 15.0f;
  Fumbo::Graphic2D::DrawText("- Trigger if Temp > 38.0 C",
                             {textX + 8.0f, textY}, font, 10,
                             {180, 180, 210, 255});
  textY += 12.0f;
  Fumbo::Graphic2D::DrawText("  AND Air Humidity < 15%", {textX + 8.0f, textY},
                             font, 10, {180, 180, 210, 255});
  textY += 22.0f;

  Fumbo::Graphic2D::DrawText("RULE-03: SEISMIC ACTIVITY", {textX, textY}, font,
                             11, {255, 220, 140, 255});
  textY += 15.0f;
  Fumbo::Graphic2D::DrawText("- Alert if Tremors > 12 / hr",
                             {textX + 8.0f, textY}, font, 10,
                             {180, 180, 210, 255});
  textY += 12.0f;
  Fumbo::Graphic2D::DrawText("  OR gas density > 350ppm", {textX + 8.0f, textY},
                             font, 10, {180, 180, 210, 255});
  textY += 24.0f;

  // Visual divider
  Fumbo::Graphic2D::DrawLineEx({textX, textY}, {textX + rightW - 24.0f, textY},
                               1.0f, {60, 68, 90, 255});
  textY += 10.0f;

  // Real-time sensor checklist snippet
  Fumbo::Graphic2D::DrawText("LIVE MONITOR STATUS:", {textX, textY}, font, 11,
                             {0, 230, 118, 255});
  textY += 16.0f;
  // Read backend threat values for the current window. Fall back to defaults
  // if the scenario isn't populated for some reason.
  const auto &scenario = m_gameManager.GetScenario();
  Game::ThreatCenterState threat;
  if (!scenario.shifts.empty()) {
    const auto &shift = scenario.shifts.at(0);
    if (m_currentTimeWindowIndex >= 0 && m_currentTimeWindowIndex < static_cast<int>(shift.windows.size())) {
      threat = shift.windows.at(static_cast<std::size_t>(m_currentTimeWindowIndex)).threatCenter;
    }
  }

  // SEC-A: Flood sensors
  char bufA[128];
  std::snprintf(bufA, sizeof(bufA), "SEC-A: %.1fm | %dmm/h (%s)",
                threat.flood.riverDepth,
                static_cast<int>(threat.flood.rainfall),
                (threat.flood.riverDepth >= 4.6f || threat.flood.rainfall >= 80.0f) ? "WARN" : "OK");
  std::string monitorA(bufA);

  // SEC-B: Seismic
  char bufB[128];
  std::snprintf(bufB, sizeof(bufB), "SEC-B: %d tremors/h (%s)",
                threat.earthquake.tremorsPerHour,
                (threat.earthquake.tremorsPerHour >= 11) ? "WATCH" : "OK");
  std::string monitorB(bufB);

  // SEC-C: Wildfire
  char bufC[128];
  std::snprintf(bufC, sizeof(bufC), "SEC-C: %d C | Hum %d%% (%s)",
                static_cast<int>(threat.wildfire.temperature),
                static_cast<int>(threat.wildfire.humidity),
                (threat.wildfire.temperature >= 41 || threat.wildfire.humidity <= 9) ? "CRITICAL" : ((threat.wildfire.temperature >= 39 || threat.wildfire.humidity <= 12) ? "WARN" : "OK"));
  std::string monitorC(bufC);
  Fumbo::Graphic2D::DrawText(monitorA, {textX + 8.0f, textY}, font, 10,
                             {180, 235, 180, 255});
  textY += 14.0f;
  Fumbo::Graphic2D::DrawText(monitorB, {textX + 8.0f, textY}, font, 10,
                             {180, 235, 180, 255});
  textY += 14.0f;
  Fumbo::Graphic2D::DrawText(monitorC, {textX + 8.0f, textY}, font, 10,
                             {255, 140, 140, 255});
}

void DemoDesktop::AddDecisionLogEntry(const std::string &title,
                                      const std::string &disaster,
                                      const std::string &sector,
                                      const std::string &status,
                                      const std::string &cost,
                                      const std::string &outcome) {
  DecisionLogEntry entry{};
  entry.title = title;
  entry.disaster = disaster;
  entry.sector = sector;
  entry.status = status;
  entry.cost = cost;
  entry.outcome = outcome;

  std::time_t now = std::time(nullptr);
  std::tm *timeInfo = std::localtime(&now);
  char stamp[6];
  std::snprintf(stamp, sizeof(stamp), "%02d:%02d", timeInfo->tm_hour,
                timeInfo->tm_min);
  entry.timestamp = stamp;

  m_decisionLog.push_back(entry);
  m_selectedDecisionLogEntry = static_cast<int>(m_decisionLog.size()) - 1;
}

void DemoDesktop::DrawDecisionLog(Rectangle area) {
  Fumbo::Graphic2D::DrawRectangleRec(area, {20, 24, 33, 255});

  Font font = GetFontDefault();
  float padding = 12.0f;
  Vector2 mouse = GetMousePosition();

  float summaryH = 84.0f;
  Rectangle summaryBar = {area.x + padding, area.y + padding,
                          area.width - 2.0f * padding, summaryH};
  Fumbo::Graphic2D::DrawRectangleRounded(summaryBar, 0.05f, 4,
                                         {24, 29, 40, 255});
  Fumbo::Graphic2D::DrawRectangleRoundedLinesEx(summaryBar, 0.05f, 4, 1.0f,
                                                {60, 70, 92, 255});

  Fumbo::Graphic2D::DrawText("TODAY'S SHIFT SUMMARY",
                             {summaryBar.x + 10.0f, summaryBar.y + 10.0f}, font,
                             11, {255, 215, 0, 255});

  // Read completed shift summaries from backend GameState
  Game::GameState &gs = m_gameManager.GetGameState();
  const auto &log = gs.decisionLog;

  std::string totalActions = std::to_string(log.size());
  int budgetSpentInt = 0;
  if (!log.empty()) budgetSpentInt = log.back().budgetSpent;
  std::string budgetSpent = "$" + std::to_string(std::max(0, budgetSpentInt));
  std::string trustDelta = (m_publicTrust >= 85) ? "+0" : std::to_string(m_publicTrust - 85);
  if (m_publicTrust != 85 && trustDelta[0] != '-') {
    trustDelta = "+" + trustDelta;
  }
  std::string primaryThreat = "No completed shifts";
  if (!log.empty()) {
    primaryThreat = "Shift " + std::to_string(log.back().shiftNumber);
  }
  std::string shiftStatus = log.empty() ? "No completed shifts" : "Review active";

  float colW = (summaryBar.width - 40.0f) / 5.0f;
  float summaryY = summaryBar.y + 30.0f;
  const char *labels[] = {"Actions", "Budget", "Trust", "Threat", "Status"};
  const char *values[] = {totalActions.c_str(), budgetSpent.c_str(),
                          trustDelta.c_str(), primaryThreat.c_str(),
                          shiftStatus.c_str()};
  for (int i = 0; i < 5; i++) {
    float x = summaryBar.x + 12.0f + i * colW;
    Fumbo::Graphic2D::DrawText(labels[i], {x, summaryY}, font, 9,
                               {150, 150, 170, 255});
    Fumbo::Graphic2D::DrawText(values[i], {x, summaryY + 16.0f}, font, 10,
                               {220, 220, 235, 255});
  }

  float listX = area.x + padding;
  float listY = summaryBar.y + summaryBar.height + 10.0f;
  float listW = area.width * 0.46f - padding;
  float listH = area.height - (listY - area.y) - padding;

  Rectangle timelinePanel = {listX, listY, listW, listH};
  Fumbo::Graphic2D::DrawRectangleRounded(timelinePanel, 0.04f, 4,
                                         {24, 29, 40, 255});
  Fumbo::Graphic2D::DrawRectangleRoundedLinesEx(timelinePanel, 0.04f, 4, 1.0f,
                                                {60, 70, 92, 255});
  Fumbo::Graphic2D::DrawText("DECISION TIMELINE",
                             {timelinePanel.x + 10.0f, timelinePanel.y + 10.0f},
                             font, 11, {0, 230, 118, 255});

  if (log.empty()) {
    Fumbo::Graphic2D::DrawText(
        "No completed shifts.",
        {timelinePanel.x + 12.0f, timelinePanel.y + 42.0f}, font, 10,
        {140, 140, 160, 255});
    Fumbo::Graphic2D::DrawText(
        "Complete a shift to generate a summary here.",
        {timelinePanel.x + 12.0f, timelinePanel.y + 60.0f}, font, 9,
        {110, 110, 135, 255});
  } else {
    float wheel = GetMouseWheelMove();
    if (wheel != 0.0f) {
      int maxScroll = std::max(0, static_cast<int>(log.size()) - 6);
      m_decisionLogScroll =
          std::clamp(m_decisionLogScroll + (wheel > 0 ? -1 : 1), 0, maxScroll);
    }

    float rowY = timelinePanel.y + 36.0f;
    int visibleRows = 6;
    for (int i = 0; i < visibleRows; i++) {
      int index = m_decisionLogScroll + i;
      if (index >= static_cast<int>(log.size()))
        break;

      const Game::GameState::ShiftSummary &entry = log[index];
      Rectangle rowRect = {timelinePanel.x + 8.0f, rowY + i * 42.0f,
                           timelinePanel.width - 16.0f, 34.0f};
      bool hovered = CheckCollisionPointRec(mouse, rowRect);
      bool selected = (index == m_selectedDecisionLogEntry);

      Color rowBg = selected ? Color{32, 42, 58, 255}
                             : (hovered ? Color{36, 42, 59, 255}
                                        : Color{26, 31, 43, 255});
      Fumbo::Graphic2D::DrawRectangleRounded(rowRect, 0.05f, 4, rowBg);
      Color borderColor =
          selected ? Color{0, 230, 118, 255} : Color{55, 63, 86, 255};
      Fumbo::Graphic2D::DrawRectangleRoundedLinesEx(rowRect, 0.05f, 4, 1.0f,
                                                    borderColor);

      // Shift number
      Fumbo::Graphic2D::DrawText(std::to_string(entry.shiftNumber),
                                 {rowRect.x + 8.0f, rowRect.y + 8.0f}, font, 9,
                                 {255, 215, 0, 255});
      Fumbo::Graphic2D::DrawText(
          std::string("Shift Summary"), {rowRect.x + 58.0f, rowRect.y + 8.0f}, font, 9, WHITE);
      Fumbo::Graphic2D::DrawText(std::to_string(entry.peopleSaved) + " saved",
                                 {rowRect.x + 58.0f, rowRect.y + 18.0f}, font,
                                 8, {150, 150, 170, 255});

      // Rating badge
      Rectangle badgeRect = {rowRect.x + rowRect.width - 64.0f,
                             rowRect.y + 8.0f, 56.0f, 16.0f};
      Color ratingColor = {255, 23, 68, 255};
      if (entry.overallRating >= 4.0f) ratingColor = {0, 230, 118, 255};
      else if (entry.overallRating >= 2.0f) ratingColor = {255, 215, 0, 255};
      Fumbo::Graphic2D::DrawRectangleRounded(badgeRect, 0.2f, 4, ratingColor);
      char ratingBuf[8];
      std::snprintf(ratingBuf, sizeof(ratingBuf), "%.1f/5", entry.overallRating);
      Fumbo::Graphic2D::DrawText(std::string(ratingBuf),
                                 {badgeRect.x + 8.0f, badgeRect.y + 3.0f}, font,
                                 8, WHITE);

      if (hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        m_selectedDecisionLogEntry = index;
      }
    }
  }

  float detailX = area.x + padding + listW + padding;
  float detailY = listY;
  float detailW = area.width - (detailX - area.x) - padding;
  float detailH = listH;

  Rectangle detailPanel = {detailX, detailY, detailW, detailH};
  Fumbo::Graphic2D::DrawRectangleRounded(detailPanel, 0.04f, 4,
                                         {24, 29, 40, 255});
  Fumbo::Graphic2D::DrawRectangleRoundedLinesEx(detailPanel, 0.04f, 4, 1.0f,
                                                {60, 70, 92, 255});
  Fumbo::Graphic2D::DrawText("ACTION DETAILS",
                             {detailPanel.x + 10.0f, detailPanel.y + 10.0f},
                             font, 11, {255, 215, 0, 255});

  if (log.empty() || m_selectedDecisionLogEntry < 0 ||
      m_selectedDecisionLogEntry >= static_cast<int>(log.size())) {
    Fumbo::Graphic2D::DrawText("Select a completed shift to inspect its details.",
                               {detailPanel.x + 12.0f, detailPanel.y + 42.0f},
                               font, 10, {140, 140, 160, 255});
  } else {
    const Game::GameState::ShiftSummary &entry = log[m_selectedDecisionLogEntry];
    float dy = detailPanel.y + 40.0f;
    auto DrawDetailRow = [&](const std::string &label,
                             const std::string &value) {
      Fumbo::Graphic2D::DrawText(label, {detailPanel.x + 12.0f, dy}, font, 9,
                                 {150, 150, 170, 255});
      Fumbo::Graphic2D::DrawText(value, {detailPanel.x + 180.0f, dy}, font, 9,
                                 {220, 220, 235, 255});
      dy += 18.0f;
    };

    DrawDetailRow("Shift", std::to_string(entry.shiftNumber));
    DrawDetailRow("Overall Rating", std::to_string(entry.overallRating));
    DrawDetailRow("People Saved", std::to_string(entry.peopleSaved));
    DrawDetailRow("Casualties", std::to_string(entry.casualties));
    DrawDetailRow("Budget Spent", std::string("$") + std::to_string(entry.budgetSpent));
    DrawDetailRow("Public Trust", std::to_string(entry.publicTrust) + "%");
    if (!entry.actionsTaken.empty()) {
      std::string actions;
      for (const auto &a : entry.actionsTaken) {
        if (!actions.empty()) actions += ", ";
        actions += a;
      }
      DrawDetailRow("Actions", actions);
    }
  }
}

void DemoDesktop::DrawCommsContent(Rectangle area) {
  // 1. Draw solid dark background for this app window
  Fumbo::Graphic2D::DrawRectangleRec(area, {20, 24, 33, 255});

  struct CommsCard {
    std::string category;
    std::string icon;
    std::string headline;
    std::string timestamp;
    std::string priority; // "LOW", "MEDIUM", "HIGH", "CRITICAL"
    std::string summary;
    std::string fullText;
    Color priorityColor;
    bool unread;
  };

  // Helper to map ReportPriority to string and color
  auto PriorityToString = [](Game::ReportPriority p) {
    switch (p) {
    case Game::ReportPriority::Info:
      return std::pair<std::string, Color>("LOW", Color{0, 230, 118, 255});
    case Game::ReportPriority::Medium:
      return std::pair<std::string, Color>("MEDIUM", Color{255, 215, 0, 255});
    case Game::ReportPriority::High:
      return std::pair<std::string, Color>("HIGH", Color{255, 145, 0, 255});
    case Game::ReportPriority::Critical:
    default:
      return std::pair<std::string, Color>("CRITICAL", Color{255, 23, 68, 255});
    }
  };

  // Read current comms from backend
  const Game::CommsState &comms = m_gameManager.GetCurrentComms();

  CommsCard cards[6];
  // 0: BreakingNews
  {
    auto pr = PriorityToString(comms.breakingNews.priority);
    cards[0] = {"BREAKING NEWS", "[!]", comms.breakingNews.title,
                comms.breakingNews.timestamp, pr.first, comms.breakingNews.preview,
                comms.breakingNews.fullReport, pr.second, comms.breakingNews.unread};
  }
  // 1: Weather
  {
    auto pr = PriorityToString(comms.weather.priority);
    cards[1] = {"WEATHER UPDATE", "[W]", comms.weather.title,
                comms.weather.timestamp, pr.first, comms.weather.preview,
                comms.weather.fullReport, pr.second, comms.weather.unread};
  }
  // 2: Citizen
  {
    auto pr = PriorityToString(comms.citizen.priority);
    cards[2] = {"CITIZEN REPORT", "[C]", comms.citizen.title,
                comms.citizen.timestamp, pr.first, comms.citizen.preview,
                comms.citizen.fullReport, pr.second, comms.citizen.unread};
  }
  // 3: Sensor
  {
    auto pr = PriorityToString(comms.sensor.priority);
    cards[3] = {"CCTV / SENSOR", "[S]", comms.sensor.title,
                comms.sensor.timestamp, pr.first, comms.sensor.preview,
                comms.sensor.fullReport, pr.second, comms.sensor.unread};
  }
  // 4: Rescue
  {
    auto pr = PriorityToString(comms.rescue.priority);
    cards[4] = {"RESCUE REPORT", "[R]", comms.rescue.title,
                comms.rescue.timestamp, pr.first, comms.rescue.preview,
                comms.rescue.fullReport, pr.second, comms.rescue.unread};
  }
  // 5: Infrastructure
  {
    auto pr = PriorityToString(comms.infrastructure.priority);
    cards[5] = {"INFRASTRUCTURE", "[I]", comms.infrastructure.title,
                comms.infrastructure.timestamp, pr.first,
                comms.infrastructure.preview, comms.infrastructure.fullReport,
                pr.second, comms.infrastructure.unread};
  }

  float padding = 12.0f;
  float gap = 10.0f;
  Font font = GetFontDefault();
  std::string currentTime = GetCurrentTimeLabel();

  // Grid calculation split
  float leftW = (area.width - 3.0f * padding) * 0.48f;
  float rightW = (area.width - 3.0f * padding) * 0.52f;
  float leftH = area.height - 2.0f * padding;

  // Track if mouse is over any card
  Vector2 mouse = GetMousePosition();

  // Helper to draw a single card
  auto DrawCard = [&](int index, Rectangle r, bool isFeatured) {
    bool hovered = CheckCollisionPointRec(mouse, r);

    // Handle click interaction
    if (hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
        s_selectedCommsCard == -1) {
      s_selectedCommsCard = index;
    }

    Color cardBg = hovered ? Color{36, 42, 59, 255} : Color{28, 33, 46, 255};
    Color borderColor =
        isFeatured ? Color{255, 23, 68, 150} : Color{50, 60, 80, 255};

    Fumbo::Graphic2D::DrawRectangleRounded(r, 0.04f, 4, cardBg);
    Fumbo::Graphic2D::DrawRectangleRoundedLinesEx(r, 0.04f, 4, 1.0f,
                                                  borderColor);

    float x = r.x + 12.0f;
    float y = r.y + 12.0f;

    // Draw Category & icon
    std::string catStr = cards[index].icon + " " + cards[index].category;
    Fumbo::Graphic2D::DrawText(catStr, {x, y}, font, isFeatured ? 11 : 9,
                               {0, 230, 118, 255});

    // Priority badge
    float badgeW = isFeatured ? 65.0f : 50.0f;
    float badgeH = isFeatured ? 18.0f : 14.0f;
    Rectangle badgeRect = {r.x + r.width - badgeW - 12.0f, y - 2.0f, badgeW,
                           badgeH};
    Fumbo::Graphic2D::DrawRectangleRounded(badgeRect, 0.2f, 4,
                                           cards[index].priorityColor);
    Fumbo::Graphic2D::DrawText(cards[index].priority,
                               {badgeRect.x + (isFeatured ? 8.0f : 5.0f),
                                badgeRect.y + (isFeatured ? 4.0f : 2.0f)},
                               font, isFeatured ? 9 : 8, WHITE);

    // Headline
    y += isFeatured ? 22.0f : 16.0f;
    float titleSize = isFeatured ? 13.0f : 10.0f;

    // Simple headline trimmer or layout
    std::string headline = cards[index].headline;
    if (!isFeatured && headline.length() > 38) {
      headline = headline.substr(0, 35) + "...";
    }
    Fumbo::Graphic2D::DrawText(headline, {x, y}, font, titleSize, WHITE);

    // Timestamp (from backend)
    y += isFeatured ? 20.0f : 15.0f;
    Fumbo::Graphic2D::DrawText(std::string("RECEIVED: ") + cards[index].timestamp, {x, y}, font, 8,
                   {140, 140, 160, 255});

    // Divider line
    y += isFeatured ? 15.0f : 12.0f;
    Fumbo::Graphic2D::DrawLineEx({x, y}, {r.x + r.width - 12.0f, y}, 1.0f,
                                 {60, 70, 90, 180});
    y += isFeatured ? 12.0f : 8.0f;

    // Summary lines
    if (isFeatured) {
      Fumbo::Graphic2D::DrawText("OPERATIONAL DISPATCH SUMMARY:", {x, y}, font,
                                 11, {255, 23, 68, 255});
      y += 18.0f;
      Fumbo::Graphic2D::DrawText(
          "- Dam actuator systems jammed in open-risk position.", {x, y}, font,
          10, {210, 210, 230, 255});
      y += 15.0f;
      Fumbo::Graphic2D::DrawText("- Water level rising at 0.5 meters per hour.",
                                 {x, y}, font, 10, {210, 210, 230, 255});
      y += 15.0f;
      Fumbo::Graphic2D::DrawText("- Heavy storm fronts converging upstream.",
                                 {x, y}, font, 10, {210, 210, 230, 255});
      y += 24.0f;
      Fumbo::Graphic2D::DrawText("> CLICK FOR FULL INTEL REPORT", {x, y}, font,
                                 9, {255, 215, 0, 255});
    } else {
      std::string summary = cards[index].summary;
      if (summary.length() > 40) {
        std::string line1 = summary.substr(0, 38) + "-";
        std::string line2 = summary.substr(38);
        if (line2.length() > 38)
          line2 = line2.substr(0, 35) + "...";
        Fumbo::Graphic2D::DrawText(line1, {x, y}, font, 9,
                                   {170, 170, 190, 255});
        y += 12.0f;
        Fumbo::Graphic2D::DrawText(line2, {x, y}, font, 9,
                                   {170, 170, 190, 255});
      } else {
        Fumbo::Graphic2D::DrawText(summary, {x, y}, font, 9,
                                   {170, 170, 190, 255});
      }
    }
  };

  // --- 2. Draw Cards ---
  // Card 0: Featured Breaking News Card (Left Column)
  Rectangle rect0 = {area.x + padding, area.y + padding, leftW, leftH};
  DrawCard(0, rect0, true);

  // Right Column calculations
  float rightX = area.x + padding + leftW + padding;
  float colW = (rightW - gap) * 0.5f;

  float col1X = rightX;
  float col2X = rightX + colW + gap;

  // Col 1: 2 stacked cards (Card 1, Card 2)
  float col1H = (leftH - gap) * 0.5f;
  Rectangle rect1 = {col1X, area.y + padding, colW, col1H};
  Rectangle rect2 = {col1X, area.y + padding + col1H + gap, colW, col1H};
  DrawCard(1, rect1, false);
  DrawCard(2, rect2, false);

  // Col 2: 3 stacked cards (Card 3, Card 4, Card 5)
  float col2H = (leftH - 2.0f * gap) / 3.0f;
  Rectangle rect3 = {col2X, area.y + padding, colW, col2H};
  Rectangle rect4 = {col2X, area.y + padding + col2H + gap, colW, col2H};
  Rectangle rect5 = {col2X, area.y + padding + 2.0f * (col2H + gap), colW,
                     col2H};
  DrawCard(3, rect3, false);
  DrawCard(4, rect4, false);
  DrawCard(5, rect5, false);

  // --- 3. Draw Article Modal Overlay if selected ---
  if (s_selectedCommsCard >= 0 && s_selectedCommsCard < 6) {
    Fumbo::Graphic2D::DrawRectangleRec(area,
                                       {12, 14, 21, 220}); // Dark blur overlay

    float modalW = area.width * 0.75f;
    float modalH = area.height * 0.78f;
    float modalX = area.x + (area.width - modalW) * 0.5f;
    float modalY = area.y + (area.height - modalH) * 0.5f;
    Rectangle modalRect = {modalX, modalY, modalW, modalH};

    Fumbo::Graphic2D::DrawRectangleRounded(modalRect, 0.03f, 4,
                                           {23, 27, 38, 255});
    Fumbo::Graphic2D::DrawRectangleRoundedLinesEx(
        modalRect, 0.03f, 4, 1.5f, cards[s_selectedCommsCard].priorityColor);

    float mx = modalX + 24.0f;
    float my = modalY + 24.0f;

    // Category banner
    std::string catText = cards[s_selectedCommsCard].icon + " " +
                          cards[s_selectedCommsCard].category;
    Fumbo::Graphic2D::DrawText(catText, {mx, my}, font, 11, {0, 230, 118, 255});

    // Priority badge
    float badgeW = 75.0f;
    float badgeH = 20.0f;
    Rectangle badgeRect = {modalX + modalW - badgeW - 24.0f, my - 2.0f, badgeW,
                           badgeH};
    Fumbo::Graphic2D::DrawRectangleRounded(
        badgeRect, 0.2f, 4, cards[s_selectedCommsCard].priorityColor);
    Fumbo::Graphic2D::DrawText(cards[s_selectedCommsCard].priority,
                               {badgeRect.x + 12.0f, badgeRect.y + 4.0f}, font,
                               9, WHITE);

    // Headline
    my += 22.0f;
    Fumbo::Graphic2D::DrawText(cards[s_selectedCommsCard].headline, {mx, my},
                               font, 13, WHITE);

    // Classification stamp with backend timestamp
    my += 20.0f;
    Fumbo::Graphic2D::DrawText(std::string("TIME: ") + cards[s_selectedCommsCard].timestamp +
                     "  |  STATUS: CLASSIFIED INTEL",
                   {mx, my}, font, 9, {140, 140, 160, 255});

    // Divider
    my += 16.0f;
    Fumbo::Graphic2D::DrawLineEx({mx, my}, {modalX + modalW - 24.0f, my}, 1.0f,
                                 {60, 70, 90, 255});
    my += 18.0f;

    // Full briefing title
    Fumbo::Graphic2D::DrawText("SITUATIONAL LOG BRIEFING:", {mx, my}, font, 10,
                               {255, 215, 0, 255});
    my += 20.0f;

    // Wrap fullText into paragraphs
    std::string text = cards[s_selectedCommsCard].fullText;
    int lineStart = 0;
    int lineLength = 65; // Characters per line limit
    while (lineStart < (int)text.length()) {
      std::string line = text.substr(lineStart, lineLength);
      if (lineStart + lineLength < (int)text.length()) {
        size_t lastSpace = line.find_last_of(' ');
        if (lastSpace != std::string::npos) {
          line = line.substr(0, lastSpace);
          lineStart += lastSpace + 1;
        } else {
          lineStart += lineLength;
        }
      } else {
        lineStart += lineLength;
      }
      Fumbo::Graphic2D::DrawText(line, {mx, my}, font, 10,
                                 {210, 210, 225, 255});
      my += 16.0f;
    }

    // Modal Close Button (Bottom Right)
    float btnW = 90.0f;
    float btnH = 26.0f;
    Rectangle closeBtn = {modalX + modalW - btnW - 24.0f,
                          modalY + modalH - btnH - 24.0f, btnW, btnH};
    bool btnHovered = CheckCollisionPointRec(mouse, closeBtn);

    Color btnBg = btnHovered ? Color{60, 70, 90, 255} : Color{38, 45, 61, 255};
    Fumbo::Graphic2D::DrawRectangleRounded(closeBtn, 0.15f, 4, btnBg);
    Fumbo::Graphic2D::DrawRectangleRoundedLinesEx(closeBtn, 0.15f, 4, 1.0f,
                                                  {80, 90, 110, 255});
    Fumbo::Graphic2D::DrawText(
        "CLOSE [X]", {closeBtn.x + 18.0f, closeBtn.y + 7.0f}, font, 9, WHITE);

    if (btnHovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
      s_selectedCommsCard = -1; // Dismiss modal
    }
  }
}

void DemoDesktop::DrawMitigationHub(Rectangle area) {
  // Dark background
  Fumbo::Graphic2D::DrawRectangleRec(area, {20, 24, 33, 255});

  Font font = GetFontDefault();
  float padding = 12.0f;
  Vector2 mouse = GetMousePosition();
  s_hoveredMitAction = -1; // Reset each frame

  // --- Action Data ---
  struct ActionInfo {
    const char *name;
    const char *budgetImpact;
    const char *resourceUse;
    const char *benefits;
    const char *drawbacks;
  };

  const char *tabNames[3] = {"FLOOD", "WILDFIRE", "VOLCANO"};
  const char *tabIcons[3] = {"~", "*", "^"};
  Color tabColors[3] = {
      {41, 121, 255, 255}, {255, 145, 0, 255}, {255, 23, 68, 255}};

  ActionInfo actions[3][5] = {
      // Flood
      {{"Issue Early Warning", "-", "+", "Early public awareness",
        "May cause panic"},
       {"Deploy Rescue Team", "--", "++", "Direct life-saving",
        "Team unavailable elsewhere"},
       {"Open Floodgates", "-", "+", "Reduce reservoir pressure",
        "Downstream flooding risk"},
       {"Close Bridge", "-", "+", "Prevent casualties",
        "Blocks evacuation route"},
       {"Evacuate Residents", "---", "+++", "Maximum safety",
        "High cost & public unrest"}},
      // Wildfire
      {{"Issue Smoke Advisory", "-", "+", "Public health protection",
        "Economic slowdown"},
       {"Deploy Firefighters", "--", "++", "Direct containment",
        "Personnel at risk"},
       {"Request Water Bomber", "---", "+", "Rapid fire suppression",
        "Very expensive"},
       {"Evacuate Villages", "--", "+++", "Civilian safety",
        "Displacement costs"},
       {"Close National Park", "-", "+", "Prevent tourist deaths",
        "Revenue loss"}},
      // Volcano
      {{"Raise Alert Level", "-", "+", "Preparedness boost", "Public anxiety"},
       {"Evacuate Danger Zone", "---", "+++", "Maximum safety",
        "Massive logistics"},
       {"Close Airport", "--", "+", "Aircraft safety", "Economic disruption"},
       {"Prepare Shelters", "--", "++", "Safe havens ready",
        "Resource intensive"},
       {"Deploy Medical Teams", "--", "++", "Rapid response ready",
        "Teams committed"}}};

  Fumbo::Graphic2D::DrawText("SHIFT " + std::to_string(m_shiftNumber),
                             {area.x + padding, area.y + padding + 2.0f}, font,
                             10, {255, 215, 0, 255});
  Fumbo::Graphic2D::DrawText("TIME: " + GetCurrentTimeLabel(),
                             {area.x + padding + 90.0f, area.y + padding + 2.0f},
                             font, 10, {0, 230, 118, 255});
  std::string progressText = "WINDOW " + std::to_string(m_currentTimeWindowIndex + 1) + " / 4";
  Fumbo::Graphic2D::DrawText(progressText,
                             {area.x + padding + 210.0f, area.y + padding + 2.0f},
                             font, 10, {140, 180, 255, 255});

  // ========== 1. RESOURCE OVERVIEW BAR ==========
  float barH = 34.0f;
  Rectangle resBar = {area.x + padding, area.y + padding,
                      area.width - 2 * padding, barH};
  Fumbo::Graphic2D::DrawRectangleRounded(resBar, 0.08f, 4, {28, 33, 46, 255});
  Fumbo::Graphic2D::DrawRectangleRoundedLinesEx(resBar, 0.08f, 4, 1.0f,
                                                {50, 60, 80, 255});

  float rx = resBar.x + 14.0f;
  float ry = resBar.y + 10.0f;
  float colSpacing = (resBar.width - 28.0f) / 3.0f;

  Fumbo::Graphic2D::DrawText("BUDGET:", {rx, ry}, font, 9,
                             {150, 150, 170, 255});
  Fumbo::Graphic2D::DrawText("$50,000", {rx + 55.0f, ry}, font, 10,
                             {140, 255, 140, 255});

  Fumbo::Graphic2D::DrawText("PUBLIC TRUST:", {rx + colSpacing, ry}, font, 9,
                             {150, 150, 170, 255});
  Fumbo::Graphic2D::DrawText("85%", {rx + colSpacing + 90.0f, ry}, font, 10,
                             {0, 230, 118, 255});

  Fumbo::Graphic2D::DrawText("RESCUE TEAMS:", {rx + 2 * colSpacing, ry}, font,
                             9, {150, 150, 170, 255});
  // Live values from backend GameState
  {
    Game::GameState &gs = m_gameManager.GetGameState();
    Fumbo::Graphic2D::DrawText(std::string("$") + std::to_string(gs.budget), {rx + 55.0f, ry}, font, 10,
                               {140, 255, 140, 255});
    Fumbo::Graphic2D::DrawText(std::to_string(gs.publicTrust) + "%", {rx + colSpacing + 90.0f, ry}, font, 10,
                               {0, 230, 118, 255});
    Fumbo::Graphic2D::DrawText(std::to_string(gs.rescueTeams) + " AVAILABLE", {rx + 2 * colSpacing + 95.0f, ry}, font, 10, {140, 180, 255, 255});
  }

  // ========== 2. DISASTER TABS ==========
  float tabY = area.y + padding + barH + 10.0f;
  float tabGap = 8.0f;
  float tabW = (area.width - 2 * padding - 2 * tabGap) / 3.0f;
  float tabH = 30.0f;

  for (int t = 0; t < 3; t++) {
    Rectangle tabRect = {area.x + padding + t * (tabW + tabGap), tabY, tabW,
                         tabH};
    bool isActive = (t == s_mitigationTab);
    bool tabHovered = CheckCollisionPointRec(mouse, tabRect);

    Color tabBg = isActive ? tabColors[t]
                           : (tabHovered ? Color{40, 48, 65, 255}
                                         : Color{28, 33, 46, 255});
    Fumbo::Graphic2D::DrawRectangleRounded(tabRect, 0.12f, 4, tabBg);
    if (!isActive) {
      Fumbo::Graphic2D::DrawRectangleRoundedLinesEx(tabRect, 0.12f, 4, 1.0f,
                                                    tabColors[t]);
    }

    std::string tabLabel = std::string("[") + tabIcons[t] + "] " + tabNames[t];
    Color tabTextColor = isActive ? WHITE : tabColors[t];
    float textOff = (tabW - 10.0f * (float)tabLabel.length()) * 0.5f;
    if (textOff < 8.0f)
      textOff = 8.0f;
    Fumbo::Graphic2D::DrawText(tabLabel,
                               {tabRect.x + textOff, tabRect.y + 9.0f}, font,
                               10, tabTextColor);

    if (tabHovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
      s_mitigationTab = t;
    }
  }

  // ========== 3. CONTENT AREA ==========
  float contentY = tabY + tabH + 10.0f;
  float contentH = area.height - (contentY - area.y) - padding;
  float leftW = (area.width - 3 * padding) * 0.55f;
  float rightW = (area.width - 3 * padding) * 0.45f;
  float leftX = area.x + padding;
  float rightX = leftX + leftW + padding;
  int tab = s_mitigationTab;

  // --- LEFT PANEL: Available Actions ---
  Rectangle leftPanel = {leftX, contentY, leftW, contentH};
  Fumbo::Graphic2D::DrawRectangleRounded(leftPanel, 0.03f, 4,
                                         {23, 27, 38, 255});
  Fumbo::Graphic2D::DrawRectangleRoundedLinesEx(leftPanel, 0.03f, 4, 1.0f,
                                                {50, 60, 80, 255});

  Fumbo::Graphic2D::DrawText("AVAILABLE ACTIONS",
                             {leftX + 12.0f, contentY + 10.0f}, font, 10,
                             tabColors[tab]);

  float actionY = contentY + 32.0f;
  float actionH = 30.0f;
  float actionGap = 4.0f;

  // Helper mapping from tab/action index -> Game::ActionID (available to the whole function)
  auto getActionId = [](int tabIdx, int actionIdx) -> Game::ActionID {
    using Game::ActionID;
    if (tabIdx == 0) {
      switch (actionIdx) {
      case 0: return ActionID::IssueWarning;
      case 1: return ActionID::DeployRescue;
      case 2: return ActionID::OpenShelter;
      case 3: return ActionID::CloseRoad;
      case 4: return ActionID::EvacuateResidents;
      }
    } else if (tabIdx == 1) {
      switch (actionIdx) {
      case 0: return ActionID::IssueWarning;
      case 1: return ActionID::DeployRescue;
      case 2: return ActionID::RequestAirSupport;
      case 3: return ActionID::EvacuateResidents;
      case 4: return ActionID::CloseRoad;
      }
    } else {
      switch (actionIdx) {
      case 0: return ActionID::IssueWarning;
      case 1: return ActionID::EvacuateResidents;
      case 2: return ActionID::CloseRoad;
      case 3: return ActionID::OpenShelter;
      case 4: return ActionID::DeployRescue;
      }
    }
    return ActionID::IssueWarning;
  };

  for (int i = 0; i < 5; i++) {
    Rectangle actionRect = {leftX + 6.0f, actionY, leftW - 12.0f, actionH};
    bool hovered = CheckCollisionPointRec(mouse, actionRect);
    // Determine if this action is currently selected in GameState
    Game::GameState &gs = m_gameManager.GetGameState();

    Game::ActionID aid = getActionId(tab, i);
    bool queued = (std::find(gs.selectedActions.begin(), gs.selectedActions.end(), aid) != gs.selectedActions.end());

    Color bg =
        queued ? Color{25, 50, 35, 255}
               : (hovered ? Color{36, 42, 59, 255} : Color{26, 30, 42, 255});
    Fumbo::Graphic2D::DrawRectangleRounded(actionRect, 0.06f, 4, bg);

    if (queued) {
      Fumbo::Graphic2D::DrawRectangleRoundedLinesEx(actionRect, 0.06f, 4, 1.0f,
                                                    {0, 230, 118, 200});
    } else if (hovered) {
      Fumbo::Graphic2D::DrawRectangleRoundedLinesEx(actionRect, 0.06f, 4, 1.0f,
                                                    {80, 90, 110, 200});
    }

    // Checkbox
    std::string checkStr = queued ? "[x]" : "[ ]";
    Color checkColor =
        queued ? Color{0, 230, 118, 255} : Color{120, 120, 150, 255};
    Fumbo::Graphic2D::DrawText(checkStr,
                               {actionRect.x + 8.0f, actionRect.y + 8.0f}, font,
                               10, checkColor);

    // Action name
    Fumbo::Graphic2D::DrawText(actions[tab][i].name,
                               {actionRect.x + 40.0f, actionRect.y + 8.0f},
                               font, 10, {210, 210, 230, 255});

    // Cost indicator on the right
    std::string costStr = "COST: " + std::string(actions[tab][i].budgetImpact);
    Fumbo::Graphic2D::DrawText(
        costStr, {actionRect.x + actionRect.width - 90.0f, actionRect.y + 8.0f},
        font, 9, {255, 140, 140, 255});

    // Toggle on click: add/remove ActionID from GameState.selectedActions
    if (hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
      auto it = std::find(gs.selectedActions.begin(), gs.selectedActions.end(), aid);
      if (it != gs.selectedActions.end()) {
        gs.selectedActions.erase(it);
      } else {
        gs.selectedActions.push_back(aid);
      }
    }

    if (hovered) {
      s_hoveredMitAction = i;
    }

    actionY += actionH + actionGap;
  }

  // --- PREVIEW PANEL (below actions, shows on hover) ---
  float previewY = actionY + 8.0f;
  float previewH = contentY + contentH - previewY - 8.0f;

  if (previewH > 50.0f && s_hoveredMitAction >= 0 && s_hoveredMitAction < 5) {
    int ai = s_hoveredMitAction;
    Rectangle previewRect = {leftX + 6.0f, previewY, leftW - 12.0f, previewH};
    Fumbo::Graphic2D::DrawRectangleRounded(previewRect, 0.04f, 4,
                                           {18, 22, 32, 255});
    Fumbo::Graphic2D::DrawRectangleRoundedLinesEx(previewRect, 0.04f, 4, 1.0f,
                                                  {60, 70, 90, 200});

    float px = previewRect.x + 12.0f;
    float py = previewRect.y + 10.0f;

    Fumbo::Graphic2D::DrawText("ACTION PREVIEW", {px, py}, font, 10,
                               {255, 215, 0, 255});
    py += 18.0f;

    Fumbo::Graphic2D::DrawLineEx({px, py}, {px + previewRect.width - 24.0f, py},
                                 1.0f, {50, 60, 80, 180});
    py += 10.0f;

    Fumbo::Graphic2D::DrawText("Budget Impact:", {px, py}, font, 9,
                               {150, 150, 170, 255});
    Fumbo::Graphic2D::DrawText(actions[tab][ai].budgetImpact, {px + 100.0f, py},
                               font, 10, {255, 140, 140, 255});
    py += 16.0f;

    Fumbo::Graphic2D::DrawText("Resource Use:", {px, py}, font, 9,
                               {150, 150, 170, 255});
    Fumbo::Graphic2D::DrawText(actions[tab][ai].resourceUse, {px + 100.0f, py},
                               font, 10, {140, 180, 255, 255});
    py += 16.0f;

    Fumbo::Graphic2D::DrawText("Benefits:", {px, py}, font, 9,
                               {150, 150, 170, 255});
    Fumbo::Graphic2D::DrawText(actions[tab][ai].benefits, {px + 100.0f, py},
                               font, 10, {0, 230, 118, 255});
    py += 16.0f;

    Fumbo::Graphic2D::DrawText("Drawbacks:", {px, py}, font, 9,
                               {150, 150, 170, 255});
    Fumbo::Graphic2D::DrawText(actions[tab][ai].drawbacks, {px + 100.0f, py},
                               font, 10, {255, 100, 100, 255});
  } else if (previewH > 30.0f && s_hoveredMitAction < 0) {
    Rectangle previewRect = {leftX + 6.0f, previewY, leftW - 12.0f, previewH};
    Fumbo::Graphic2D::DrawRectangleRounded(previewRect, 0.04f, 4,
                                           {18, 22, 32, 255});
    float px = previewRect.x + 12.0f;
    float py = previewRect.y + previewH * 0.5f - 6.0f;
    Fumbo::Graphic2D::DrawText("Hover over an action to see details", {px, py},
                               font, 9, {90, 90, 120, 255});
  }

  // ========== 4. RIGHT PANEL: Today's Action Queue ==========
  Rectangle rightPanel = {rightX, contentY, rightW, contentH};
  Fumbo::Graphic2D::DrawRectangleRounded(rightPanel, 0.03f, 4,
                                         {23, 27, 38, 255});
  Fumbo::Graphic2D::DrawRectangleRoundedLinesEx(rightPanel, 0.03f, 4, 1.0f,
                                                {50, 60, 80, 255});

  Fumbo::Graphic2D::DrawText("TODAY'S ACTION QUEUE",
                             {rightX + 12.0f, contentY + 10.0f}, font, 10,
                             {255, 215, 0, 255});

  // Divider below header
  float qDivY = contentY + 30.0f;
  Fumbo::Graphic2D::DrawLineEx({rightX + 10.0f, qDivY},
                               {rightX + rightW - 10.0f, qDivY}, 1.0f,
                               {50, 60, 80, 180});

  float queueY = qDivY + 8.0f;
  int queueCount = 0;

  // Build queue view from GameState.selectedActions
  Game::GameState &gsQueue = m_gameManager.GetGameState();
  for (int t = 0; t < 3; t++) {
    // collect actions for this tab
    std::vector<int> actionsInTab;
    for (int a = 0; a < 5; a++) {
      Game::ActionID aid = getActionId(t, a);
      if (std::find(gsQueue.selectedActions.begin(), gsQueue.selectedActions.end(), aid) != gsQueue.selectedActions.end()) {
        actionsInTab.push_back(a);
      }
    }
    if (actionsInTab.empty()) continue;

    // Tab section label
    std::string secLabel = std::string("[") + tabIcons[t] + "] " + tabNames[t];
    Fumbo::Graphic2D::DrawText(secLabel, {rightX + 12.0f, queueY}, font, 9, tabColors[t]);
    queueY += 16.0f;

    for (int idx : actionsInTab) {
      Rectangle qItemRect = {rightX + 8.0f, queueY, rightW - 16.0f, 24.0f};
      bool qHovered = CheckCollisionPointRec(mouse, qItemRect);

      Color qBg = qHovered ? Color{50, 30, 30, 255} : Color{26, 30, 42, 255};
      Fumbo::Graphic2D::DrawRectangleRounded(qItemRect, 0.06f, 4, qBg);

      // Colored left accent
      Rectangle accent = {qItemRect.x, qItemRect.y + 2.0f, 3.0f, qItemRect.height - 4.0f};
      Fumbo::Graphic2D::DrawRectangleRec(accent, tabColors[t]);

      // Action name
      std::string aName = actions[t][idx].name;
      if (aName.length() > 24) aName = aName.substr(0, 21) + "...";
      Fumbo::Graphic2D::DrawText(aName, {qItemRect.x + 10.0f, qItemRect.y + 6.0f}, font, 9, {210, 210, 230, 255});

      // Remove button
      float removeX = qItemRect.x + qItemRect.width - 26.0f;
      Color removeColor = qHovered ? Color{255, 80, 80, 255} : Color{120, 120, 150, 255};
      Fumbo::Graphic2D::DrawText("[X]", {removeX, qItemRect.y + 6.0f}, font, 9, removeColor);

      if (qHovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        // remove from GameState.selectedActions
        Game::ActionID aid = getActionId(t, idx);
        auto it = std::find(gsQueue.selectedActions.begin(), gsQueue.selectedActions.end(), aid);
        if (it != gsQueue.selectedActions.end()) gsQueue.selectedActions.erase(it);
      }

      queueY += 28.0f;
      queueCount++;
    }
    queueY += 4.0f;
  }

  if (queueCount == 0) {
    float emptyY = qDivY + 30.0f;
    Fumbo::Graphic2D::DrawText("No actions queued.", {rightX + 12.0f, emptyY},
                               font, 9, {120, 120, 150, 255});
    Fumbo::Graphic2D::DrawText("Select actions from the",
                               {rightX + 12.0f, emptyY + 18.0f}, font, 9,
                               {100, 100, 130, 255});
    Fumbo::Graphic2D::DrawText("left panel to queue them.",
                               {rightX + 12.0f, emptyY + 36.0f}, font, 9,
                               {100, 100, 130, 255});
  }

  // --- END SHIFT BUTTON ---
  float btnW = rightW - 24.0f;
  float btnH = 34.0f;
  Rectangle endBtn = {rightX + 12.0f, contentY + contentH - btnH - 12.0f, btnW,
                      btnH};
  bool endHovered = CheckCollisionPointRec(mouse, endBtn);

  bool canAdvance = queueCount > 0;
  std::string buttonLabel = (m_currentTimeWindowIndex < 3)
                                ? "EXECUTE PLAN & ADVANCE TIME"
                                : "COMPLETE SHIFT";

  Color endBg = (canAdvance)
                    ? (endHovered ? Color{200, 40, 40, 255} : Color{160, 25, 25, 255})
                    : Color{50, 50, 60, 255};
  Fumbo::Graphic2D::DrawRectangleRounded(endBtn, 0.1f, 4, endBg);
  Fumbo::Graphic2D::DrawRectangleRoundedLinesEx(
      endBtn, 0.1f, 4, 1.2f,
      canAdvance ? Color{255, 80, 80, 255} : Color{70, 70, 85, 255});

  Color btnTextColor = canAdvance ? WHITE : Color{100, 100, 110, 255};
  Fumbo::Graphic2D::DrawText(buttonLabel,
                             {endBtn.x + 12.0f, endBtn.y + 10.0f}, font, 10,
                             btnTextColor);

  // Queue count badge
  if (queueCount > 0) {
    std::string countStr = "[" + std::to_string(queueCount) + "]";
    Fumbo::Graphic2D::DrawText(countStr,
                               {endBtn.x + btnW - 30.0f, endBtn.y + 10.0f},
                               font, 10, {255, 215, 0, 255});
  }

  if (m_shiftSummaryVisible) {
    Fumbo::Graphic2D::DrawRectangleRec(area, {8, 10, 16, 230});
    Rectangle summaryRect = {area.x + 24.0f, area.y + 24.0f, area.width - 48.0f,
                             area.height - 48.0f};
    Fumbo::Graphic2D::DrawRectangleRounded(summaryRect, 0.03f, 4,
                                           {20, 24, 33, 255});
    Fumbo::Graphic2D::DrawRectangleRoundedLinesEx(summaryRect, 0.03f, 4, 1.2f,
                                                  {255, 215, 0, 255});
    Fumbo::Graphic2D::DrawText("SHIFT SUMMARY",
                               {summaryRect.x + 16.0f, summaryRect.y + 16.0f}, font,
                               13, {255, 215, 0, 255});
    Fumbo::Graphic2D::DrawText("Operational performance snapshot",
                               {summaryRect.x + 16.0f, summaryRect.y + 38.0f}, font,
                               10, {180, 180, 210, 255});

    std::string stats[][2] = {{"Lives Saved", "142"}, {"Casualties", "8"},
                              {"Infrastructure Damage", "24%"},
                              {"Budget Remaining", "$27,500"},
                              {"Public Trust Change", "+6%"},
                              {"Overall Rating", "4.2/5"}};
    float statY = summaryRect.y + 70.0f;
    for (int i = 0; i < 6; i++) {
      Fumbo::Graphic2D::DrawText(stats[i][0], {summaryRect.x + 16.0f, statY + i * 22.0f}, font, 10,
                                 {220, 220, 235, 255});
      Fumbo::Graphic2D::DrawText(stats[i][1], {summaryRect.x + summaryRect.width - 120.0f, statY + i * 22.0f}, font, 10,
                                 {0, 230, 118, 255});
    }

    Fumbo::Graphic2D::DrawText(
        "A family made it home tonight, but eight others were lost to the storm's delay.",
        {summaryRect.x + 16.0f, summaryRect.y + summaryRect.height - 86.0f}, font,
        10, {255, 180, 120, 255});
    Fumbo::Graphic2D::DrawText(
        "The city will remember who acted fast, and who hesitated too long.",
        {summaryRect.x + 16.0f, summaryRect.y + summaryRect.height - 66.0f}, font,
        10, {255, 120, 120, 255});

    Rectangle closeBtn = {summaryRect.x + summaryRect.width - 104.0f,
                          summaryRect.y + summaryRect.height - 40.0f, 88.0f,
                          24.0f};
    bool closeHovered = CheckCollisionPointRec(mouse, closeBtn);
    Fumbo::Graphic2D::DrawRectangleRounded(closeBtn, 0.1f, 4, closeHovered ? Color{80, 80, 95, 255} : Color{50, 55, 70, 255});
    Fumbo::Graphic2D::DrawText("CONTINUE", {closeBtn.x + 20.0f, closeBtn.y + 6.0f}, font, 9, WHITE);

    if (closeHovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
      m_shiftSummaryVisible = false;
      m_shiftBriefingVisible = true;
      m_shiftNumber++;
      m_currentTimeWindowIndex = 0;
      m_currentTimeLabel = "08:00";
      m_cityStatus = "ALERT";
      for (int t = 0; t < 3; t++)
        for (int a = 0; a < 5; a++)
          s_queuedActions[t][a] = false;
    }
  } else if (m_shiftBriefingVisible) {
    Fumbo::Graphic2D::DrawRectangleRec(area, {8, 10, 16, 230});
    Rectangle briefingRect = {area.x + 24.0f, area.y + 24.0f, area.width - 48.0f,
                              area.height - 48.0f};
    Fumbo::Graphic2D::DrawRectangleRounded(briefingRect, 0.03f, 4,
                                           {20, 24, 33, 255});
    Fumbo::Graphic2D::DrawRectangleRoundedLinesEx(briefingRect, 0.03f, 4, 1.2f,
                                                  {0, 230, 118, 255});
    Fumbo::Graphic2D::DrawText("OPERATIONAL BRIEFING",
                               {briefingRect.x + 16.0f, briefingRect.y + 16.0f}, font,
                               13, {0, 230, 118, 255});
    Fumbo::Graphic2D::DrawText("Next shift readiness update",
                               {briefingRect.x + 16.0f, briefingRect.y + 38.0f}, font,
                               10, {180, 180, 210, 255});
    Fumbo::Graphic2D::DrawText("- Recheck sensor calibration before dispatch.",
                               {briefingRect.x + 16.0f, briefingRect.y + 70.0f}, font,
                               10, {220, 220, 235, 255});
    Fumbo::Graphic2D::DrawText("- Prepare shelter and communications routes.",
                               {briefingRect.x + 16.0f, briefingRect.y + 92.0f}, font,
                               10, {220, 220, 235, 255});
    Fumbo::Graphic2D::DrawText("- Maintain public trust with clear updates.",
                               {briefingRect.x + 16.0f, briefingRect.y + 114.0f}, font,
                               10, {220, 220, 235, 255});

    Rectangle continueBtn = {briefingRect.x + briefingRect.width - 104.0f,
                             briefingRect.y + briefingRect.height - 40.0f, 88.0f,
                             24.0f};
    bool continueHovered = CheckCollisionPointRec(mouse, continueBtn);
    Fumbo::Graphic2D::DrawRectangleRounded(continueBtn, 0.1f, 4, continueHovered ? Color{80, 120, 90, 255} : Color{50, 90, 60, 255});
    Fumbo::Graphic2D::DrawText("RETURN", {continueBtn.x + 24.0f, continueBtn.y + 6.0f}, font, 9, WHITE);

    if (continueHovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
      m_shiftBriefingVisible = false;
      m_desktop->Notify("Operations", "Shift " + std::to_string(m_shiftNumber) + " briefing complete.");
    }
  } else if (s_showEndShiftConfirm && canAdvance) {
    Fumbo::Graphic2D::DrawRectangleRec(area, {12, 14, 21, 220});
    Rectangle modalRect = {area.x + 70.0f, area.y + 70.0f, area.width - 140.0f,
                           area.height - 140.0f};
    Fumbo::Graphic2D::DrawRectangleRounded(modalRect, 0.04f, 4,
                                           {24, 29, 40, 255});
    Fumbo::Graphic2D::DrawRectangleRoundedLinesEx(modalRect, 0.04f, 4, 1.2f,
                                                  {255, 80, 80, 255});

    std::string confirmTitle = (m_currentTimeWindowIndex < 3) ? "ADVANCE TIME WINDOW" : "COMPLETE SHIFT";
    std::string confirmText = (m_currentTimeWindowIndex < 3)
                                  ? "Execute the queued actions and advance to the next operational window?"
                                  : "Complete the shift and review the summary?";
    Fumbo::Graphic2D::DrawText(confirmTitle,
                               {modalRect.x + 16.0f, modalRect.y + 16.0f}, font,
                               11, {255, 215, 0, 255});
    Fumbo::Graphic2D::DrawText(confirmText,
                               {modalRect.x + 16.0f, modalRect.y + 48.0f}, font,
                               10, {220, 220, 235, 255});

    Rectangle confirmBtn = {modalRect.x + 16.0f,
                            modalRect.y + modalRect.height - 44.0f, 92.0f,
                            28.0f};
    Rectangle cancelBtn = {modalRect.x + modalRect.width - 108.0f,
                           modalRect.y + modalRect.height - 44.0f, 92.0f,
                           28.0f};

    bool confirmHovered = CheckCollisionPointRec(mouse, confirmBtn);
    bool cancelHovered = CheckCollisionPointRec(mouse, cancelBtn);

    Fumbo::Graphic2D::DrawRectangleRounded(
        confirmBtn, 0.12f, 4,
        confirmHovered ? Color{40, 120, 80, 255} : Color{28, 92, 60, 255});
    Fumbo::Graphic2D::DrawRectangleRounded(
        cancelBtn, 0.12f, 4,
        cancelHovered ? Color{80, 80, 95, 255} : Color{50, 55, 70, 255});
    Fumbo::Graphic2D::DrawText(
        "CONFIRM", {confirmBtn.x + 20.0f, confirmBtn.y + 8.0f}, font, 9, WHITE);
    Fumbo::Graphic2D::DrawText(
        "CANCEL", {cancelBtn.x + 24.0f, cancelBtn.y + 8.0f}, font, 9, WHITE);

    if (confirmHovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
      // Create decision log entries from the current selected actions
      Game::GameState &gsConfirm = m_gameManager.GetGameState();
      for (int t = 0; t < 3; t++) {
        for (int a = 0; a < 5; a++) {
          Game::ActionID aid = getActionId(t, a);
          if (std::find(gsConfirm.selectedActions.begin(), gsConfirm.selectedActions.end(), aid) == gsConfirm.selectedActions.end())
            continue;
          std::string disaster = (t == 0) ? "Flood" : (t == 1 ? "Wildfire" : "Volcano");
          std::string sector = (t == 0) ? "Sector A" : (t == 1 ? "Sector C" : "Sector B");
          AddDecisionLogEntry(actions[t][a].name, disaster, sector, "Completed",
                             (std::string(actions[t][a].budgetImpact) == "---" || std::string(actions[t][a].budgetImpact) == "--" || std::string(actions[t][a].budgetImpact) == "-" ? "$5,000" : "$10,000"),
                             "Action executed during the shift");
        }
      }

      // Execute the simulation turn using the backend Simulation system.
      // Simulation will process actions, evaluate outcomes, generate comms,
      // clear selected actions, and advance the GameManager window.
      {
        Game::Simulation sim;
        bool shiftContinues = sim.ExecuteTurn(m_gameManager);

        // Sync UI-level values from backend GameState/GameManager
        Game::GameState &gs = m_gameManager.GetGameState();
        m_currentTimeWindowIndex = m_gameManager.GetCurrentWindow();
        m_currentTimeLabel = std::string(Game::GameState::GetWindowDisplayTime(m_currentTimeWindowIndex));
        m_budget = gs.budget;
        m_publicTrust = gs.publicTrust;
        m_shiftNumber = m_gameManager.GetCurrentShift();

        if (!shiftContinues) {
          // Shift ended: record a ShiftSummary in GameState.decisionLog and show summary
          Game::GameState &gsFinal = m_gameManager.GetGameState();
          Game::GameState::ShiftSummary summary;
          summary.shiftNumber = gsFinal.currentShift;
          summary.peopleSaved = gsFinal.peopleSaved;
          summary.casualties = gsFinal.casualties;
          summary.budgetSpent = gsFinal.shiftStartingBudget - gsFinal.budget;
          summary.publicTrust = gsFinal.publicTrust;
          // Build actionsTaken from the entries we added to the UI decision log earlier
          for (const auto &entry : m_decisionLog) {
            summary.actionsTaken.push_back(entry.title);
          }
          // Compute a simple overall rating from public trust (normalized)
          summary.overallRating = std::clamp(gsFinal.publicTrust / 20.0f, 0.0f, 5.0f);
          gsFinal.decisionLog.push_back(summary);
          m_shiftSummaryVisible = true;
        } else {
          // Shift continues: keep UI in next window. Keep legacy notifications.
          if (m_currentTimeWindowIndex == 2) {
            m_desktop->Notify("Threat Center",
                              "Critical update: conditions are worsening across the city.");
          } else if (m_currentTimeWindowIndex == 3) {
            m_desktop->Notify("Threat Center",
                              "Emergency update: evacuation zones are now active.");
          } else {
            m_desktop->Notify("Operations",
                              "Advance complete. Current time is " + GetCurrentTimeLabel() + ".");
          }
        }
      }

      s_showEndShiftConfirm = false;
    } else if (cancelHovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
      s_showEndShiftConfirm = false;
    }
  } else if (endHovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && canAdvance) {
    s_showEndShiftConfirm = true;
  }
}

void DemoDesktop::DrawStatusBar() {
  float screenWidth = (float)GetScreenWidth();
  float barHeight = 30.0f;

  // Background
  Fumbo::Graphic2D::DrawRectangleRec({0.0f, 0.0f, screenWidth, barHeight},
                                     {192, 192, 192, 255});
  // Bottom Border
  Fumbo::Graphic2D::DrawLineEx({0.0f, barHeight - 1.0f}, {screenWidth, barHeight - 1.0f},
                               1.0f, {128, 128, 128, 255});

  Font font = GetFontDefault();
  float textY = 8.0f;

  auto DrawInsetBox = [](Rectangle r) {
    Fumbo::Graphic2D::DrawRectangleRec(r, {192, 192, 192, 255});
    Fumbo::Graphic2D::DrawLineEx({r.x, r.y}, {r.x + r.width, r.y}, 1.0f, {128, 128, 128, 255});
    Fumbo::Graphic2D::DrawLineEx({r.x, r.y}, {r.x, r.y + r.height}, 1.0f, {128, 128, 128, 255});
    Fumbo::Graphic2D::DrawLineEx({r.x + r.width - 1.0f, r.y}, {r.x + r.width - 1.0f, r.y + r.height}, 1.0f, WHITE);
    Fumbo::Graphic2D::DrawLineEx({r.x, r.y + r.height - 1.0f}, {r.x + r.width, r.y + r.height - 1.0f}, 1.0f, WHITE);
  };

  // Helper mapping from tab/action index -> Game::ActionID
  auto getActionId = [](int tabIdx, int actionIdx) -> Game::ActionID {
    using Game::ActionID;
    if (tabIdx == 0) {
      switch (actionIdx) {
      case 0: return ActionID::IssueWarning;
      case 1: return ActionID::DeployRescue;
      case 2: return ActionID::OpenShelter;
      case 3: return ActionID::CloseRoad;
      case 4: return ActionID::EvacuateResidents;
      }
    } else if (tabIdx == 1) {
      switch (actionIdx) {
      case 0: return ActionID::IssueWarning;
      case 1: return ActionID::DeployRescue;
      case 2: return ActionID::RequestAirSupport;
      case 3: return ActionID::EvacuateResidents;
      case 4: return ActionID::CloseRoad;
      }
    } else {
      switch (actionIdx) {
      case 0: return ActionID::IssueWarning;
      case 1: return ActionID::EvacuateResidents;
      case 2: return ActionID::CloseRoad;
      case 3: return ActionID::OpenShelter;
      case 4: return ActionID::DeployRescue;
      }
    }
    return ActionID::IssueWarning;
  };

  const Game::GameState &gs = m_gameManager.GetGameState();

  // 1. Day Counter
  DrawInsetBox({10.0f, 4.0f, 150.0f, 22.0f});
  std::string dayText = "DAY: " + std::to_string(gs.currentShift);
  Fumbo::Graphic2D::DrawText(dayText, {20.0f, textY}, font, 12, BLACK);

  // 2. City Status
  DrawInsetBox({180.0f, 4.0f, 380.0f, 22.0f});
  Fumbo::Graphic2D::DrawText("CITY STATUS: ", {190.0f, textY}, font, 12, BLACK);
  Color statusColor = {0, 128, 0, 255}; // Green
  if (m_cityStatus == "ALERT")
    statusColor = {215, 100, 0, 255}; // Orange
  else if (m_cityStatus == "CRITICAL" || m_cityStatus == "EVACUATE")
    statusColor = {180, 0, 0, 255}; // Red
  Fumbo::Graphic2D::DrawText(m_cityStatus, {290.0f, textY}, font, 12, statusColor);

  // 3. Budget
  DrawInsetBox({580.0f, 4.0f, 320.0f, 22.0f});
  Fumbo::Graphic2D::DrawText("BUDGET: ", {590.0f, textY}, font, 12, BLACK);
  std::string budgetValText = "$" + std::to_string(gs.budget);
  Fumbo::Graphic2D::DrawText(budgetValText, {660.0f, textY}, font, 12, {0, 128, 0, 255});

  // 4. Public Trust
  DrawInsetBox({920.0f, 4.0f, 320.0f, 22.0f});
  Fumbo::Graphic2D::DrawText("PUBLIC TRUST: ", {930.0f, textY}, font, 12, BLACK);
  std::string trustText = std::to_string(gs.publicTrust) + "%";
  Color trustColor = {0, 128, 0, 255};
  if (gs.publicTrust < 35)
    trustColor = {180, 0, 0, 255};
  else if (gs.publicTrust < 70)
    trustColor = {215, 100, 0, 255};
  Fumbo::Graphic2D::DrawText(trustText, {1040.0f, textY}, font, 12, trustColor);

  // 5. City Status Indicator (Overall Health)
  // Calculate overall city status from existing GameState values
  // Formula: Population Safety (40%) + Infrastructure (25%) + Public Trust (20%) + Emergency Capacity (15%)
  
  // Population Safety: Based on casualties vs people saved ratio
  float populationSafety = 100.0f;
  int totalAffected = gs.peopleSaved + gs.casualties;
  if (totalAffected > 0) {
    populationSafety = (static_cast<float>(gs.peopleSaved) / static_cast<float>(totalAffected)) * 100.0f;
  }
  
  // Infrastructure: Inverted damage percentage (100 - damage%)
  float infrastructureHealth = 100.0f - static_cast<float>(gs.infrastructureDamage);
  if (infrastructureHealth < 0.0f) infrastructureHealth = 0.0f;
  
  // Public Trust: Already 0-100 scale
  float publicTrustScore = static_cast<float>(gs.publicTrust);
  
  // Emergency Capacity: Based on remaining rescue teams and budget
  float rescueTeamCapacity = (static_cast<float>(gs.rescueTeams) / 3.0f) * 100.0f; // 3 is initial count
  float budgetCapacity = (static_cast<float>(gs.budget) / 50000.0f) * 100.0f; // 50000 is initial budget
  float emergencyCapacity = (rescueTeamCapacity + budgetCapacity) / 2.0f;
  if (emergencyCapacity > 100.0f) emergencyCapacity = 100.0f;
  
  // Calculate weighted overall score
  float overallCityStatus = (populationSafety * 0.40f) + 
                            (infrastructureHealth * 0.25f) + 
                            (publicTrustScore * 0.20f) + 
                            (emergencyCapacity * 0.15f);
  
  // Clamp to 0-100 range
  if (overallCityStatus > 100.0f) overallCityStatus = 100.0f;
  if (overallCityStatus < 0.0f) overallCityStatus = 0.0f;
  
  // Determine status label and color based on percentage
  std::string statusLabel;
  Color statusIndicatorColor;
  
  if (overallCityStatus >= 80.0f) {
    statusLabel = "STABLE";
    statusIndicatorColor = {0, 180, 0, 255}; // Bright green
  } else if (overallCityStatus >= 60.0f) {
    statusLabel = "CONCERN";
    statusIndicatorColor = {215, 170, 0, 255}; // Yellow-orange
  } else if (overallCityStatus >= 40.0f) {
    statusLabel = "CRITICAL";
    statusIndicatorColor = {255, 140, 0, 255}; // Orange
  } else if (overallCityStatus >= 20.0f) {
    statusLabel = "EMERGENCY";
    statusIndicatorColor = {255, 60, 0, 255}; // Red-orange
  } else {
    statusLabel = "COLLAPSE";
    statusIndicatorColor = {200, 0, 0, 255}; // Dark red
  }
  
  // Draw City Status Indicator box (positioned after Public Trust)
  float cityStatusX = 1260.0f;
  DrawInsetBox({cityStatusX, 4.0f, 340.0f, 22.0f});
  
  // Draw label
  Fumbo::Graphic2D::DrawText("CITY: ", {cityStatusX + 10.0f, textY}, font, 12, BLACK);
  
  // Draw percentage
  char percentBuf[8];
  snprintf(percentBuf, sizeof(percentBuf), "%d%%", static_cast<int>(overallCityStatus));
  Fumbo::Graphic2D::DrawText(percentBuf, {cityStatusX + 50.0f, textY}, font, 12, statusIndicatorColor);
  
  // Draw status label
  Fumbo::Graphic2D::DrawText(" | " + statusLabel, {cityStatusX + 90.0f, textY}, font, 12, statusIndicatorColor);
}
