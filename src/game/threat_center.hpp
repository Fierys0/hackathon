#pragma once

namespace Game {

// Data-only snapshot of the flood threat state.
// These values describe the current environmental conditions for the flood disaster.
struct FloodStatus {
  float riverDepth = 3.8f;   // meters
  float rainfall = 42.0f;   // mm/h
};

// Data-only snapshot of the earthquake threat state.
// These values describe the current seismic activity for the earthquake disaster.
struct EarthquakeStatus {
  int tremorsPerHour = 7;   // tremors per hour
  float gasEmission = 120.0f;  // ppm
  float groundDeformation = 5.0f; // mm
};

// Data-only snapshot of the wildfire threat state.
// These values describe the current environmental conditions for the wildfire disaster.
struct WildfireStatus {
  float temperature = 39.0f;   // degrees Celsius
  float humidity = 12.0f;      // percent
};

// Data-only container for the Threat Center view.
// This holds the current threat values that the UI can read from later.
struct ThreatCenterState {
  FloodStatus flood;
  EarthquakeStatus earthquake;
  WildfireStatus wildfire;
};

} // namespace Game
