#pragma once

#include "fumbo.hpp"
#include <functional>
#include <string>
#include <vector>

// NotificationManager: Toast notifications with slide-in/out animation

namespace OS {

// Notification style
struct NotificationStyle {
  Color backgroundColor = {35, 35, 55, 240};
  Color borderColor = {80, 80, 140, 200};
  Color titleColor = WHITE;
  Color messageColor = {200, 200, 220, 255};
  Color progressColor = {80, 120, 220, 200};
  float borderThickness = 1.0f;
  float cornerRoundness = 0.08f;
  float width = 280.0f;
  float minHeight = 60.0f;
  float padding = 10.0f;
  float spacing = 8.0f; // Space between stacked notifications
  int titleFontSize = 13;
  int messageFontSize = 11;
  float animationDuration = 0.3f; // Slide in/out duration in seconds
  float marginRight = 8.0f;
  float marginTop = 8.0f;
};

// Internal notification data
struct Notification {
  std::string title;
  std::string message;
  Texture2D icon = {0};
  float duration;          // Total display time (seconds)
  float elapsed = 0.0f;    // Time elapsed
  float height = 60.0f;    // Calculated height

  // Click callback
  std::function<void()> onClick;

  // Animation state
  enum class Phase { SLIDE_IN, VISIBLE, SLIDE_OUT, DONE };
  Phase phase = Phase::SLIDE_IN;
  float animTimer = 0.0f;
};

class NotificationManager {
public:
  static NotificationManager &Instance() {
    static NotificationManager instance;
    return instance;
  }
  // Simple notification
  void Push(const std::string &title, const std::string &message,
            float duration = 4.0f);

  // With icon
  void Push(const std::string &title, const std::string &message,
            float duration, Texture2D icon);

  // With click callback
  void Push(const std::string &title, const std::string &message,
            float duration, std::function<void()> onClick,
            Texture2D icon = {0});
  void SetFont(Font font);
  void SetStyle(const NotificationStyle &style);
  void SetMaxVisible(int max);
  void ClearAll();
  // Returns true if mouse is over a notification
  bool Update();
  void Draw();

private:
  NotificationManager() = default;
  ~NotificationManager() = default;
  NotificationManager(const NotificationManager &) = delete;
  NotificationManager &operator=(const NotificationManager &) = delete;

  NotificationStyle m_style;
  Font m_font = {0};
  int m_maxVisible = 5;

  std::vector<Notification> m_notifications;

  float CalcNotificationHeight(const Notification &notif) const;
};

} // namespace OS
