#pragma once

#include <string>

namespace Game {

enum class ReportPriority {
  Info,
  Medium,
  High,
  Critical
};

enum class ReportType {
  BreakingNews,
  Weather,
  Citizen,
  Sensor,
  Rescue,
  Infrastructure
};

// Reusable data container for a single intelligence report.
// The UI reads this directly to render the card preview and the expanded detail view.
struct Report {
  ReportType type = ReportType::BreakingNews;
  std::string title;
  std::string preview;
  std::string fullReport;
  std::string timestamp;
  ReportPriority priority = ReportPriority::Info;
  bool unread = true;
};

// Fixed-slot Comms state matching the existing frontend layout.
// The six report slots are intentionally explicit so the UI can access them by slot.
struct CommsState {
  // Featured breaking news report shown in the large card.
  Report breakingNews;

  // Weather update report shown in the secondary card layout.
  Report weather;

  // Citizen report from the local population.
  Report citizen;

  // Sensor report from environmental monitoring systems.
  Report sensor;

  // Rescue report from field response teams.
  Report rescue;

  // Infrastructure report concerning city systems and critical assets.
  Report infrastructure;
};

} // namespace Game
