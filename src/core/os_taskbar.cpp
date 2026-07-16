#include "os_taskbar.hpp"
#include <algorithm>
#include <ctime>

namespace OS {

// Setup

void Taskbar::SetFont(Font font) { m_font = font; }
void Taskbar::SetStyle(const TaskbarStyle &style) { m_style = style; }

// Start Menu

void Taskbar::AddStartMenuItem(const std::string &label,
                               std::function<void()> callback,
                               Texture2D icon) {
  m_startMenuItems.push_back({label, icon, std::move(callback)});
}

void Taskbar::ClearStartMenu() { m_startMenuItems.clear(); }

// System Tray

void Taskbar::AddTrayItem(const std::string &id,
                          std::function<void(Rectangle area)> drawCallback,
                          std::function<void()> clickCallback, float width) {
  // Replace if exists
  RemoveTrayItem(id);
  m_trayItems.push_back({id, std::move(drawCallback), std::move(clickCallback), width});
}

void Taskbar::RemoveTrayItem(const std::string &id) {
  m_trayItems.erase(
      std::remove_if(m_trayItems.begin(), m_trayItems.end(),
                     [&](const TrayItem &item) { return item.id == id; }),
      m_trayItems.end());
}

// Queries

float Taskbar::GetHeight() const { return m_style.height; }

Rectangle Taskbar::GetBounds() const {
  return {0, Fumbo::Utils::UI_HEIGHT - m_style.height,
          Fumbo::Utils::UI_WIDTH, m_style.height};
}

bool Taskbar::IsStartMenuOpen() const { return m_startMenuOpen; }

void Taskbar::CloseStartMenu() { m_startMenuOpen = false; }

// Helpers

Rectangle Taskbar::GetStartButtonRect() const {
  float y = Fumbo::Utils::UI_HEIGHT - m_style.height;
  return {2.0f, y + 2.0f, m_style.startButtonWidth, m_style.height - 4.0f};
}

Rectangle Taskbar::GetWindowListArea() const {
  float y = Fumbo::Utils::UI_HEIGHT - m_style.height;
  float startEnd = m_style.startButtonWidth + 8.0f;

  // Calculate tray width
  float trayWidth = 0;
  for (auto &item : m_trayItems) {
    trayWidth += item.width + 2.0f;
  }
  trayWidth += 8.0f; // padding

  float listWidth = Fumbo::Utils::UI_WIDTH - startEnd - trayWidth - 8.0f;
  return {startEnd, y, listWidth, m_style.height};
}

Rectangle Taskbar::GetTrayArea() const {
  float y = Fumbo::Utils::UI_HEIGHT - m_style.height;
  float trayWidth = 0;
  for (auto &item : m_trayItems) {
    trayWidth += item.width + 2.0f;
  }
  trayWidth += 8.0f;

  return {Fumbo::Utils::UI_WIDTH - trayWidth, y, trayWidth, m_style.height};
}

Rectangle Taskbar::GetStartMenuRect() const {
  float menuHeight = m_startMenuItems.size() * m_style.startMenuItemHeight + 8.0f;
  float y = Fumbo::Utils::UI_HEIGHT - m_style.height - menuHeight;
  return {2.0f, y, m_style.startMenuWidth, menuHeight};
}

// Update

bool Taskbar::Update() {
  Vector2 scale = Fumbo::Utils::GetUIScale();
  Vector2 offset = Fumbo::Utils::GetUIOffset();
  Vector2 mouse = GetMousePosition();
  Vector2 mouseUI = {(mouse.x - offset.x) / scale.x,
                     (mouse.y - offset.y) / scale.y};

  bool consumed = false;

  // Check taskbar area
  Rectangle taskbarBounds = GetBounds();
  bool overTaskbar = CheckCollisionPointRec(mouseUI, taskbarBounds);

  if (m_startMenuOpen) {
    Rectangle menuRect = GetStartMenuRect();
    bool overMenu = CheckCollisionPointRec(mouseUI, menuRect);

    m_startMenuHoverIndex = -1;
    if (overMenu) {
      float itemY = menuRect.y + 4.0f;
      for (int i = 0; i < (int)m_startMenuItems.size(); i++) {
        Rectangle itemRect = {menuRect.x + 4.0f, itemY,
                              menuRect.width - 8.0f, m_style.startMenuItemHeight};
        if (CheckCollisionPointRec(mouseUI, itemRect)) {
          m_startMenuHoverIndex = i;
          if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            if (m_startMenuItems[i].callback)
              m_startMenuItems[i].callback();
            m_startMenuOpen = false;
            return true;
          }
        }
        itemY += m_style.startMenuItemHeight;
      }
      consumed = true;
    }

    // Click outside menu and taskbar closes the menu
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && !overMenu && !overTaskbar) {
      m_startMenuOpen = false;
    }
  }

  if (!overTaskbar)
    return consumed || m_startMenuOpen;

  consumed = true;

  Rectangle startBtn = GetStartButtonRect();
  m_startHover = CheckCollisionPointRec(mouseUI, startBtn);

  if (m_startHover && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
    m_startMenuOpen = !m_startMenuOpen;
    return true;
  }

  Rectangle listArea = GetWindowListArea();
  m_windowItemHoverId = -1;

  auto &wm = WindowManager::Instance();
  auto openIds = wm.GetOpenWindowIds();

  float itemX = listArea.x + m_style.itemSpacing;
  float itemY = listArea.y + (listArea.height - m_style.windowItemHeight) * 0.5f;

  for (int id : openIds) {
    auto *w = wm.GetWindow(id);
    if (!w)
      continue;

    Rectangle itemRect = {itemX, itemY, m_style.windowItemWidth,
                          m_style.windowItemHeight};

    if (CheckCollisionPointRec(mouseUI, itemRect)) {
      m_windowItemHoverId = id;

      if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        if (w->IsMinimized()) {
          wm.RestoreWindow(id);
        } else if (w->IsFocused()) {
          wm.MinimizeWindow(id);
        } else {
          wm.FocusWindow(id);
        }
        return true;
      }
    }

    itemX += m_style.windowItemWidth + m_style.itemSpacing;
    if (itemX + m_style.windowItemWidth > listArea.x + listArea.width)
      break;
  }

  Rectangle trayArea = GetTrayArea();
  float trayX = trayArea.x + 4.0f;
  for (auto &item : m_trayItems) {
    Rectangle itemRect = {trayX, trayArea.y + 2.0f, item.width,
                          trayArea.height - 4.0f};
    if (CheckCollisionPointRec(mouseUI, itemRect) &&
        IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
      if (item.clickCallback)
        item.clickCallback();
      return true;
    }
    trayX += item.width + 2.0f;
  }

  return consumed;
}

// Draw

void Taskbar::Draw() {
  Rectangle taskbarBounds = GetBounds();
  Fumbo::Graphic2D::DrawRectangleRec(taskbarBounds, m_style.backgroundColor);

  // Top border line
  Fumbo::Graphic2D::DrawLineEx({taskbarBounds.x, taskbarBounds.y},
                               {taskbarBounds.x + taskbarBounds.width,
                                taskbarBounds.y},
                               1.0f, m_style.borderColor);

  Rectangle startBtn = GetStartButtonRect();
  Color startColor = m_startHover || m_startMenuOpen
                         ? m_style.startButtonHoverColor
                         : m_style.startButtonColor;
  Fumbo::Graphic2D::DrawRectangleRounded(startBtn, 0.15f, 4, startColor);

  // Start button text
  float textY =
      startBtn.y + (startBtn.height - m_style.fontSize) * 0.5f;
  Fumbo::Graphic2D::DrawText("Start", {startBtn.x + 20.0f, textY}, m_font,
                             m_style.fontSize, m_style.textColor);

  // A small icon/symbol on start button
  float dotY = startBtn.y + startBtn.height * 0.5f;
  Fumbo::Graphic2D::DrawCircleV({startBtn.x + 10.0f, dotY}, 3.5f,
                                m_style.textColor);

  auto &wm = WindowManager::Instance();
  auto openIds = wm.GetOpenWindowIds();

  Rectangle listArea = GetWindowListArea();
  float itemX = listArea.x + m_style.itemSpacing;
  float itemY = listArea.y + (listArea.height - m_style.windowItemHeight) * 0.5f;

  for (int id : openIds) {
    auto *w = wm.GetWindow(id);
    if (!w)
      continue;

    Rectangle itemRect = {itemX, itemY, m_style.windowItemWidth,
                          m_style.windowItemHeight};

    // Background color
    Color bgColor = m_style.itemColor;
    if (w->IsFocused() && w->IsVisible())
      bgColor = m_style.itemActiveColor;
    else if (m_windowItemHoverId == id)
      bgColor = m_style.itemHoverColor;

    Fumbo::Graphic2D::DrawRectangleRounded(itemRect, 0.1f, 4, bgColor);

    // Window title (truncated)
    std::string title = w->GetTitle();
    if (title.length() > 16)
      title = title.substr(0, 14) + "..";

    float textItemY =
        itemRect.y + (itemRect.height - m_style.fontSize) * 0.5f;
    Fumbo::Graphic2D::DrawText(title, {itemRect.x + 8.0f, textItemY}, m_font,
                               m_style.fontSize, m_style.textColor);

    itemX += m_style.windowItemWidth + m_style.itemSpacing;
    if (itemX + m_style.windowItemWidth > listArea.x + listArea.width)
      break;
  }

  Rectangle trayArea = GetTrayArea();
  float trayX = trayArea.x + 4.0f;
  for (auto &item : m_trayItems) {
    Rectangle itemRect = {trayX, trayArea.y + 2.0f, item.width,
                          trayArea.height - 4.0f};
    if (item.drawCallback) {
      item.drawCallback(itemRect);
    }
    trayX += item.width + 2.0f;
  }

  if (m_startMenuOpen && !m_startMenuItems.empty()) {
    Rectangle menuRect = GetStartMenuRect();

    // Shadow
    Rectangle shadowRect = {menuRect.x + 3.0f, menuRect.y + 3.0f,
                            menuRect.width, menuRect.height};
    Fumbo::Graphic2D::DrawRectangleRounded(shadowRect, 0.03f, 4,
                                           {0, 0, 0, 80});

    // Background
    Fumbo::Graphic2D::DrawRectangleRounded(menuRect, 0.03f, 4,
                                           m_style.startMenuBg);
    Fumbo::Graphic2D::DrawRectangleRoundedLinesEx(
        menuRect, 0.03f, 4, 1.0f, m_style.startMenuBorder);

    // Items
    float itemMenuY = menuRect.y + 4.0f;
    for (int i = 0; i < (int)m_startMenuItems.size(); i++) {
      Rectangle itemRect = {menuRect.x + 4.0f, itemMenuY,
                            menuRect.width - 8.0f, m_style.startMenuItemHeight};

      if (i == m_startMenuHoverIndex) {
        Fumbo::Graphic2D::DrawRectangleRounded(itemRect, 0.1f, 4,
                                               m_style.startMenuItemHover);
      }

      // Icon
      float textStartX = itemRect.x + 10.0f;
      if (m_startMenuItems[i].icon.id != 0) {
        float iconSz = m_style.startMenuItemHeight - 8.0f;
        Fumbo::Graphic2D::DrawTexture(m_startMenuItems[i].icon,
                                      {itemRect.x + 6.0f, itemRect.y + 4.0f},
                                      {iconSz, iconSz});
        textStartX = itemRect.x + 6.0f + iconSz + 6.0f;
      }

      // Label
      float labelY = itemRect.y +
                     (m_style.startMenuItemHeight - m_style.startMenuFontSize) * 0.5f;
      Fumbo::Graphic2D::DrawText(m_startMenuItems[i].label,
                                 {textStartX, labelY}, m_font,
                                 m_style.startMenuFontSize, m_style.textColor);

      itemMenuY += m_style.startMenuItemHeight;
    }
  }
}

} // namespace OS
