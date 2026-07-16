#include "os_context_menu.hpp"
#include <algorithm>

namespace OS {

// Setup

void ContextMenu::SetFont(Font font) { m_font = font; }

void ContextMenu::SetStyle(const ContextMenuStyle &style) { m_style = style; }

// Menu Construction

void ContextMenu::AddItem(const std::string &label,
                          std::function<void()> callback, Texture2D icon,
                          bool enabled) {
  ContextMenuItem item;
  item.label = label;
  item.callback = std::move(callback);
  item.icon = icon;
  item.enabled = enabled;
  item.isSeparator = false;
  m_items.push_back(std::move(item));
}

void ContextMenu::AddSeparator() {
  ContextMenuItem sep;
  sep.isSeparator = true;
  m_items.push_back(sep);
}

void ContextMenu::ClearItems() { m_items.clear(); }

// Show/Hide

void ContextMenu::Show(Vector2 positionUI) {
  m_position = positionUI;
  m_open = true;
  m_hoverIndex = -1;

  // Clamp so menu stays within UI bounds
  float menuW = CalcMenuWidth();
  float menuH = CalcMenuHeight();

  if (m_position.x + menuW > Fumbo::Utils::UI_WIDTH) {
    m_position.x = Fumbo::Utils::UI_WIDTH - menuW - 4.0f;
  }
  if (m_position.y + menuH > Fumbo::Utils::UI_HEIGHT) {
    m_position.y = Fumbo::Utils::UI_HEIGHT - menuH - 4.0f;
  }
  if (m_position.x < 0)
    m_position.x = 4.0f;
  if (m_position.y < 0)
    m_position.y = 4.0f;
}

void ContextMenu::ShowAtMouse() {
  Vector2 scale = Fumbo::Utils::GetUIScale();
  Vector2 offset = Fumbo::Utils::GetUIOffset();
  Vector2 mouse = GetMousePosition();
  Vector2 mouseUI = {(mouse.x - offset.x) / scale.x,
                     (mouse.y - offset.y) / scale.y};
  Show(mouseUI);
}

void ContextMenu::Close() {
  m_open = false;
  m_hoverIndex = -1;
}

bool ContextMenu::IsOpen() const { return m_open; }

// Helpers

float ContextMenu::CalcMenuWidth() const {
  float maxW = m_style.minWidth;
  for (auto &item : m_items) {
    if (item.isSeparator)
      continue;
    // Estimate text width
    float w = item.label.length() * m_style.fontSize * 0.6f +
              m_style.padding * 2 + 20.0f;
    if (item.icon.id != 0)
      w += m_style.iconSize + 6.0f;
    if (w > maxW)
      maxW = w;
  }
  return maxW;
}

float ContextMenu::CalcMenuHeight() const {
  float h = m_style.padding * 2;
  for (auto &item : m_items) {
    h += item.isSeparator ? m_style.separatorHeight : m_style.itemHeight;
  }
  return h;
}

// Update

bool ContextMenu::Update() {
  if (!m_open)
    return false;

  Vector2 scale = Fumbo::Utils::GetUIScale();
  Vector2 offset = Fumbo::Utils::GetUIOffset();
  Vector2 mouse = GetMousePosition();
  Vector2 mouseUI = {(mouse.x - offset.x) / scale.x,
                     (mouse.y - offset.y) / scale.y};

  float menuW = CalcMenuWidth();
  float menuH = CalcMenuHeight();
  Rectangle menuRect = {m_position.x, m_position.y, menuW, menuH};

  bool overMenu = CheckCollisionPointRec(mouseUI, menuRect);

  // Find hovered item
  m_hoverIndex = -1;
  if (overMenu) {
    float y = m_position.y + m_style.padding;
    for (int i = 0; i < (int)m_items.size(); i++) {
      float itemH = m_items[i].isSeparator ? m_style.separatorHeight
                                           : m_style.itemHeight;
      if (!m_items[i].isSeparator) {
        Rectangle itemRect = {m_position.x + m_style.padding, y,
                              menuW - m_style.padding * 2, itemH};
        if (CheckCollisionPointRec(mouseUI, itemRect) && m_items[i].enabled) {
          m_hoverIndex = i;
        }
      }
      y += itemH;
    }
  }

  // Click
  if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
    if (m_hoverIndex >= 0 && m_items[m_hoverIndex].callback) {
      m_items[m_hoverIndex].callback();
      Close();
      return true;
    }
    // Click outside closes menu
    if (!overMenu) {
      Close();
      return false;
    }
  }

  // Right-click outside closes menu too
  if (IsMouseButtonPressed(MOUSE_RIGHT_BUTTON) && !overMenu) {
    Close();
    return false;
  }

  return overMenu;
}

// Draw

void ContextMenu::Draw() {
  if (!m_open || m_items.empty())
    return;

  float menuW = CalcMenuWidth();
  float menuH = CalcMenuHeight();
  Rectangle menuRect = {m_position.x, m_position.y, menuW, menuH};

  // Shadow
  Rectangle shadowRect = {menuRect.x + 3.0f, menuRect.y + 3.0f, menuRect.width,
                          menuRect.height};
  Fumbo::Graphic2D::DrawRectangleRounded(shadowRect, m_style.cornerRoundness, 4,
                                         {0, 0, 0, 80});

  // Background
  Fumbo::Graphic2D::DrawRectangleRounded(menuRect, m_style.cornerRoundness, 4,
                                         m_style.backgroundColor);

  // Border
  Fumbo::Graphic2D::DrawRectangleRoundedLinesEx(
      menuRect, m_style.cornerRoundness, 4, m_style.borderThickness,
      m_style.borderColor);

  // Items
  float y = m_position.y + m_style.padding;
  for (int i = 0; i < (int)m_items.size(); i++) {
    auto &item = m_items[i];

    if (item.isSeparator) {
      // Separator line
      float sepY = y + m_style.separatorHeight * 0.5f;
      Fumbo::Graphic2D::DrawLineEx(
          {m_position.x + m_style.padding + 4.0f, sepY},
          {m_position.x + menuW - m_style.padding - 4.0f, sepY}, 1.0f,
          m_style.separatorColor);
      y += m_style.separatorHeight;
      continue;
    }

    Rectangle itemRect = {m_position.x + m_style.padding, y,
                          menuW - m_style.padding * 2, m_style.itemHeight};

    // Hover highlight
    if (i == m_hoverIndex) {
      Fumbo::Graphic2D::DrawRectangleRounded(itemRect, 0.15f, 4,
                                             m_style.hoverColor);
    }

    float textX = itemRect.x + 8.0f;

    // Icon
    if (item.icon.id != 0) {
      float iconY = itemRect.y + (m_style.itemHeight - m_style.iconSize) * 0.5f;
      Fumbo::Graphic2D::DrawTexture(item.icon, {textX, iconY},
                                    {m_style.iconSize, m_style.iconSize});
      textX += m_style.iconSize + 6.0f;
    }

    // Label
    float labelY = itemRect.y + (m_style.itemHeight - m_style.fontSize) * 0.5f;
    Color labelColor = item.enabled ? item.textColor : m_style.disabledTextColor;
    Fumbo::Graphic2D::DrawText(item.label, {textX, labelY}, m_font,
                               m_style.fontSize, labelColor);

    y += m_style.itemHeight;
  }
}

} // namespace OS
