#include "demoDesktop.hpp"
#include "../core/globals.hpp"
#include "../game/game_manager.hpp"
#include "../game/simulation.hpp"
#include "fumbo.hpp"
#include "titlescreen.hpp"
#include <algorithm>
#include <cstdio>
#include <ctime>
#include <string>

static int s_selectedCommsCard = -1;    // Tracks selected card in Comms UI
static int s_mitigationTab = 0;         // 0=Flood, 1=Wildfire, 2=Volcano
static bool s_queuedActions[3][5] = {}; // [tab][action] queued state
static int s_hoveredMitAction = -1;     // Currently hovered action index
static bool s_showEndShiftConfirm = false;
static bool s_campaignVictoryVisible = false;
// IGameState interface: delegates to OSDesktop

void DemoDesktop::Init() {
  m_desktop = std::make_shared<OS::OSDesktop>();
  m_desktop->SetFont(OS::GlobalFont);

  // Initialize status bar game state
  m_day = 1;
  m_budget = 50000;
  m_publicTrust = 50;
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
  m_notesIconTex = Fumbo::Assets::LoadTexture("assets/icons/notes.png");

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

  // Setup Field Radio chatbot app
  m_radioWindowId = -1;
  m_radioInputTextbox =
      Fumbo::UI::Textbox({0, 0, 100, 100}, OS::GlobalFont, 14);
  m_radioInputTextbox.SetMultiline(false);
  m_radioInputTextbox.SetBackgroundColor(WHITE);
  m_radioInputTextbox.SetTextColor(BLACK);
  m_radioInputTextbox.SetOutlineColor({128, 128, 128, 255}, {10, 36, 106, 255});
  m_radioInputTextbox.SetPadding({6.0f, 6.0f});
  m_radioInputTextbox.SetText("");
  m_radioPending = false;
  m_radioChatHistory.clear();
  m_radioChatHistory.push_back(
      {"RADIO", "Field Radio initialized. VHF channel open."});
  m_radioChatHistory.push_back(
      {"Hendro", "Hendro here. Sector A flood monitoring. Let me know if you "
                 "need telemetry reports or structural gate advice."});

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

  // Auto-open tutorial manual on first shift
  if (m_shiftNumber == 1) {
    OpenTutorialWindow();
  }

  // Load and play background music safely (guarded in AudioManager)
  auto &am = Fumbo::Engine::Instance().GetAudioManager();
  am.LoadAudio("backgroundmsc", "assets/music/backgroundmsc.mp3",
               Fumbo::Audio::AudioType::MUSIC);
  if (!am.IsMusicPlaying(0)) {
    am.PlayMusic("backgroundmsc", 0, true);
  }
}

std::string DemoDesktop::GetCurrentTimeLabel() const {
  return m_currentTimeLabel;
}

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
  m_gameManager.Update();

  if (m_shiftSummaryVisible || m_shiftBriefingVisible ||
      s_campaignVictoryVisible) {
    return;
  }

  if (m_desktop) {
    m_desktop->Update();

    // Update notes textbox if Notes window is open and visible
    auto *notesWin = m_desktop->GetWindowManager().GetWindow(m_notesWindowId);
    if (notesWin && notesWin->IsVisible()) {
      m_notesTextbox.SetFocused(notesWin->IsFocused());
      m_notesTextbox.Update();
    }

    // Update radio input textbox if Field Radio window is open and visible
    auto *radioWin = m_desktop->GetWindowManager().GetWindow(m_radioWindowId);
    if (radioWin && radioWin->IsVisible()) {
      m_radioInputTextbox.SetFocused(radioWin->IsFocused());
      m_radioInputTextbox.Update();
    }
  }

  if (m_radioPending) {
    auto status = m_radioFuture.wait_for(std::chrono::seconds(0));
    if (status == std::future_status::ready) {
      m_radioPending = false;
      Game::AIChatResult res = m_radioFuture.get();

      std::string contact = "Hendro";
      if (m_shiftNumber == 2 || m_shiftNumber == 5)
        contact = "Budi";
      else if (m_shiftNumber == 3 || m_shiftNumber == 6 || m_shiftNumber == 7)
        contact = "Dr. Surono";

      if (res.success) {
        m_radioChatHistory.push_back({contact, res.message});
      } else {
        // Fallback offline responses if API key or connection failed
        std::string fallback =
            "VHF static... Signal weak. Standby for operational instructions.";
        if (contact == "Hendro") {
          fallback = "Hendro here. We are monitoring overflow rates. Focus on "
                     "warning evacuation zones.";
        } else if (contact == "Budi") {
          fallback = "Budi here. Evacuating villages in path of winds. "
                     "Maintain staging lines!";
        } else {
          fallback = "Dr. Surono. Seismic tremors accelerating. Evacuation "
                     "routes must remain clear.";
        }
        m_radioChatHistory.push_back({contact, fallback});
      }
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

  Font font = OS::GlobalFont;
  Vector2 scale = Fumbo::Utils::GetUIScale();
  Vector2 offset = Fumbo::Utils::GetUIOffset();
  Vector2 rawMouse = GetMousePosition();
  Vector2 mouse = {(rawMouse.x - offset.x) / scale.x,
                   (rawMouse.y - offset.y) / scale.y};

  // Dynamic guide highlight overlays for the first shift tutorial
  if (m_shiftNumber == 1 && !m_shiftSummaryVisible && !m_shiftBriefingVisible &&
      !s_campaignVictoryVisible) {
    float pulse = (sinf(GetTime() * 6.0f) + 1.0f) * 0.5f;
    Color glowColor = {255, 215, 0, (unsigned char)(100 + pulse * 155)};

    bool threatOpen = false;
    bool commsOpen = false;
    bool mitigationOpen = false;
    bool tutorialOpen = false;

    auto openIds = m_desktop->GetWindowManager().GetOpenWindowIds();
    for (int id : openIds) {
      auto *win = m_desktop->GetWindowManager().GetWindow(id);
      if (win && win->IsVisible()) {
        if (win->GetTitle() == "Threat Center")
          threatOpen = true;
        if (win->GetTitle() == "Comms")
          commsOpen = true;
        if (win->GetTitle() == "Mitigation Hub")
          mitigationOpen = true;
        if (win->GetTitle() == "How to Play - Sentinel")
          tutorialOpen = true;
      }
    }

    const auto &icons = m_desktop->GetDesktopIcons();
    for (const auto &icon : icons) {
      bool highlight = false;
      if (icon.label == "How to Play" && !tutorialOpen && !threatOpen &&
          !commsOpen && !mitigationOpen) {
        highlight = true;
      } else if (icon.label == "Threat Center" && !threatOpen && !commsOpen &&
                 !mitigationOpen) {
        highlight = true;
      } else if (icon.label == "Comms" && threatOpen && !commsOpen &&
                 !mitigationOpen) {
        highlight = true;
      } else if (icon.label == "Mitigation Hub" && (threatOpen || commsOpen) &&
                 !mitigationOpen) {
        highlight = true;
      }

      if (highlight) {
        Rectangle r = {icon.position.x - 6.0f, icon.position.y - 6.0f, 60.0f,
                       75.0f};
        Fumbo::Graphic2D::DrawRectangleRoundedLinesEx(r, 0.1f, 4, 2.0f,
                                                      glowColor);
      }
    }
  }

  if (m_shiftSummaryVisible) {
    // Cover the entire screen with a classic Windows 95 teal background
    Fumbo::Graphic2D::DrawRectangleRec({0.0f, 0.0f, 1280.0f, 720.0f},
                                       {0, 128, 128, 255});

    // Centered classic 3D dialog box
    Rectangle summaryRect = {340.0f, 110.0f, 600.0f, 500.0f};
    Fumbo::Graphic2D::DrawRectangleRec(summaryRect, {192, 192, 192, 255});

    // Windows 95 3D raised border lines
    Fumbo::Graphic2D::DrawLineEx(
        {summaryRect.x, summaryRect.y},
        {summaryRect.x + summaryRect.width, summaryRect.y}, 1.5f, WHITE);
    Fumbo::Graphic2D::DrawLineEx(
        {summaryRect.x, summaryRect.y},
        {summaryRect.x, summaryRect.y + summaryRect.height}, 1.5f, WHITE);
    Fumbo::Graphic2D::DrawLineEx(
        {summaryRect.x + summaryRect.width - 1.5f, summaryRect.y},
        {summaryRect.x + summaryRect.width - 1.5f,
         summaryRect.y + summaryRect.height},
        1.5f, {128, 128, 128, 255});
    Fumbo::Graphic2D::DrawLineEx(
        {summaryRect.x, summaryRect.y + summaryRect.height - 1.5f},
        {summaryRect.x + summaryRect.width,
         summaryRect.y + summaryRect.height - 1.5f},
        1.5f, {128, 128, 128, 255});

    // Dialog Header bar (Blue)
    Rectangle headerRect = {summaryRect.x + 4.0f, summaryRect.y + 4.0f,
                            summaryRect.width - 8.0f, 22.0f};
    Fumbo::Graphic2D::DrawRectangleRec(headerRect, {0, 0, 128, 255});
    Fumbo::Graphic2D::DrawText("SHIFT SUMMARY REPORT",
                               {headerRect.x + 8.0f, headerRect.y + 5.0f}, font,
                               11, WHITE);

    // Render Stats
    Game::GameState &gsFinal = m_gameManager.GetGameState();
    std::string budgetRemainingStr = "$" + std::to_string(gsFinal.budget);
    std::string publicTrustStr = std::to_string(gsFinal.publicTrust) + "%";

    float ratingVal = gsFinal.publicTrust / 20.0f;
    char ratingBuf[32];
    std::snprintf(ratingBuf, sizeof(ratingBuf), "%.1f/5", ratingVal);
    std::string ratingStr(ratingBuf);

    std::string stats[][2] = {
        {"Lives Saved", std::to_string(gsFinal.peopleSaved)},
        {"Casualties", std::to_string(gsFinal.casualties)},
        {"Infrastructure Damage",
         std::to_string(gsFinal.infrastructureDamage) + "%"},
        {"Budget Remaining", budgetRemainingStr},
        {"Public Trust", publicTrustStr},
        {"Overall Rating", ratingStr}};

    float statY = summaryRect.y + 50.0f;
    for (int i = 0; i < 6; i++) {
      Fumbo::Graphic2D::DrawText(stats[i][0],
                                 {summaryRect.x + 30.0f, statY + i * 28.0f},
                                 font, 12, BLACK);
      Fumbo::Graphic2D::DrawText(
          stats[i][1],
          {summaryRect.x + summaryRect.width - 150.0f, statY + i * 28.0f}, font,
          12, {0, 100, 0, 255});
    }

    std::string line1Text = "Operations completed successfully for the day.";
    std::string line2Text = "The city is safe. Good job, officer.";
    if (gsFinal.casualties > 0) {
      line1Text =
          std::to_string(gsFinal.peopleSaved) + " lives were saved, but " +
          std::to_string(gsFinal.casualties) + " citizens were lost today.";
      line2Text =
          "The city will remember who acted fast, and who hesitated too long.";
    }

    Fumbo::Graphic2D::DrawText(
        line1Text,
        {summaryRect.x + 30.0f, summaryRect.y + summaryRect.height - 110.0f},
        font, 10, {100, 0, 0, 255});
    Fumbo::Graphic2D::DrawText(
        line2Text,
        {summaryRect.x + 30.0f, summaryRect.y + summaryRect.height - 90.0f},
        font, 10, {200, 0, 0, 255});

    // Continue Button (3D raised)
    Rectangle closeBtn = {summaryRect.x + summaryRect.width - 140.0f,
                          summaryRect.y + summaryRect.height - 45.0f, 110.0f,
                          30.0f};
    bool closeHovered = CheckCollisionPointRec(mouse, closeBtn);

    Fumbo::Graphic2D::DrawRectangleRec(closeBtn, {192, 192, 192, 255});
    Fumbo::Graphic2D::DrawLineEx({closeBtn.x, closeBtn.y},
                                 {closeBtn.x + closeBtn.width, closeBtn.y},
                                 1.5f, WHITE);
    Fumbo::Graphic2D::DrawLineEx({closeBtn.x, closeBtn.y},
                                 {closeBtn.x, closeBtn.y + closeBtn.height},
                                 1.5f, WHITE);
    Fumbo::Graphic2D::DrawLineEx(
        {closeBtn.x + closeBtn.width - 1.5f, closeBtn.y},
        {closeBtn.x + closeBtn.width - 1.5f, closeBtn.y + closeBtn.height},
        1.5f, BLACK);
    Fumbo::Graphic2D::DrawLineEx(
        {closeBtn.x, closeBtn.y + closeBtn.height - 1.5f},
        {closeBtn.x + closeBtn.width, closeBtn.y + closeBtn.height - 1.5f},
        1.5f, BLACK);

    Fumbo::Graphic2D::DrawText(
        "CONTINUE", {closeBtn.x + 28.0f, closeBtn.y + 10.0f}, font, 10, BLACK);

    if (closeHovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
      Fumbo::Engine::Instance().GetAudioManager().PlaySound("click");
      m_shiftSummaryVisible = false;

      if (m_shiftNumber >= 7) {
        s_campaignVictoryVisible = true;
      } else {
        m_shiftBriefingVisible = true;
        m_shiftNumber++;
        m_currentTimeWindowIndex = 0;
        m_currentTimeLabel = "08:00";
        m_cityStatus = "ALERT";

        auto &gs = m_gameManager.GetGameState();
        gs.currentShift = m_shiftNumber;
        gs.ResetForNewShift();
        m_gameManager.TriggerAIUpdate();

        m_radioChatHistory.clear();
        m_radioChatHistory.push_back(
            {"RADIO",
             "VHF Channel re-initialized. Establishing connection..."});
        if (m_shiftNumber == 1 || m_shiftNumber == 4) {
          m_radioChatHistory.push_back(
              {"Hendro",
               "Hendro here. Sector A flood monitoring. Let me know if you "
               "need telemetry reports or structural gate advice."});
        } else if (m_shiftNumber == 2 || m_shiftNumber == 5) {
          m_radioChatHistory.push_back(
              {"Budi",
               "Captain Budi, Sector C fire command. High dry winds reported. "
               "Send user queries for fire containment strategy."});
        } else {
          m_radioChatHistory.push_back(
              {"Dr. Surono",
               "Dr. Surono here. Monitoring Mt. Fumbo volcanic gas emissions "
               "and tremor frequency. Ready for hazard query."});
        }
        m_radioPending = false;
        m_radioInputTextbox.SetText("");

        for (int t = 0; t < 3; t++)
          for (int a = 0; a < 5; a++)
            s_queuedActions[t][a] = false;
      }
    }
  } else if (m_shiftBriefingVisible) {
    // Full screen briefing cover
    Fumbo::Graphic2D::DrawRectangleRec({0.0f, 0.0f, 1280.0f, 720.0f},
                                       {0, 128, 128, 255});

    Rectangle briefingRect = {340.0f, 110.0f, 600.0f, 500.0f};
    Fumbo::Graphic2D::DrawRectangleRec(briefingRect, {192, 192, 192, 255});

    Fumbo::Graphic2D::DrawLineEx(
        {briefingRect.x, briefingRect.y},
        {briefingRect.x + briefingRect.width, briefingRect.y}, 1.5f, WHITE);
    Fumbo::Graphic2D::DrawLineEx(
        {briefingRect.x, briefingRect.y},
        {briefingRect.x, briefingRect.y + briefingRect.height}, 1.5f, WHITE);
    Fumbo::Graphic2D::DrawLineEx(
        {briefingRect.x + briefingRect.width - 1.5f, briefingRect.y},
        {briefingRect.x + briefingRect.width - 1.5f,
         briefingRect.y + briefingRect.height},
        1.5f, {128, 128, 128, 255});
    Fumbo::Graphic2D::DrawLineEx(
        {briefingRect.x, briefingRect.y + briefingRect.height - 1.5f},
        {briefingRect.x + briefingRect.width,
         briefingRect.y + briefingRect.height - 1.5f},
        1.5f, {128, 128, 128, 255});

    Rectangle headerRect = {briefingRect.x + 4.0f, briefingRect.y + 4.0f,
                            briefingRect.width - 8.0f, 22.0f};
    Fumbo::Graphic2D::DrawRectangleRec(headerRect, {0, 0, 128, 255});
    Fumbo::Graphic2D::DrawText("OPERATIONAL BRIEFING",
                               {headerRect.x + 8.0f, headerRect.y + 5.0f}, font,
                               11, WHITE);

    Fumbo::Graphic2D::DrawText("Next shift readiness directions:",
                               {briefingRect.x + 30.0f, briefingRect.y + 50.0f},
                               font, 11, BLACK);

    std::string b1 = "- Recheck sensor calibration before dispatch.";
    std::string b2 = "- Prepare shelter and communications routes.";
    std::string b3 = "- Maintain public trust with clear updates.";
    if (m_shiftNumber == 1 || m_shiftNumber == 4) {
      b1 = "- Monitor river levels and highlands precipitation.";
      b2 = "- Prepare sandbag barricades along Sector A.";
      b3 = "- Issue early warning alerts to panic prevention.";
    } else if (m_shiftNumber == 2 || m_shiftNumber == 5) {
      b1 = "- Set up smoke advisory notifications for Sector C.";
      b2 = "- Prepare fire containment staging zones.";
      b3 = "- Monitor dry wind gusts and temperature patterns.";
    } else {
      b1 = "- Prepare emergency shelters in Sector B.";
      b2 = "- Coordinate with aviation authorities for airport closure.";
      b3 = "- Monitor gas emissions (SO2/CO2) and tremor frequency.";
    }

    Fumbo::Graphic2D::DrawText(
        b1, {briefingRect.x + 30.0f, briefingRect.y + 100.0f}, font, 11, BLACK);
    Fumbo::Graphic2D::DrawText(
        b2, {briefingRect.x + 30.0f, briefingRect.y + 140.0f}, font, 11, BLACK);
    Fumbo::Graphic2D::DrawText(
        b3, {briefingRect.x + 30.0f, briefingRect.y + 180.0f}, font, 11, BLACK);

    Rectangle continueBtn = {briefingRect.x + briefingRect.width - 140.0f,
                             briefingRect.y + briefingRect.height - 45.0f,
                             110.0f, 30.0f};
    bool continueHovered = CheckCollisionPointRec(mouse, continueBtn);
    Fumbo::Graphic2D::DrawRectangleRec(continueBtn, {192, 192, 192, 255});
    Fumbo::Graphic2D::DrawLineEx(
        {continueBtn.x, continueBtn.y},
        {continueBtn.x + continueBtn.width, continueBtn.y}, 1.5f, WHITE);
    Fumbo::Graphic2D::DrawLineEx(
        {continueBtn.x, continueBtn.y},
        {continueBtn.x, continueBtn.y + continueBtn.height}, 1.5f, WHITE);
    Fumbo::Graphic2D::DrawLineEx(
        {continueBtn.x + continueBtn.width - 1.5f, continueBtn.y},
        {continueBtn.x + continueBtn.width - 1.5f,
         continueBtn.y + continueBtn.height},
        1.5f, BLACK);
    Fumbo::Graphic2D::DrawLineEx(
        {continueBtn.x, continueBtn.y + continueBtn.height - 1.5f},
        {continueBtn.x + continueBtn.width,
         continueBtn.y + continueBtn.height - 1.5f},
        1.5f, BLACK);

    Fumbo::Graphic2D::DrawText("START SHIFT",
                               {continueBtn.x + 24.0f, continueBtn.y + 10.0f},
                               font, 10, BLACK);

    if (continueHovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
      Fumbo::Engine::Instance().GetAudioManager().PlaySound("click");
      m_shiftBriefingVisible = false;
      m_desktop->Notify("Operations", "Shift " + std::to_string(m_shiftNumber) +
                                          " briefing complete.");
    }
  } else if (s_campaignVictoryVisible) {
    Fumbo::Graphic2D::DrawRectangleRec({0.0f, 0.0f, 1280.0f, 720.0f},
                                       {0, 128, 128, 255});

    Rectangle vicRect = {340.0f, 110.0f, 600.0f, 500.0f};
    Fumbo::Graphic2D::DrawRectangleRec(vicRect, {192, 192, 192, 255});

    Fumbo::Graphic2D::DrawLineEx({vicRect.x, vicRect.y},
                                 {vicRect.x + vicRect.width, vicRect.y}, 1.5f,
                                 WHITE);
    Fumbo::Graphic2D::DrawLineEx({vicRect.x, vicRect.y},
                                 {vicRect.x, vicRect.y + vicRect.height}, 1.5f,
                                 WHITE);
    Fumbo::Graphic2D::DrawLineEx(
        {vicRect.x + vicRect.width - 1.5f, vicRect.y},
        {vicRect.x + vicRect.width - 1.5f, vicRect.y + vicRect.height}, 1.5f,
        {128, 128, 128, 255});
    Fumbo::Graphic2D::DrawLineEx(
        {vicRect.x, vicRect.y + vicRect.height - 1.5f},
        {vicRect.x + vicRect.width, vicRect.y + vicRect.height - 1.5f}, 1.5f,
        {128, 128, 128, 255});

    Rectangle headerRect = {vicRect.x + 4.0f, vicRect.y + 4.0f,
                            vicRect.width - 8.0f, 22.0f};
    Fumbo::Graphic2D::DrawRectangleRec(headerRect, {0, 0, 128, 255});
    Fumbo::Graphic2D::DrawText("CAMPAIGN SUCCESS",
                               {headerRect.x + 8.0f, headerRect.y + 5.0f}, font,
                               11, WHITE);

    Fumbo::Graphic2D::DrawText(
        "Congratulations Officer! Your 7-day tenure at the NDMC is complete:",
        {vicRect.x + 30.0f, vicRect.y + 45.0f}, font, 11, BLACK);
    Fumbo::Graphic2D::DrawText("- Shift 1: Northern River Flood - SECURED",
                               {vicRect.x + 45.0f, vicRect.y + 80.0f}, font, 10,
                               {0, 100, 0, 255});
    Fumbo::Graphic2D::DrawText("- Shift 2: Sector C Wildfire - CONTAINED",
                               {vicRect.x + 45.0f, vicRect.y + 102.0f}, font,
                               10, {0, 100, 0, 255});
    Fumbo::Graphic2D::DrawText(
        "- Shift 3: Mt. Fumbo Volcano Eruption - EVACUATED",
        {vicRect.x + 45.0f, vicRect.y + 124.0f}, font, 10, {0, 100, 0, 255});
    Fumbo::Graphic2D::DrawText("- Shift 4: Flash Flood Surge - RESOLVED",
                               {vicRect.x + 45.0f, vicRect.y + 146.0f}, font,
                               10, {0, 100, 0, 255});
    Fumbo::Graphic2D::DrawText(
        "- Shift 5: Highlands Forest Fire - EXTINGUISHED",
        {vicRect.x + 45.0f, vicRect.y + 168.0f}, font, 10, {0, 100, 0, 255});
    Fumbo::Graphic2D::DrawText("- Shift 6: Volcanic Tremor Crisis - MANAGED",
                               {vicRect.x + 45.0f, vicRect.y + 190.0f}, font,
                               10, {0, 100, 0, 255});
    Fumbo::Graphic2D::DrawText(
        "- Shift 7: Mega-Hazard Cascading Collapse - MITIGATED",
        {vicRect.x + 45.0f, vicRect.y + 212.0f}, font, 10, {0, 100, 0, 255});

    Fumbo::Graphic2D::DrawText(
        "NDMC Executive Board Evaluation: EXCELLENT PERFORMANCE.",
        {vicRect.x + 30.0f, vicRect.y + 250.0f}, font, 11, BLACK);
    Fumbo::Graphic2D::DrawText(
        "Your critical thinking under pressure saved countless lives.",
        {vicRect.x + 30.0f, vicRect.y + 280.0f}, font, 11, BLACK);
    Fumbo::Graphic2D::DrawText(
        "Early warning telemetry works only when humans are brave enough",
        {vicRect.x + 30.0f, vicRect.y + 310.0f}, font, 11, BLACK);
    Fumbo::Graphic2D::DrawText(
        "to analyze facts, assess risk, and act before it is too late.",
        {vicRect.x + 30.0f, vicRect.y + 330.0f}, font, 11, BLACK);

    Rectangle mainBtn = {vicRect.x + vicRect.width - 150.0f,
                         vicRect.y + vicRect.height - 45.0f, 130.0f, 30.0f};
    bool btnHovered = CheckCollisionPointRec(mouse, mainBtn);
    Fumbo::Graphic2D::DrawRectangleRec(mainBtn, {192, 192, 192, 255});
    Fumbo::Graphic2D::DrawLineEx({mainBtn.x, mainBtn.y},
                                 {mainBtn.x + mainBtn.width, mainBtn.y}, 1.5f,
                                 WHITE);
    Fumbo::Graphic2D::DrawLineEx({mainBtn.x, mainBtn.y},
                                 {mainBtn.x, mainBtn.y + mainBtn.height}, 1.5f,
                                 WHITE);
    Fumbo::Graphic2D::DrawLineEx(
        {mainBtn.x + mainBtn.width - 1.5f, mainBtn.y},
        {mainBtn.x + mainBtn.width - 1.5f, mainBtn.y + mainBtn.height}, 1.5f,
        BLACK);
    Fumbo::Graphic2D::DrawLineEx(
        {mainBtn.x, mainBtn.y + mainBtn.height - 1.5f},
        {mainBtn.x + mainBtn.width, mainBtn.y + mainBtn.height - 1.5f}, 1.5f,
        BLACK);

    Fumbo::Graphic2D::DrawText("RETURN TO MENU",
                               {mainBtn.x + 16.0f, mainBtn.y + 10.0f}, font, 9,
                               BLACK);

    if (btnHovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
      Fumbo::Engine::Instance().GetAudioManager().PlaySound("click");
      s_campaignVictoryVisible = false;
      Fumbo::Instance().ChangeState(std::make_shared<TitleScreen>());
    }
  }
}

// Setup Helpers

void DemoDesktop::SetupDesktopIcons() {
  // How to Play icon
  m_desktop->AddDesktopIcon("How to Play", m_bookmarkIconTex, m_bgIconTex,
                            [this]() { OpenTutorialWindow(); });

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
        m_desktop->Notify("Mitigation Hub", "Operational window active at " +
                                                GetCurrentTimeLabel() + ".");
      });

  // Decision Log icon
  m_desktop->AddDesktopIcon(
      "Decision Log", m_bookmarkIconTex, m_bgIconTex, [this]() {
        m_desktop->OpenWindow(
            "Decision Log", {100, 100, 500, 350},
            [this](Rectangle area) { DrawDecisionLog(area); },
            m_bookmarkIconTex);
        m_desktop->Notify("Decision Log", "Decision Log opened at " +
                                              GetCurrentTimeLabel() + ".");
      });

  // Notes icon
  m_desktop->AddDesktopIcon("Notes", m_notesIconTex, m_bgIconTex, [this]() {
    m_notesWindowId = m_desktop->OpenWindow(
        "Notes", {300, 140, 380, 280},
        [this](Rectangle area) { DrawNotesContent(area); }, m_notesIconTex);
  });

  // Field Radio icon
  m_desktop->AddDesktopIcon(
      "Field Radio", m_commsIconTex, m_bgIconTex, [this]() {
        m_radioWindowId = m_desktop->OpenWindow(
            "Field Radio", {200, 80, 520, 480},
            [this](Rectangle area) { DrawRadioContent(area); }, m_commsIconTex);
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
          Fumbo::Graphic2D::DrawText("Content of " + title,
                                     {area.x + 15.0f, area.y + 15.0f},
                                     OS::GlobalFont, 14, {200, 200, 230, 255});
          Fumbo::Graphic2D::DrawText("Drag the title bar to move",
                                     {area.x + 15.0f, area.y + 40.0f},
                                     OS::GlobalFont, 12, {150, 150, 180, 255});
          Fumbo::Graphic2D::DrawText("Drag edges to resize",
                                     {area.x + 15.0f, area.y + 58.0f},
                                     OS::GlobalFont, 12, {150, 150, 180, 255});
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
        Font font = OS::GlobalFont;

        Vector2 timeSize =
            MeasureTextEx(font, GetCurrentTimeLabel().c_str(), 10, 1.0f);
        Vector2 dateSize = MeasureTextEx(font, dateText.c_str(), 8, 1.0f);

        float totalHeight = timeSize.y + 2.0f + dateSize.y;
        float startY = area.y + (area.height - totalHeight) * 0.5f;

        Fumbo::Graphic2D::DrawText(
            GetCurrentTimeLabel(),
            {area.x + (area.width - timeSize.x) * 0.5f, startY}, font, 10,
            BLACK);
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
  Font font = OS::GlobalFont;

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
  Font font = OS::GlobalFont;

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
  Font font = OS::GlobalFont;

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

void DemoDesktop::DrawRadioContent(Rectangle area) {
  Font font = OS::GlobalFont;
  Vector2 scale = Fumbo::Utils::GetUIScale();
  Vector2 offset = Fumbo::Utils::GetUIOffset();
  Vector2 rawMouse = GetMousePosition();
  Vector2 mouse = {(rawMouse.x - offset.x) / scale.x,
                   (rawMouse.y - offset.y) / scale.y};

  // Draw Windows 95 gray body background
  Fumbo::Graphic2D::DrawRectangleRec(area, {192, 192, 192, 255});

  // Active contact info banner at top
  std::string contact = "Hendro";
  std::string role = "Dam Warden (Sector A)";
  if (m_shiftNumber == 2 || m_shiftNumber == 5) {
    contact = "Budi";
    role = "Fire Captain (Sector C)";
  } else if (m_shiftNumber == 3 || m_shiftNumber == 6 || m_shiftNumber == 7) {
    contact = "Dr. Surono";
    role = "Chief Volcanologist (Sector B)";
  }

  Rectangle banner = {area.x + 8.0f, area.y + 8.0f, area.width - 16.0f, 26.0f};
  Fumbo::Graphic2D::DrawRectangleRec(
      banner, {10, 36, 106, 255}); // Dark blue Windows 95 header
  Fumbo::Graphic2D::DrawText("CHANNEL 1: " + contact + " - " + role,
                             {banner.x + 8.0f, banner.y + 7.0f}, font, 10,
                             WHITE);

  // Chat message logs panel (inset frame style)
  Rectangle chatPanel = {area.x + 8.0f, area.y + 40.0f, area.width - 16.0f,
                         area.height - 95.0f};
  Fumbo::Graphic2D::DrawRectangleRec(chatPanel, WHITE);
  // Draw classic 3D border around chat panel (inset)
  Fumbo::Graphic2D::DrawLineEx({chatPanel.x, chatPanel.y},
                               {chatPanel.x + chatPanel.width, chatPanel.y},
                               1.5f, {128, 128, 128, 255});
  Fumbo::Graphic2D::DrawLineEx({chatPanel.x, chatPanel.y},
                               {chatPanel.x, chatPanel.y + chatPanel.height},
                               1.5f, {128, 128, 128, 255});
  Fumbo::Graphic2D::DrawLineEx(
      {chatPanel.x + chatPanel.width, chatPanel.y},
      {chatPanel.x + chatPanel.width, chatPanel.y + chatPanel.height}, 1.5f,
      WHITE);
  Fumbo::Graphic2D::DrawLineEx(
      {chatPanel.x, chatPanel.y + chatPanel.height},
      {chatPanel.x + chatPanel.width, chatPanel.y + chatPanel.height}, 1.5f,
      WHITE);

  // Render recent messages from bottom to top
  float chatY = chatPanel.y + chatPanel.height - 12.0f;
  for (int i = static_cast<int>(m_radioChatHistory.size()) - 1; i >= 0; --i) {
    const auto &msg = m_radioChatHistory[i];
    std::string sender = msg.first;
    std::string text = msg.second;
    std::string fullMsg = "[" + sender + "] " + text;

    // Wrap fullMsg into lines
    std::vector<std::string> lines;
    int lineStart = 0;
    int lineLength = 52; // Characters per line limit inside chat panel
    while (lineStart < (int)fullMsg.length()) {
      std::string line = fullMsg.substr(lineStart, lineLength);
      if (lineStart + lineLength < (int)fullMsg.length()) {
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
      lines.push_back(line);
    }

    // Draw lines of the wrapped message starting from the last line up
    for (int l = static_cast<int>(lines.size()) - 1; l >= 0; --l) {
      chatY -= 14.0f;
      if (chatY < chatPanel.y + 6.0f)
        break;

      Color col = BLACK;
      if (sender == "RADIO")
        col = {100, 100, 150, 255};
      else if (sender == "Hendro")
        col = {0, 0, 150, 255};
      else if (sender == "Budi")
        col = {150, 0, 0, 255};
      else if (sender == "Dr. Surono")
        col = {0, 120, 0, 255};
      else
        col = {40, 40, 40, 255}; // user

      Fumbo::Graphic2D::DrawText(lines[l], {chatPanel.x + 8.0f, chatY}, font, 9,
                                 col);
    }

    if (chatY < chatPanel.y + 6.0f)
      break;
  }

  // Draw loading overlay if pending AI response
  if (m_radioPending) {
    Fumbo::Graphic2D::DrawRectangleRec({chatPanel.x + 10.0f,
                                        chatPanel.y + chatPanel.height - 30.0f,
                                        chatPanel.width - 20.0f, 20.0f},
                                       {230, 230, 230, 200});
    Fumbo::Graphic2D::DrawText(
        "RECEIVING TRANSMISSION...",
        {chatPanel.x + 18.0f, chatPanel.y + chatPanel.height - 25.0f}, font, 9,
        {255, 140, 0, 255});
  }

  // Draw input text box and send button
  float inputY = area.y + area.height - 36.0f;
  m_radioInputTextbox.SetBounds(
      {area.x + 8.0f, inputY, area.width - 100.0f, 28.0f});
  m_radioInputTextbox.Draw();

  Rectangle sendBtn = {area.x + area.width - 82.0f, inputY, 74.0f, 28.0f};
  bool sendHovered = CheckCollisionPointRec(mouse, sendBtn);
  Fumbo::Graphic2D::DrawRectangleRec(sendBtn, {192, 192, 192, 255});
  Fumbo::Graphic2D::DrawLineEx({sendBtn.x, sendBtn.y},
                               {sendBtn.x + sendBtn.width, sendBtn.y}, 1.5f,
                               WHITE);
  Fumbo::Graphic2D::DrawLineEx({sendBtn.x, sendBtn.y},
                               {sendBtn.x, sendBtn.y + sendBtn.height}, 1.5f,
                               WHITE);
  Fumbo::Graphic2D::DrawLineEx(
      {sendBtn.x + sendBtn.width - 1.5f, sendBtn.y},
      {sendBtn.x + sendBtn.width - 1.5f, sendBtn.y + sendBtn.height}, 1.5f,
      BLACK);
  Fumbo::Graphic2D::DrawLineEx(
      {sendBtn.x, sendBtn.y + sendBtn.height - 1.5f},
      {sendBtn.x + sendBtn.width, sendBtn.y + sendBtn.height - 1.5f}, 1.5f,
      BLACK);

  Fumbo::Graphic2D::DrawText("SEND", {sendBtn.x + 22.0f, sendBtn.y + 9.0f},
                             font, 9, BLACK);

  // Trigger send action if clicked or enter pressed
  bool triggerSend = false;
  if (sendHovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
    triggerSend = true;
  }
  if (m_radioInputTextbox.IsFocused() &&
      (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER))) {
    triggerSend = true;
  }

  if (triggerSend && !m_radioPending) {
    std::string msg = m_radioInputTextbox.GetText();
    if (!msg.empty()) {
      Fumbo::Engine::Instance().GetAudioManager().PlaySound("click");
      m_radioChatHistory.push_back({"Officer", msg});
      m_radioInputTextbox.SetText("");

      // Generate telemetry context for the prompt
      std::string telemetryStr = "";
      int shift = m_shiftNumber;
      const auto &threat = m_gameManager.GetCurrentThreatCenter();
      if (shift == 1 || shift == 4) {
        telemetryStr =
            "Rainfall: " + std::to_string(threat.flood.rainfall) +
            " mm/h, River Depth: " + std::to_string(threat.flood.riverDepth) +
            " m";
      } else if (shift == 2 || shift == 5) {
        telemetryStr =
            "Temperature: " + std::to_string(threat.wildfire.temperature) +
            "C, Humidity: " + std::to_string(threat.wildfire.humidity) + "%";
      } else {
        telemetryStr =
            "Tremors: " + std::to_string(threat.earthquake.tremorsPerHour) +
            "/h, Gas: " + std::to_string(threat.earthquake.gasEmission) +
            " ppm, Ground: " +
            std::to_string(threat.earthquake.groundDeformation) + " mm";
      }

      // Launch async AI request
      m_radioFuture =
          Game::AIService::RequestChatResponseAsync(contact, msg, telemetryStr);
      m_radioPending = true;
    }
  }
}

void DemoDesktop::DrawAboutContent(Rectangle area) {
  Font font = OS::GlobalFont;
  Fumbo::Graphic2D::DrawText("Sentinels", {area.x + 15.0f, area.y + 15.0f},
                             font, 16, WHITE);
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
  Font font = OS::GlobalFont;

  // 2. Define Layout columns (58% left map, 42% right SOP)
  float leftWidth = area.width * 0.58f;
  float rightWidth = area.width * 0.42f;

  float leftX = area.x + padding;
  float leftY = area.y + padding;
  float leftW = leftWidth - (padding * 1.5f);
  float leftH = area.height - (padding * 2.0f);

  // Draw header for Map section
  Fumbo::Graphic2D::DrawText("REGIONAL MONITORING MAP",
                             {leftX + 5.0f, leftY + 3.0f}, font, 13,
                             {0, 230, 118, 255});
  Fumbo::Graphic2D::DrawText("CURRENT TIME: " + GetCurrentTimeLabel(),
                             {leftX + 5.0f, leftY + 20.0f}, font, 10,
                             {255, 215, 0, 255});

  // Draw simulated map box
  Rectangle mapBox = {leftX, leftY + 38.0f, leftW, leftH - 38.0f};
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
                             {rightX + 5.0f, rightY + 3.0f}, font, 13,
                             {255, 215, 0, 255});

  // Draw a content frame for SOP text
  Rectangle sopBox = {rightX, rightY + 38.0f, rightW, rightH - 38.0f};
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
    if (m_currentTimeWindowIndex >= 0 &&
        m_currentTimeWindowIndex < static_cast<int>(shift.windows.size())) {
      threat =
          shift.windows.at(static_cast<std::size_t>(m_currentTimeWindowIndex))
              .threatCenter;
    }
  }

  // SEC-A: Flood sensors
  char bufA[128];
  std::snprintf(
      bufA, sizeof(bufA), "SEC-A: %.1fm | %dmm/h (%s)", threat.flood.riverDepth,
      static_cast<int>(threat.flood.rainfall),
      (threat.flood.riverDepth >= 4.6f || threat.flood.rainfall >= 80.0f)
          ? "WARN"
          : "OK");
  std::string monitorA(bufA);

  // SEC-B: Seismic
  char bufB[128];
  std::snprintf(bufB, sizeof(bufB), "SEC-B: %d tremors/h (%s)",
                threat.earthquake.tremorsPerHour,
                (threat.earthquake.tremorsPerHour >= 11) ? "WATCH" : "OK");
  std::string monitorB(bufB);

  // SEC-C: Wildfire
  char bufC[128];
  std::snprintf(
      bufC, sizeof(bufC), "SEC-C: %d C | Hum %d%% (%s)",
      static_cast<int>(threat.wildfire.temperature),
      static_cast<int>(threat.wildfire.humidity),
      (threat.wildfire.temperature >= 41 || threat.wildfire.humidity <= 9)
          ? "CRITICAL"
          : ((threat.wildfire.temperature >= 39 ||
              threat.wildfire.humidity <= 12)
                 ? "WARN"
                 : "OK"));
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

  Font font = OS::GlobalFont;
  float padding = 12.0f;
  Vector2 scale = Fumbo::Utils::GetUIScale();
  Vector2 offset = Fumbo::Utils::GetUIOffset();
  Vector2 rawMouse = GetMousePosition();
  Vector2 mouse = {(rawMouse.x - offset.x) / scale.x,
                   (rawMouse.y - offset.y) / scale.y};

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
  if (!log.empty())
    budgetSpentInt = log.back().budgetSpent;
  std::string budgetSpent = "$" + std::to_string(std::max(0, budgetSpentInt));
  std::string trustDelta =
      (m_publicTrust >= 85) ? "+0" : std::to_string(m_publicTrust - 85);
  if (m_publicTrust != 85 && trustDelta[0] != '-') {
    trustDelta = "+" + trustDelta;
  }
  std::string primaryThreat = "No completed shifts";
  if (!log.empty()) {
    primaryThreat = "Shift " + std::to_string(log.back().shiftNumber);
  }
  std::string shiftStatus =
      log.empty() ? "No completed shifts" : "Review active";

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
      Fumbo::Graphic2D::DrawText(std::string("Shift Summary"),
                                 {rowRect.x + 58.0f, rowRect.y + 8.0f}, font, 9,
                                 WHITE);
      Fumbo::Graphic2D::DrawText(std::to_string(entry.peopleSaved) + " saved",
                                 {rowRect.x + 58.0f, rowRect.y + 18.0f}, font,
                                 8, {150, 150, 170, 255});

      // Rating badge
      Rectangle badgeRect = {rowRect.x + rowRect.width - 64.0f,
                             rowRect.y + 8.0f, 56.0f, 16.0f};
      Color ratingColor = {255, 23, 68, 255};
      if (entry.overallRating >= 4.0f)
        ratingColor = {0, 230, 118, 255};
      else if (entry.overallRating >= 2.0f)
        ratingColor = {255, 215, 0, 255};
      Fumbo::Graphic2D::DrawRectangleRounded(badgeRect, 0.2f, 4, ratingColor);
      char ratingBuf[8];
      std::snprintf(ratingBuf, sizeof(ratingBuf), "%.1f/5",
                    entry.overallRating);
      Fumbo::Graphic2D::DrawText(std::string(ratingBuf),
                                 {badgeRect.x + 8.0f, badgeRect.y + 3.0f}, font,
                                 8, WHITE);

      if (hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        Fumbo::Engine::Instance().GetAudioManager().PlaySound("click");
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
    Fumbo::Graphic2D::DrawText(
        "Select a completed shift to inspect its details.",
        {detailPanel.x + 12.0f, detailPanel.y + 42.0f}, font, 10,
        {140, 140, 160, 255});
  } else {
    const Game::GameState::ShiftSummary &entry =
        log[m_selectedDecisionLogEntry];
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
    DrawDetailRow("Budget Spent",
                  std::string("$") + std::to_string(entry.budgetSpent));
    DrawDetailRow("Public Trust", std::to_string(entry.publicTrust) + "%");
    if (!entry.actionsTaken.empty()) {
      std::string actions;
      for (const auto &a : entry.actionsTaken) {
        if (!actions.empty())
          actions += ", ";
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
    cards[0] = {"BREAKING NEWS",
                "[!]",
                comms.breakingNews.title,
                comms.breakingNews.timestamp,
                pr.first,
                comms.breakingNews.preview,
                comms.breakingNews.fullReport,
                pr.second,
                comms.breakingNews.unread};
  }
  // 1: Weather
  {
    auto pr = PriorityToString(comms.weather.priority);
    cards[1] = {"WEATHER UPDATE",         "[W]",     comms.weather.title,
                comms.weather.timestamp,  pr.first,  comms.weather.preview,
                comms.weather.fullReport, pr.second, comms.weather.unread};
  }
  // 2: Citizen
  {
    auto pr = PriorityToString(comms.citizen.priority);
    cards[2] = {"CITIZEN REPORT",         "[C]",     comms.citizen.title,
                comms.citizen.timestamp,  pr.first,  comms.citizen.preview,
                comms.citizen.fullReport, pr.second, comms.citizen.unread};
  }
  // 3: Sensor
  {
    auto pr = PriorityToString(comms.sensor.priority);
    cards[3] = {"CCTV / SENSOR",         "[S]",     comms.sensor.title,
                comms.sensor.timestamp,  pr.first,  comms.sensor.preview,
                comms.sensor.fullReport, pr.second, comms.sensor.unread};
  }
  // 4: Rescue
  {
    auto pr = PriorityToString(comms.rescue.priority);
    cards[4] = {"RESCUE REPORT",         "[R]",     comms.rescue.title,
                comms.rescue.timestamp,  pr.first,  comms.rescue.preview,
                comms.rescue.fullReport, pr.second, comms.rescue.unread};
  }
  // 5: Infrastructure
  {
    auto pr = PriorityToString(comms.infrastructure.priority);
    cards[5] = {"INFRASTRUCTURE",
                "[I]",
                comms.infrastructure.title,
                comms.infrastructure.timestamp,
                pr.first,
                comms.infrastructure.preview,
                comms.infrastructure.fullReport,
                pr.second,
                comms.infrastructure.unread};
  }

  float padding = 12.0f;
  float gap = 10.0f;
  Font font = OS::GlobalFont;
  std::string currentTime = GetCurrentTimeLabel();

  // Grid calculation split
  float leftW = (area.width - 3.0f * padding) * 0.48f;
  float rightW = (area.width - 3.0f * padding) * 0.52f;
  float leftH = area.height - 2.0f * padding;

  // Track if mouse is over any card
  Vector2 scale = Fumbo::Utils::GetUIScale();
  Vector2 offset = Fumbo::Utils::GetUIOffset();
  Vector2 rawMouse = GetMousePosition();
  Vector2 mouse = {(rawMouse.x - offset.x) / scale.x,
                   (rawMouse.y - offset.y) / scale.y};

  // Helper to draw a single card
  auto DrawCard = [&](int index, Rectangle r, bool isFeatured) {
    bool hovered = CheckCollisionPointRec(mouse, r);

    // Handle click interaction
    if (hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
        s_selectedCommsCard == -1) {
      Fumbo::Engine::Instance().GetAudioManager().PlaySound("click");
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
    Fumbo::Graphic2D::DrawText(std::string("RECEIVED: ") +
                                   cards[index].timestamp,
                               {x, y}, font, 8, {140, 140, 160, 255});

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
    Fumbo::Graphic2D::DrawText(std::string("TIME: ") +
                                   cards[s_selectedCommsCard].timestamp +
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
      Fumbo::Engine::Instance().GetAudioManager().PlaySound("click");
      s_selectedCommsCard = -1; // Dismiss modal
    }
  }
}

void DemoDesktop::DrawMitigationHub(Rectangle area) {
  int shiftIdx = m_shiftNumber;
  if (shiftIdx == 1 || shiftIdx == 4)
    s_mitigationTab = 0;
  else if (shiftIdx == 2 || shiftIdx == 5)
    s_mitigationTab = 1;
  else
    s_mitigationTab = 2;

  Game::DisasterType currentType = Game::DisasterType::Flood;
  if (s_mitigationTab == 1)
    currentType = Game::DisasterType::Wildfire;
  else if (s_mitigationTab == 2)
    currentType = Game::DisasterType::Earthquake;

  // Dark background
  Fumbo::Graphic2D::DrawRectangleRec(area, {20, 24, 33, 255});

  Font font = OS::GlobalFont;
  float padding = 12.0f;
  Vector2 scale = Fumbo::Utils::GetUIScale();
  Vector2 offset = Fumbo::Utils::GetUIOffset();
  Vector2 rawMouse = GetMousePosition();
  Vector2 mouse = {(rawMouse.x - offset.x) / scale.x,
                   (rawMouse.y - offset.y) / scale.y};
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
  Fumbo::Graphic2D::DrawText(
      "TIME: " + GetCurrentTimeLabel(),
      {area.x + padding + 90.0f, area.y + padding + 2.0f}, font, 10,
      {0, 230, 118, 255});
  std::string progressText =
      "WINDOW " + std::to_string(m_currentTimeWindowIndex + 1) + " / 4";
  Fumbo::Graphic2D::DrawText(
      progressText, {area.x + padding + 210.0f, area.y + padding + 2.0f}, font,
      10, {140, 180, 255, 255});

  // ========== 1. RESOURCE OVERVIEW BAR ==========
  float barH = 34.0f;
  Rectangle resBar = {area.x + padding, area.y + padding + 18.0f,
                      area.width - 2 * padding, barH};
  Fumbo::Graphic2D::DrawRectangleRounded(resBar, 0.08f, 4, {28, 33, 46, 255});
  Fumbo::Graphic2D::DrawRectangleRoundedLinesEx(resBar, 0.08f, 4, 1.0f,
                                                {50, 60, 80, 255});

  float rx = resBar.x + 14.0f;
  float ry = resBar.y + 10.0f;
  float colSpacing = (resBar.width - 28.0f) / 3.0f;

  Fumbo::Graphic2D::DrawText("BUDGET:", {rx, ry}, font, 9,
                             {150, 150, 170, 255});

  Fumbo::Graphic2D::DrawText("PUBLIC TRUST:", {rx + colSpacing, ry}, font, 9,
                             {150, 150, 170, 255});

  Fumbo::Graphic2D::DrawText("RESCUE TEAMS:", {rx + 2 * colSpacing, ry}, font,
                             9, {150, 150, 170, 255});
  // Live values from backend GameState
  {
    Game::GameState &gs = m_gameManager.GetGameState();
    Fumbo::Graphic2D::DrawText(std::string("$") + std::to_string(gs.budget),
                               {rx + 55.0f, ry}, font, 10,
                               {140, 255, 140, 255});
    Fumbo::Graphic2D::DrawText(std::to_string(gs.publicTrust) + "%",
                               {rx + colSpacing + 90.0f, ry}, font, 10,
                               {0, 230, 118, 255});
    Fumbo::Graphic2D::DrawText(std::to_string(gs.rescueTeams) + " AVAILABLE",
                               {rx + 2 * colSpacing + 95.0f, ry}, font, 10,
                               {140, 180, 255, 255});
  }

  // ========== 2. DISASTER TABS ==========
  float tabY = area.y + padding + 18.0f + barH + 8.0f;
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

    if (tabHovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
        t == s_mitigationTab) {
      Fumbo::Engine::Instance().GetAudioManager().PlaySound("click");
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

  // Helper mapping from tab/action index -> Game::ActionID (available to the
  // whole function)
  auto getActionId = [](int tabIdx, int actionIdx) -> Game::ActionID {
    using Game::ActionID;
    if (tabIdx == 0) {
      switch (actionIdx) {
      case 0:
        return ActionID::IssueWarning;
      case 1:
        return ActionID::DeployRescue;
      case 2:
        return ActionID::OpenShelter;
      case 3:
        return ActionID::CloseRoad;
      case 4:
        return ActionID::EvacuateResidents;
      }
    } else if (tabIdx == 1) {
      switch (actionIdx) {
      case 0:
        return ActionID::IssueWarning;
      case 1:
        return ActionID::DeployRescue;
      case 2:
        return ActionID::RequestAirSupport;
      case 3:
        return ActionID::EvacuateResidents;
      case 4:
        return ActionID::CloseRoad;
      }
    } else {
      switch (actionIdx) {
      case 0:
        return ActionID::IssueWarning;
      case 1:
        return ActionID::EvacuateResidents;
      case 2:
        return ActionID::CloseRoad;
      case 3:
        return ActionID::OpenShelter;
      case 4:
        return ActionID::DeployRescue;
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
    bool queued =
        (std::find(gs.selectedActions.begin(), gs.selectedActions.end(), aid) !=
         gs.selectedActions.end());

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
    std::string costStr =
        "COST: $" +
        std::to_string(Game::Simulation::GetActionBudgetCost(aid, currentType));
    Fumbo::Graphic2D::DrawText(
        costStr, {actionRect.x + actionRect.width - 90.0f, actionRect.y + 8.0f},
        font, 9, {255, 140, 140, 255});

    // Toggle on click: add/remove ActionID from GameState.selectedActions
    if (hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
      Fumbo::Engine::Instance().GetAudioManager().PlaySound("click");
      auto it =
          std::find(gs.selectedActions.begin(), gs.selectedActions.end(), aid);
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
    Game::ActionID previewAid = getActionId(tab, ai);
    std::string costValStr =
        "$" + std::to_string(Game::Simulation::GetActionBudgetCost(
                  previewAid, currentType));
    Fumbo::Graphic2D::DrawText(costValStr, {px + 100.0f, py}, font, 10,
                               {255, 140, 140, 255});
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
      if (std::find(gsQueue.selectedActions.begin(),
                    gsQueue.selectedActions.end(),
                    aid) != gsQueue.selectedActions.end()) {
        actionsInTab.push_back(a);
      }
    }
    if (actionsInTab.empty())
      continue;

    // Tab section label
    std::string secLabel = std::string("[") + tabIcons[t] + "] " + tabNames[t];
    Fumbo::Graphic2D::DrawText(secLabel, {rightX + 12.0f, queueY}, font, 9,
                               tabColors[t]);
    queueY += 16.0f;

    for (int idx : actionsInTab) {
      Rectangle qItemRect = {rightX + 8.0f, queueY, rightW - 16.0f, 24.0f};
      bool qHovered = CheckCollisionPointRec(mouse, qItemRect);

      Color qBg = qHovered ? Color{50, 30, 30, 255} : Color{26, 30, 42, 255};
      Fumbo::Graphic2D::DrawRectangleRounded(qItemRect, 0.06f, 4, qBg);

      // Colored left accent
      Rectangle accent = {qItemRect.x, qItemRect.y + 2.0f, 3.0f,
                          qItemRect.height - 4.0f};
      Fumbo::Graphic2D::DrawRectangleRec(accent, tabColors[t]);

      // Action name
      std::string aName = actions[t][idx].name;
      if (aName.length() > 24)
        aName = aName.substr(0, 21) + "...";
      Fumbo::Graphic2D::DrawText(aName,
                                 {qItemRect.x + 10.0f, qItemRect.y + 6.0f},
                                 font, 9, {210, 210, 230, 255});

      // Remove button
      float removeX = qItemRect.x + qItemRect.width - 26.0f;
      Color removeColor =
          qHovered ? Color{255, 80, 80, 255} : Color{120, 120, 150, 255};
      Fumbo::Graphic2D::DrawText("[X]", {removeX, qItemRect.y + 6.0f}, font, 9,
                                 removeColor);

      if (qHovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        Fumbo::Engine::Instance().GetAudioManager().PlaySound("click");
        // remove from GameState.selectedActions
        Game::ActionID aid = getActionId(t, idx);
        auto it = std::find(gsQueue.selectedActions.begin(),
                            gsQueue.selectedActions.end(), aid);
        if (it != gsQueue.selectedActions.end())
          gsQueue.selectedActions.erase(it);
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

  Color endBg = (canAdvance) ? (endHovered ? Color{200, 40, 40, 255}
                                           : Color{160, 25, 25, 255})
                             : Color{50, 50, 60, 255};
  Fumbo::Graphic2D::DrawRectangleRounded(endBtn, 0.1f, 4, endBg);
  Fumbo::Graphic2D::DrawRectangleRoundedLinesEx(
      endBtn, 0.1f, 4, 1.2f,
      canAdvance ? Color{255, 80, 80, 255} : Color{70, 70, 85, 255});

  Color btnTextColor = canAdvance ? WHITE : Color{100, 100, 110, 255};
  Fumbo::Graphic2D::DrawText(buttonLabel, {endBtn.x + 12.0f, endBtn.y + 10.0f},
                             font, 10, btnTextColor);

  // Queue count badge
  if (queueCount > 0) {
    std::string countStr = "[" + std::to_string(queueCount) + "]";
    Fumbo::Graphic2D::DrawText(countStr,
                               {endBtn.x + btnW - 30.0f, endBtn.y + 10.0f},
                               font, 10, {255, 215, 0, 255});
  }

  if (m_shiftNumber == 1) {
    float pulse = (sinf(GetTime() * 6.0f) + 1.0f) * 0.5f;
    Color glowColor = {255, 215, 0, (unsigned char)(100 + pulse * 155)};
    Fumbo::Graphic2D::DrawRectangleRoundedLinesEx(endBtn, 0.1f, 4, 2.0f,
                                                  glowColor);
  }

  if (s_showEndShiftConfirm && canAdvance) {
    Fumbo::Graphic2D::DrawRectangleRec(area, {12, 14, 21, 220});
    Rectangle modalRect = {area.x + 70.0f, area.y + 70.0f, area.width - 140.0f,
                           area.height - 140.0f};
    Fumbo::Graphic2D::DrawRectangleRounded(modalRect, 0.04f, 4,
                                           {24, 29, 40, 255});
    Fumbo::Graphic2D::DrawRectangleRoundedLinesEx(modalRect, 0.04f, 4, 1.2f,
                                                  {255, 80, 80, 255});

    std::string confirmTitle = (m_currentTimeWindowIndex < 3)
                                   ? "ADVANCE TIME WINDOW"
                                   : "COMPLETE SHIFT";
    std::string confirmText =
        (m_currentTimeWindowIndex < 3)
            ? "Execute the queued actions and advance to the next operational "
              "window?"
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
      Fumbo::Engine::Instance().GetAudioManager().PlaySound("click");
      // Create decision log entries from the current selected actions
      Game::GameState &gsConfirm = m_gameManager.GetGameState();
      for (int t = 0; t < 3; t++) {
        for (int a = 0; a < 5; a++) {
          Game::ActionID aid = getActionId(t, a);
          if (std::find(gsConfirm.selectedActions.begin(),
                        gsConfirm.selectedActions.end(),
                        aid) == gsConfirm.selectedActions.end())
            continue;
          std::string disaster =
              (t == 0) ? "Flood" : (t == 1 ? "Wildfire" : "Volcano");
          std::string sector =
              (t == 0) ? "Sector A" : (t == 1 ? "Sector C" : "Sector B");
          AddDecisionLogEntry(
              actions[t][a].name, disaster, sector, "Completed",
              (std::string(actions[t][a].budgetImpact) == "---" ||
                       std::string(actions[t][a].budgetImpact) == "--" ||
                       std::string(actions[t][a].budgetImpact) == "-"
                   ? "$5,000"
                   : "$10,000"),
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
        m_currentTimeLabel = std::string(
            Game::GameState::GetWindowDisplayTime(m_currentTimeWindowIndex));
        m_budget = gs.budget;
        m_publicTrust = gs.publicTrust;
        m_shiftNumber = m_gameManager.GetCurrentShift();

        if (!shiftContinues) {
          // Shift ended: record a ShiftSummary in GameState.decisionLog and
          // show summary
          Game::GameState &gsFinal = m_gameManager.GetGameState();
          Game::GameState::ShiftSummary summary;
          summary.shiftNumber = gsFinal.currentShift;
          summary.peopleSaved = gsFinal.peopleSaved;
          summary.casualties = gsFinal.casualties;
          summary.budgetSpent = gsFinal.shiftStartingBudget - gsFinal.budget;
          summary.publicTrust = gsFinal.publicTrust;
          // Build actionsTaken from the entries we added to the UI decision log
          // earlier
          for (const auto &entry : m_decisionLog) {
            summary.actionsTaken.push_back(entry.title);
          }
          // Compute a simple overall rating from public trust (normalized)
          summary.overallRating =
              std::clamp(gsFinal.publicTrust / 20.0f, 0.0f, 5.0f);
          gsFinal.decisionLog.push_back(summary);
          m_shiftSummaryVisible = true;
        } else {
          // Shift continues: keep UI in next window. Keep legacy notifications.
          if (m_currentTimeWindowIndex >= 2) {
            Fumbo::Engine::Instance().GetAudioManager().PlaySound("warning");
          }
          if (m_currentTimeWindowIndex == 2) {
            m_desktop->Notify(
                "Threat Center",
                "Critical update: conditions are worsening across the city.");
          } else if (m_currentTimeWindowIndex == 3) {
            m_desktop->Notify(
                "Threat Center",
                "Emergency update: evacuation zones are now active.");
          } else {
            m_desktop->Notify("Operations",
                              "Advance complete. Current time is " +
                                  GetCurrentTimeLabel() + ".");
          }
        }
      }

      s_showEndShiftConfirm = false;
    } else if (cancelHovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
      Fumbo::Engine::Instance().GetAudioManager().PlaySound("click");
      s_showEndShiftConfirm = false;
    }
  } else if (endHovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
             canAdvance) {
    Fumbo::Engine::Instance().GetAudioManager().PlaySound("click");
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
  Fumbo::Graphic2D::DrawLineEx({0.0f, barHeight - 1.0f},
                               {screenWidth, barHeight - 1.0f}, 1.0f,
                               {128, 128, 128, 255});

  Font font = GetFontDefault();
  float textY = 8.0f;

  auto DrawInsetBox = [](Rectangle r) {
    Fumbo::Graphic2D::DrawRectangleRec(r, {192, 192, 192, 255});
    Fumbo::Graphic2D::DrawLineEx({r.x, r.y}, {r.x + r.width, r.y}, 1.0f,
                                 {128, 128, 128, 255});
    Fumbo::Graphic2D::DrawLineEx({r.x, r.y}, {r.x, r.y + r.height}, 1.0f,
                                 {128, 128, 128, 255});
    Fumbo::Graphic2D::DrawLineEx({r.x + r.width - 1.0f, r.y},
                                 {r.x + r.width - 1.0f, r.y + r.height}, 1.0f,
                                 WHITE);
    Fumbo::Graphic2D::DrawLineEx({r.x, r.y + r.height - 1.0f},
                                 {r.x + r.width, r.y + r.height - 1.0f}, 1.0f,
                                 WHITE);
  };

  // Helper mapping from tab/action index -> Game::ActionID
  auto getActionId = [](int tabIdx, int actionIdx) -> Game::ActionID {
    using Game::ActionID;
    if (tabIdx == 0) {
      switch (actionIdx) {
      case 0:
        return ActionID::IssueWarning;
      case 1:
        return ActionID::DeployRescue;
      case 2:
        return ActionID::OpenShelter;
      case 3:
        return ActionID::CloseRoad;
      case 4:
        return ActionID::EvacuateResidents;
      }
    } else if (tabIdx == 1) {
      switch (actionIdx) {
      case 0:
        return ActionID::IssueWarning;
      case 1:
        return ActionID::DeployRescue;
      case 2:
        return ActionID::RequestAirSupport;
      case 3:
        return ActionID::EvacuateResidents;
      case 4:
        return ActionID::CloseRoad;
      }
    } else {
      switch (actionIdx) {
      case 0:
        return ActionID::IssueWarning;
      case 1:
        return ActionID::EvacuateResidents;
      case 2:
        return ActionID::CloseRoad;
      case 3:
        return ActionID::OpenShelter;
      case 4:
        return ActionID::DeployRescue;
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
  Fumbo::Graphic2D::DrawText(m_cityStatus, {335.0f, textY}, font, 12,
                             statusColor);

  // 3. Budget
  DrawInsetBox({580.0f, 4.0f, 320.0f, 22.0f});
  Fumbo::Graphic2D::DrawText("BUDGET: ", {590.0f, textY}, font, 12, BLACK);
  std::string budgetValText = "$" + std::to_string(gs.budget);
  Fumbo::Graphic2D::DrawText(budgetValText, {685.0f, textY}, font, 12,
                             {0, 128, 0, 255});

  // 4. Public Trust
  DrawInsetBox({920.0f, 4.0f, 320.0f, 22.0f});
  Fumbo::Graphic2D::DrawText("PUBLIC TRUST: ", {930.0f, textY}, font, 12,
                             BLACK);
  std::string trustText = std::to_string(gs.publicTrust) + "%";
  Color trustColor = {0, 128, 0, 255};
  if (gs.publicTrust < 35)
    trustColor = {180, 0, 0, 255};
  else if (gs.publicTrust < 70)
    trustColor = {215, 100, 0, 255};
  Fumbo::Graphic2D::DrawText(trustText, {1090.0f, textY}, font, 12, trustColor);
}

void DemoDesktop::OpenTutorialWindow() {
  m_desktop->OpenWindow(
      "How to Play - Sentinel", {360, 100, 560, 480},
      [this](Rectangle area) { DrawTutorialContent(area); }, m_bookmarkIconTex);
}

void DemoDesktop::DrawTutorialContent(Rectangle area) {
  Fumbo::Graphic2D::DrawRectangleRec(
      area, {240, 240, 240, 255}); // Win95 light gray window bg
  Font font = GetFontDefault();
  float x = area.x + 15.0f;
  float y = area.y + 15.0f;

  Fumbo::Graphic2D::DrawText("SENTINEL OPERATIONAL MANUAL", {x, y}, font, 11,
                             {0, 0, 128, 255});
  y += 20.0f;

  const char *manualLines[] = {
      "Your objective: Protect the city while managing resources.",
      "",
      "1. READ TELEMETRY (Threat Center)",
      "   Open the Threat Center window to monitor sensors.",
      "   Watch out for thresholds exceeding safe levels.",
      "",
      "2. READ COMMS & REPORTS (Comms)",
      "   Check social media posts, BMKG, and breaking news.",
      "   Some reports may be unverified, conflicting, or false.",
      "",
      "3. CO-ORDINATE MITIGATION (Mitigation Hub)",
      "   Select actions such as evacuation, warnings, and rescue.",
      "   Click the EXECUTE button at the bottom to process actions",
      "   and advance the time window (08:00 -> 11:00 -> 14:00 -> 17:00).",
      "",
      "4. THE ECONOMIC BALANCE (Critical Learning)",
      "   * Evacuating too early: Costs money ($12,000) and decreases",
      "     Public Trust due to false panic/economic disruption.",
      "   * Evacuating too late: Prevents dam/fire/volcano casualties,",
      "     but leads to higher casualties and infrastructure damage.",
      "",
      "5. DAYS & SHIFTS",
      "   * Shift 1: Flood (Sector A)",
      "   * Shift 2: Wildfire (Sector C)",
      "   * Shift 3: Volcano Eruption (Sector B)",
      "   Maintain budget & trust across all shifts to win."};

  for (int i = 0; i < 26; i++) {
    Fumbo::Graphic2D::DrawText(manualLines[i], {x, y}, font, 9, BLACK);
    y += 15.0f;
  }
}
