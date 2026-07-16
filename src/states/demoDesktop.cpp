#include "demoDesktop.hpp"
#include "../core/globals.hpp"
#include <ctime>
#include <string>

// IGameState interface: delegates to OSDesktop

void DemoDesktop::Init() {
  m_desktop = std::make_shared<OS::OSDesktop>();
  m_desktop->SetFont(OS::GlobalFont);

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
}

// Setup Helpers

void DemoDesktop::SetupDesktopIcons() {
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
