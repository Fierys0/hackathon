#pragma once

#include "fumbo.hpp"
#include <functional>
#include <string>
#include <vector>

// ContextMenu: Right-click popup menu with items, separators, and callbacks

namespace OS {

// Context menu item
struct ContextMenuItem {
  std::string label;
  Texture2D icon = {0};
  std::function<void()> callback;
  bool isSeparator = false;
  bool enabled = true;
  Color textColor = {220, 220, 240, 255};
};

// Context menu style
struct ContextMenuStyle {
  Color backgroundColor = {30, 30, 48, 245};
  Color borderColor = {70, 70, 110, 200};
  Color hoverColor = {55, 55, 100, 255};
  Color disabledTextColor = {100, 100, 120, 180};
  Color separatorColor = {60, 60, 90, 180};
  float itemHeight = 28.0f;
  float separatorHeight = 8.0f;
  float minWidth = 160.0f;
  float padding = 4.0f;
  float iconSize = 18.0f;
  float cornerRoundness = 0.05f;
  float borderThickness = 1.0f;
  int fontSize = 12;
};

class ContextMenu {
public:
  ContextMenu() = default;
  ~ContextMenu() = default;
  void SetFont(Font font);
  void SetStyle(const ContextMenuStyle &style);
  // Add an item
  void AddItem(const std::string &label, std::function<void()> callback,
               Texture2D icon = {0}, bool enabled = true);

  // Add a separator line
  void AddSeparator();

  // Clear all items
  void ClearItems();
  // Show the menu at the given UI-space position
  void Show(Vector2 positionUI);

  // Show at current mouse position (convenience)
  void ShowAtMouse();

  // Close the menu
  void Close();

  bool IsOpen() const;
  // Returns true if the menu consumed the mouse
  bool Update();
  void Draw();

private:
  ContextMenuStyle m_style;
  Font m_font = {0};

  bool m_open = false;
  Vector2 m_position = {0, 0}; // UI space position
  std::vector<ContextMenuItem> m_items;
  int m_hoverIndex = -1;

  float CalcMenuWidth() const;
  float CalcMenuHeight() const;
};

} // namespace OS
