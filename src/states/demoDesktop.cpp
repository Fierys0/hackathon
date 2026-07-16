#include "demoDesktop.hpp"
#include "../core/globals.hpp"
#include <ctime>
#include <string>

// IGameState interface: delegates to OSDesktop

void DemoDesktop::Init() {
  m_desktop = std::make_shared<OS::OSDesktop>();
  m_desktop->SetFont(OS::GlobalFont);

  // Initialize status bar game state
  m_day = 3;
  m_budget = 50000;
  m_publicTrust = 85;
  m_cityStatus = "ALERT";

  // Setup all OS components
  SetupDesktopIcons();
  SetupStartMenu();
  SetupContextMenu();
  SetupSystemTray();

  // Initialize the desktop
  m_desktop->Init();

  // Welcome notification
  m_desktop->Notify("Welcome", "Fumbo OS is ready. Double-click an icon to start.");
}

void DemoDesktop::Cleanup() {
  if (m_desktop) {
    m_desktop->Cleanup();
    m_desktop.reset();
  }
}

void DemoDesktop::Update() {
  if (m_desktop)
    m_desktop->Update();
}

void DemoDesktop::DrawClean() {
  if (m_desktop)
    m_desktop->DrawClean();
}

void DemoDesktop::DrawDirty() {
  if (m_desktop)
    m_desktop->DrawDirty();

  DrawStatusBar();
}

// Setup Helpers

void DemoDesktop::SetupDesktopIcons() {
  // Threat Center icon
  m_desktop->AddDesktopIcon("Threat Center", {0}, [this]() {
    m_desktop->OpenWindow("Threat Center", {100, 80, 750, 450},
                          DrawThreatCenterContent);
    m_desktop->Notify("Threat Center", "Threat Monitoring System activated.");
  });

  // Comms icon
  m_desktop->AddDesktopIcon("Comms", {0}, [this]() {
    m_desktop->OpenWindow("Comms", {100, 100, 500, 350},
                          DrawCommsContent);
    m_desktop->Notify("Comms & Media", "Comms Activated");
  });

  // Mitigation Hub icon
  m_desktop->AddDesktopIcon("Mitigation Hub", {0}, [this]() {
    m_desktop->OpenWindow("Mitigation Hub", {100, 100, 500, 350},
                          DrawMitigationHub);
    m_desktop->Notify("Mitigation Hub", "Don't let this disaster continue.");
  });

  // Analysis Center Icon
  m_desktop->AddDesktopIcon("Analysis Center", {0}, [this]() {
    m_desktop->OpenWindow("Analysis Center", {100, 100, 500, 350},
                          DrawAnalysisCenter);
    m_desktop->Notify("Analysis Center", "Analysis Center activated.");
  });

  // Terminal icon
  m_desktop->AddDesktopIcon("Terminal", {0}, [this]() {
    m_desktop->OpenWindow("Terminal", {200, 100, 500, 350},
                          DrawTerminalContent);
    m_desktop->Notify("Terminal", "Terminal window opened.");
  });

  // Settings icon
  m_desktop->AddDesktopIcon("Settings", {0}, [this]() {
    m_desktop->OpenWindow("Settings", {250, 120, 450, 320},
                          DrawSettingsContent);
  });

  // File Manager icon
  m_desktop->AddDesktopIcon("Files", {0}, [this]() {
    m_desktop->OpenWindow("File Manager", {180, 90, 520, 380},
                          DrawFileManagerContent);
  });

  // Notes icon
  m_desktop->AddDesktopIcon("Notes", {0}, [this]() {
    m_desktop->OpenWindow("Notes", {300, 140, 380, 280},
                          DrawNotesContent);
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
        title,
        {150.0f + count * 20.0f, 80.0f + count * 15.0f, 420.0f, 280.0f},
        [title](Rectangle area) {
          Fumbo::Graphic2D::DrawText(
              "Content of " + title, {area.x + 15.0f, area.y + 15.0f},
              GetFontDefault(), 14, {200, 200, 230, 255});
          Fumbo::Graphic2D::DrawText(
              "Drag the title bar to move",
              {area.x + 15.0f, area.y + 40.0f}, GetFontDefault(), 12,
              {150, 150, 180, 255});
          Fumbo::Graphic2D::DrawText(
              "Drag edges to resize", {area.x + 15.0f, area.y + 58.0f},
              GetFontDefault(), 12, {150, 150, 180, 255});
        });
  });

  ctxMenu.AddItem("Send Notification", [this]() {
    m_desktop->Notify("Hello!", "This is a test notification from the desktop.");
  });

  ctxMenu.AddSeparator();

  ctxMenu.AddItem("About Fumbo OS", [this]() {
    m_desktop->OpenWindow("About", {350, 200, 350, 200}, DrawAboutContent);
  });
}

void DemoDesktop::SetupSystemTray() {
  m_desktop->AddTrayItem(
      "clock",
      [](Rectangle area) {
        time_t now = time(nullptr);
        struct tm *t = localtime(&now);
        char buf[16];
        snprintf(buf, sizeof(buf), "%02d:%02d", t->tm_hour, t->tm_min);

        float textY = area.y + (area.height - 11) * 0.5f;
        Fumbo::Graphic2D::DrawText(buf, {area.x + 2.0f, textY},
                                   GetFontDefault(), 11,
                                   {200, 200, 230, 255});
      },
      nullptr, 45.0f);
}

// Window Content Drawers (static: reusable from anywhere)

void DemoDesktop::DrawTerminalContent(Rectangle area) {
  Fumbo::Graphic2D::DrawRectangleRec(area, {15, 15, 25, 255});

  float y = area.y + 10.0f;
  float x = area.x + 10.0f;
  Font font = GetFontDefault();

  Fumbo::Graphic2D::DrawText("user@fumbo-os:~$", {x, y}, font, 12,
                             {80, 220, 80, 255});
  Fumbo::Graphic2D::DrawText("echo \"Hello, World!\"", {x + 130.0f, y},
                             font, 12, {200, 200, 220, 255});
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
    Color bg =
        (i % 2 == 0) ? Color{40, 40, 60, 200} : Color{35, 35, 52, 200};
    Fumbo::Graphic2D::DrawRectangleRounded(itemRect, 0.1f, 4, bg);
    Fumbo::Graphic2D::DrawText(settings[i], {x + 12.0f, y + 7.0f}, font,
                               12, {200, 200, 230, 255});
    Fumbo::Graphic2D::DrawText(">", {x + area.width - 50.0f, y + 7.0f},
                               font, 12, {120, 120, 160, 255});
    y += 34.0f;
  }
}

void DemoDesktop::DrawFileManagerContent(Rectangle area) {
  float y = area.y + 10.0f;
  float x = area.x + 10.0f;
  Font font = GetFontDefault();

  // Path bar
  Rectangle pathBar = {x, y, area.width - 20.0f, 24.0f};
  Fumbo::Graphic2D::DrawRectangleRounded(pathBar, 0.15f, 4,
                                         {40, 40, 60, 220});
  Fumbo::Graphic2D::DrawText("/home/user/Documents", {x + 8.0f, y + 5.0f},
                             font, 11, {180, 180, 210, 255});
  y += 32.0f;

  // File list
  const char *files[] = {"Documents/", "Pictures/", "Music/",
                          "readme.txt",  "notes.md",  "config.json"};
  const char *sizes[] = {"--", "--", "--", "4.2 KB", "1.1 KB", "256 B"};

  for (int i = 0; i < 6; i++) {
    Rectangle itemRect = {x, y, area.width - 20.0f, 26.0f};
    Color bg =
        (i % 2 == 0) ? Color{38, 38, 56, 200} : Color{34, 34, 50, 200};
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
  Fumbo::Graphic2D::DrawRectangleRec(area, {25, 25, 38, 255});

  float y = area.y + 12.0f;
  float x = area.x + 12.0f;
  Font font = GetFontDefault();

  Fumbo::Graphic2D::DrawText("My Notes", {x, y}, font, 14, WHITE);
  y += 24.0f;

  Fumbo::Graphic2D::DrawLineEx({x, y}, {x + area.width - 24.0f, y}, 1.0f,
                               {60, 60, 90, 200});
  y += 8.0f;

  const char *lines[] = {"- Buy groceries",
                          "- Finish the OS simulation",
                          "- Submit hackathon project",
                          "- Learn more C++",
                          "- Sleep more :)"};
  for (int i = 0; i < 5; i++) {
    Fumbo::Graphic2D::DrawText(lines[i], {x, y}, font, 12,
                               {190, 190, 215, 255});
    y += 20.0f;
  }
}

void DemoDesktop::DrawAboutContent(Rectangle area) {
  Font font = GetFontDefault();
  Fumbo::Graphic2D::DrawText("Fumbo OS Simulation",
                             {area.x + 15.0f, area.y + 15.0f}, font, 16,
                             WHITE);
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
  Fumbo::Graphic2D::DrawText("REGIONAL MONITORING MAP", {leftX + 5.0f, leftY + 5.0f}, font, 13, {0, 230, 118, 255});

  // Draw simulated map box
  Rectangle mapBox = {leftX, leftY + 25.0f, leftW, leftH - 25.0f};
  Fumbo::Graphic2D::DrawRectangleRounded(mapBox, 0.05f, 4, {28, 33, 46, 255});
  Fumbo::Graphic2D::DrawRectangleRoundedLinesEx(mapBox, 0.05f, 4, 1.0f, {50, 60, 80, 255});

  // Draw map grid lines
  for (int gx = 1; gx < 5; gx++) {
    float gridX = mapBox.x + (mapBox.width / 5) * gx;
    Fumbo::Graphic2D::DrawLineEx({gridX, mapBox.y}, {gridX, mapBox.y + mapBox.height}, 1.0f, {38, 44, 60, 150});
  }
  for (int gy = 1; gy < 4; gy++) {
    float gridY = mapBox.y + (mapBox.height / 4) * gy;
    Fumbo::Graphic2D::DrawLineEx({mapBox.x, gridY}, {mapBox.x + mapBox.width, gridY}, 1.0f, {38, 44, 60, 150});
  }

  // Draw 3 sectors and threat monitoring sensors
  // Sector A (North): Flood Monitoring Zone
  Vector2 secACenter = {mapBox.x + mapBox.width * 0.5f, mapBox.y + mapBox.height * 0.28f};
  Fumbo::Graphic2D::DrawCircleV(secACenter, 7.0f, {41, 121, 255, 255}); // Blue for water
  Fumbo::Graphic2D::DrawText("SEC-A [RIVER]", {secACenter.x - 38.0f, secACenter.y - 18.0f}, font, 10, {150, 180, 220, 255});

  // Sector B (South-West): Seismic Monitoring Zone
  Vector2 secBCenter = {mapBox.x + mapBox.width * 0.25f, mapBox.y + mapBox.height * 0.72f};
  Fumbo::Graphic2D::DrawCircleV(secBCenter, 7.0f, {255, 145, 0, 255}); // Orange for seismic
  Fumbo::Graphic2D::DrawText("SEC-B [SEISMIC]", {secBCenter.x - 45.0f, secBCenter.y - 18.0f}, font, 10, {220, 180, 150, 255});

  // Sector C (South-East): Wildfire Danger Zone
  Vector2 secCCenter = {mapBox.x + mapBox.width * 0.75f, mapBox.y + mapBox.height * 0.68f};
  Fumbo::Graphic2D::DrawCircleV(secCCenter, 7.0f, {255, 23, 68, 255}); // Red for fire danger
  Fumbo::Graphic2D::DrawText("SEC-C [WILDFIRE]", {secCCenter.x - 48.0f, secCCenter.y - 18.0f}, font, 10, {255, 150, 150, 255});

  // 3. Draw Vertical Divider
  float dividerX = area.x + leftWidth;
  Fumbo::Graphic2D::DrawLineEx({dividerX, area.y + padding}, {dividerX, area.y + area.height - padding}, 1.0f, {60, 68, 90, 255});

  // Right Column Content: SOP Rules Reference
  float rightX = dividerX + (padding * 0.5f);
  float rightY = area.y + padding;
  float rightW = rightWidth - (padding * 1.5f);
  float rightH = area.height - (padding * 2.0f);

  // Draw header for SOP Reference Drawer
  Fumbo::Graphic2D::DrawText("MONITORING PROTOCOLS", {rightX + 5.0f, rightY + 5.0f}, font, 13, {255, 215, 0, 255});

  // Draw a content frame for SOP text
  Rectangle sopBox = {rightX, rightY + 25.0f, rightW, rightH - 25.0f};
  Fumbo::Graphic2D::DrawRectangleRounded(sopBox, 0.05f, 4, {23, 27, 38, 255});
  Fumbo::Graphic2D::DrawRectangleRoundedLinesEx(sopBox, 0.05f, 4, 1.0f, {50, 60, 80, 255});

  // Draw actual text guidelines inside SOP Drawer
  float textY = sopBox.y + 12.0f;
  float textX = sopBox.x + 12.0f;

  Fumbo::Graphic2D::DrawText("RULE-01: FLOODING LEVEL", {textX, textY}, font, 11, {140, 180, 255, 255});
  textY += 15.0f;
  Fumbo::Graphic2D::DrawText("- Trigger if River Depth > 4.5m", {textX + 8.0f, textY}, font, 10, {180, 180, 210, 255});
  textY += 12.0f;
  Fumbo::Graphic2D::DrawText("  OR Rainfall rate > 80mm/h", {textX + 8.0f, textY}, font, 10, {180, 180, 210, 255});
  textY += 22.0f;

  Fumbo::Graphic2D::DrawText("RULE-02: WILDFIRE DANGER", {textX, textY}, font, 11, {255, 140, 140, 255});
  textY += 15.0f;
  Fumbo::Graphic2D::DrawText("- Trigger if Temp > 38.0 C", {textX + 8.0f, textY}, font, 10, {180, 180, 210, 255});
  textY += 12.0f;
  Fumbo::Graphic2D::DrawText("  AND Air Humidity < 15%", {textX + 8.0f, textY}, font, 10, {180, 180, 210, 255});
  textY += 22.0f;

  Fumbo::Graphic2D::DrawText("RULE-03: SEISMIC ACTIVITY", {textX, textY}, font, 11, {255, 220, 140, 255});
  textY += 15.0f;
  Fumbo::Graphic2D::DrawText("- Alert if Tremors > 12 / hr", {textX + 8.0f, textY}, font, 10, {180, 180, 210, 255});
  textY += 12.0f;
  Fumbo::Graphic2D::DrawText("  OR gas density > 350ppm", {textX + 8.0f, textY}, font, 10, {180, 180, 210, 255});
  textY += 24.0f;

  // Visual divider
  Fumbo::Graphic2D::DrawLineEx({textX, textY}, {textX + rightW - 24.0f, textY}, 1.0f, {60, 68, 90, 255});
  textY += 10.0f;

  // Real-time sensor checklist snippet
  Fumbo::Graphic2D::DrawText("LIVE MONITOR STATUS:", {textX, textY}, font, 11, {0, 230, 118, 255});
  textY += 16.0f;
  Fumbo::Graphic2D::DrawText("SEC-A: 3.8m | 42mm/h (OK)", {textX + 8.0f, textY}, font, 10, {180, 235, 180, 255});
  textY += 14.0f;
  Fumbo::Graphic2D::DrawText("SEC-B: 7 tremors/h   (OK)", {textX + 8.0f, textY}, font, 10, {180, 235, 180, 255});
  textY += 14.0f;
  Fumbo::Graphic2D::DrawText("SEC-C: 39 C | Hum 12% (WARN)", {textX + 8.0f, textY}, font, 10, {255, 140, 140, 255});
}

void DemoDesktop::DrawAnalysisCenter(Rectangle area) {
  // Placeholder
}

void DemoDesktop::DrawCommsContent(Rectangle area) {
    Fumbo::Graphic2D::DrawRectangleRec(area, {20, 24, 33, 255});

    
}

void DemoDesktop::DrawMitigationHub(Rectangle area) {
    // Placeholder
}

void DemoDesktop::DrawStatusBar() {
  float screenWidth = (float)GetScreenWidth();
  float barHeight = 30.0f;

  // Background
  Fumbo::Graphic2D::DrawRectangleRec({0.0f, 0.0f, screenWidth, barHeight}, {25, 29, 38, 235});
  // Bottom Border
  Fumbo::Graphic2D::DrawLineEx({0.0f, barHeight}, {screenWidth, barHeight}, 1.2f, {55, 62, 78, 255});

  Font font = GetFontDefault();
  float textY = 8.0f;

  // 1. Day Counter
  std::string dayText = "DAY: " + std::to_string(m_day);
  Fumbo::Graphic2D::DrawText(dayText, {20.0f, textY}, font, 12, {200, 200, 230, 255});

  // 2. City Status
  Fumbo::Graphic2D::DrawText("CITY STATUS: ", {200.0f, textY}, font, 12, {150, 150, 170, 255});
  Color statusColor = {0, 230, 118, 255}; // Green
  if (m_cityStatus == "ALERT") statusColor = {255, 145, 0, 255}; // Orange
  else if (m_cityStatus == "CRITICAL" || m_cityStatus == "EVACUATE") statusColor = {255, 23, 68, 255}; // Red
  Fumbo::Graphic2D::DrawText(m_cityStatus, {290.0f, textY}, font, 12, statusColor);

  // 3. Budget
  std::string budgetText = "BUDGET: $" + std::to_string(m_budget);
  Fumbo::Graphic2D::DrawText(budgetText, {600.0f, textY}, font, 12, {140, 255, 140, 255});

  // 4. Public Trust
  Fumbo::Graphic2D::DrawText("PUBLIC TRUST: ", {950.0f, textY}, font, 12, {150, 150, 170, 255});
  std::string trustText = std::to_string(m_publicTrust) + "%";
  Color trustColor = {0, 230, 118, 255};
  if (m_publicTrust < 35) trustColor = {255, 23, 68, 255};
  else if (m_publicTrust < 70) trustColor = {255, 145, 0, 255};
  Fumbo::Graphic2D::DrawText(trustText, {1050.0f, textY}, font, 12, trustColor);
}
