#include "simulation.hpp"
#include <string>
#include <sstream>
#include <algorithm>

namespace Game {

Simulation::Simulation() {
    // Constructor for future initialization of simulation systems
}

bool Simulation::ExecuteTurn(GameManager& gameManager) {
    // 1. Read currently selected actions from GameState
    GameState& gameState = gameManager.GetGameState();
    const ActionList& selectedActions = gameState.selectedActions;

    // Reset per-turn counters
    m_teamsDeployedThisTurn = 0;
    m_airSupportSpentThisTurn = 0;
    m_roadsClosedThisTurn = 0;

    // Capture snapshots of interest to detect changes during this window.
    m_prevThreat = gameManager.GetCurrentThreatCenter();
    m_prevComms = gameManager.GetCurrentComms();
    m_prevPeopleSaved = gameState.peopleSaved;
    m_prevCasualties = gameState.casualties;
    m_prevInfrastructureDamage = gameState.infrastructureDamage;
    m_prevSheltersOpened = gameState.sheltersOpened;
    m_prevEarlyWarningIssued = gameState.earlyWarningIssued;
    
    // DEBUG LOG: Remove in production
    // std::cout << "Simulation: Executing turn with " << selectedActions.size() << " selected actions\n";
    
    // 2. Validate that player has enough resources to perform those actions
    if (!ValidateActions(gameState)) {
        // In the future, this will provide feedback to the player about why validation failed
        // For now, just return false to indicate the turn cannot proceed
        return false;
    }
    
    // 3. PLACEHOLDER: Action processing will happen here in future implementation
    // TODO: Process each action and calculate its effects on the game state
    // TODO: Integrate ActionProcessor system here
    
    // For each action in selectedActions:
    //   - Calculate budget cost
    //   - Calculate rescue team usage
    //   - Determine effect on public trust
    //   - Generate outcomes (people saved, casualties, infrastructure damage)
    //   - Create comms reports based on action outcomes
    
    // Example future code structure:
    // ActionProcessor actionProcessor;
    // for (ActionID action : selectedActions) {
    //     ActionResult result = actionProcessor.ProcessAction(action, gameState);
    //     ApplyActionResult(result, gameState);
    // }
    
    // 4. PLACEHOLDER: Disaster progression will happen here in future implementation
    // TODO: Update threat center state based on disaster progression
    // TODO: Integrate DisasterSimulator system here
    
    // Future implementation will:
    //   - Update flood levels based on rainfall
    //   - Update wildfire danger based on temperature/humidity
    //   - Update seismic activity
    //   - Generate threat warnings
    
    // Example future code structure:
    // DisasterSimulator disasterSimulator;
    // ThreatCenterState newThreatState = disasterSimulator.AdvanceThreats(gameState.currentWindow);
    // gameManager.GetScenario().GetCurrentWindow().threatCenter = newThreatState;
    
    // 5. PLACEHOLDER: Comms report generation will happen here in future implementation
    // TODO: Generate comms reports based on action outcomes and disaster state
    // TODO: Integrate ReportGenerator system here
    
    // Future implementation will:
    //   - Generate breaking news reports for significant events
    //   - Update weather reports
    //   - Create citizen reports based on action outcomes
    //   - Generate sensor reports from threat center data
    //   - Create rescue team status reports
    //   - Update infrastructure damage reports
    
    // Example future code structure:
    // ReportGenerator reportGenerator;
    // CommsState newCommsState = reportGenerator.GenerateReports(gameState, selectedActions);
    // gameManager.GetScenario().GetCurrentWindow().comms = newCommsState;
    
    // 6. Process selected actions (updates GameState, no outcome calculations)
    ProcessActions(gameManager);

    // 7. Evaluate deterministic outcomes based on the processed actions
    EvaluateOutcomes(gameManager);

    // 8. Clear selected actions after processing
    ClearSelectedActions(gameState);
    
    // 9. Generate comms based on what changed during this window
    GenerateComms(gameManager);

    // 10. Advance to the next time window
    bool shiftContinues = AdvanceWindow(gameManager);
    
    // Return whether the shift continues (true) or has finished (false)
    return shiftContinues;
}

bool Simulation::ValidateActions(const GameState& gameState) {
    int totalBudgetCost = 0;
    int totalRescueTeamsRequired = 0;
    
    DisasterType type = DisasterType::Flood;
    if (gameState.currentShift == 2) {
        type = DisasterType::Wildfire;
    } else if (gameState.currentShift >= 3) {
        type = DisasterType::Earthquake; // Volcano
    }

    for (ActionID action : gameState.selectedActions) {
        totalBudgetCost += GetActionBudgetCost(action, type);
        totalRescueTeamsRequired += GetActionRescueTeamCost(action, type);
    }

    if (gameState.budget < totalBudgetCost) {
        return false;
    }

    if (gameState.rescueTeams < totalRescueTeamsRequired) {
        return false;
    }

    return true;
}

static bool HasAction(const GameState& gs, ActionID aid) {
    return std::find(gs.selectedActions.begin(), gs.selectedActions.end(), aid) != gs.selectedActions.end();
}

void Simulation::EvaluateOutcomes(GameManager& gameManager) {
    GameState& gs = gameManager.GetGameState();
    const ThreatCenterState& threat = gameManager.GetCurrentThreatCenter();

    // Map current shift to its respective disaster evaluation dynamically
    int shift = gs.currentShift;
    if (shift == 1 || shift == 4) {
        EvaluateFlood(gs, threat);
    } else if (shift == 2 || shift == 5) {
        EvaluateWildfire(gs, threat);
    } else {
        EvaluateEarthquake(gs, threat);
    }

    if (gs.publicTrust < 0) gs.publicTrust = 0;
    if (gs.publicTrust > 100) gs.publicTrust = 100;
}

void Simulation::EvaluateFlood(GameState& gs, const ThreatCenterState& threat) {
    const float riverDepth = threat.flood.riverDepth;
    
    bool evac = HasAction(gs, ActionID::EvacuateResidents);
    bool warning = gs.earlyWarningIssued;
    bool floodgate = gs.sheltersOpened;
    bool bridgeClosed = HasAction(gs, ActionID::CloseRoad);

    int stepCasualties = 0;
    int stepSaved = 0;
    int stepDamage = 0;

    if (evac && gs.currentWindow <= 1) {
        gs.publicTrust -= 15;
    }

    if (riverDepth > 4.5f) {
        stepCasualties += 80;
        stepDamage += 15;

        if (warning) {
            stepCasualties -= 40;
            stepSaved += 50;
        }

        if (evac) {
            stepCasualties -= 60;
            stepSaved += 150;
        }

        if (m_teamsDeployedThisTurn > 0) {
            stepCasualties -= m_teamsDeployedThisTurn * 20;
            stepSaved += m_teamsDeployedThisTurn * 30;
        }

        if (!bridgeClosed) {
            stepCasualties += 30;
            stepDamage += 10;
        } else {
            stepSaved += 20;
        }
    }

    if (riverDepth > 5.5f) {
        if (!floodgate) {
            stepCasualties += 120;
            stepDamage += 30;
            gs.publicTrust -= 15;
        } else {
            stepDamage += 10;
            stepSaved += 80;
            gs.publicTrust -= 5;
        }
    }

    if (stepCasualties < 0) stepCasualties = 0;
    gs.casualties += stepCasualties;
    gs.peopleSaved += stepSaved;
    gs.infrastructureDamage = std::min(100, gs.infrastructureDamage + stepDamage);

    if (stepCasualties > 0) {
        gs.publicTrust -= stepCasualties / 10;
    } else if (riverDepth > 4.5f) {
        gs.publicTrust += 5;
    }
}

void Simulation::EvaluateWildfire(GameState& gs, const ThreatCenterState& threat) {
    const float temp = threat.wildfire.temperature;
    const float humidity = threat.wildfire.humidity;
    
    bool warning = gs.earlyWarningIssued;
    bool evac = HasAction(gs, ActionID::EvacuateResidents);
    bool air = m_airSupportSpentThisTurn > 0;
    bool parkClosed = HasAction(gs, ActionID::CloseRoad);
    
    int stepCasualties = 0;
    int stepSaved = 0;
    int stepDamage = 0;

    if (evac && gs.currentWindow <= 1) {
        gs.publicTrust -= 10;
    }

    if (temp > 37.0f && humidity < 15.0f) {
        stepCasualties += 50;
        stepDamage += 15;

        if (warning) {
            stepCasualties -= 20;
            stepSaved += 30;
        }

        if (evac) {
            stepCasualties -= 40;
            stepSaved += 100;
        }

        if (m_teamsDeployedThisTurn > 0) {
            stepCasualties -= m_teamsDeployedThisTurn * 15;
            stepSaved += m_teamsDeployedThisTurn * 25;
            stepDamage -= m_teamsDeployedThisTurn * 5;
        }

        if (!parkClosed) {
            stepCasualties += 25;
        } else {
            stepSaved += 15;
        }
    }

    if (temp > 40.0f) {
        if (!air) {
            stepCasualties += 80;
            stepDamage += 25;
        } else {
            stepDamage += 5;
            stepSaved += 120;
            gs.publicTrust += 8;
        }
    }

    if (stepCasualties < 0) stepCasualties = 0;
    gs.casualties += stepCasualties;
    gs.peopleSaved += stepSaved;
    gs.infrastructureDamage = std::min(100, std::max(0, gs.infrastructureDamage + stepDamage));

    if (stepCasualties > 0) {
        gs.publicTrust -= stepCasualties / 8;
    } else if (temp > 37.0f) {
        gs.publicTrust += 6;
    }
}

void Simulation::EvaluateEarthquake(GameState& gs, const ThreatCenterState& threat) {
    const int tremors = threat.earthquake.tremorsPerHour;
    const float gas = threat.earthquake.gasEmission;
    
    bool warning = gs.earlyWarningIssued;
    bool evac = HasAction(gs, ActionID::EvacuateResidents);
    bool airportClosed = HasAction(gs, ActionID::CloseRoad);
    bool shelter = gs.sheltersOpened;
    
    int stepCasualties = 0;
    int stepSaved = 0;
    int stepDamage = 0;

    if (evac && gs.currentWindow <= 1) {
        gs.publicTrust -= 20;
    }

    if (tremors > 12 || gas > 250.0f) {
        stepCasualties += 70;
        stepDamage += 20;

        if (warning) {
            stepCasualties -= 30;
            stepSaved += 50;
        }

        if (evac) {
            stepCasualties -= 50;
            stepSaved += 180;
        }

        if (m_teamsDeployedThisTurn > 0) {
            stepCasualties -= m_teamsDeployedThisTurn * 25;
            stepSaved += m_teamsDeployedThisTurn * 35;
        }

        if (!airportClosed) {
            stepCasualties += 40;
            stepDamage += 15;
        } else {
            stepSaved += 30;
        }
    }

    if (tremors > 30) {
        if (!shelter) {
            stepCasualties += 90;
            stepDamage += 10;
        } else {
            stepSaved += 100;
        }
    }

    if (stepCasualties < 0) stepCasualties = 0;
    gs.casualties += stepCasualties;
    gs.peopleSaved += stepSaved;
    gs.infrastructureDamage = std::min(100, gs.infrastructureDamage + stepDamage);

    if (stepCasualties > 0) {
        gs.publicTrust -= stepCasualties / 10;
    } else if (tremors > 12) {
        gs.publicTrust += 8;
    }
}

void Simulation::ProcessActions(GameManager& gameManager) {
    GameState& gs = gameManager.GetGameState();

    DisasterType type = DisasterType::Flood;
    if (gs.currentShift == 2) {
        type = DisasterType::Wildfire;
    } else if (gs.currentShift >= 3) {
        type = DisasterType::Earthquake;
    }

    for (ActionID action : gs.selectedActions) {
        int cost = GetActionBudgetCost(action, type);
        int teams = GetActionRescueTeamCost(action, type);

        gs.budget -= cost;
        gs.rescueTeams -= teams;

        switch (action) {
            case ActionID::IssueWarning:
                gs.earlyWarningIssued = true;
                break;

            case ActionID::DeployRescue:
                m_teamsDeployedThisTurn += 1;
                break;

            case ActionID::OpenShelter:
                gs.sheltersOpened = true;
                break;

            case ActionID::CloseRoad:
                m_roadsClosedThisTurn += 1;
                break;

            case ActionID::RequestAirSupport:
                m_airSupportSpentThisTurn += 1;
                break;

            case ActionID::EvacuateResidents:
                // Evaluated in the outcomes
                break;

            default:
                break;
        }
    }
}

bool Simulation::AdvanceWindow(GameManager& gameManager) {
    GameState& gs = gameManager.GetGameState();
    // Replenish rescue teams for the next window
    gs.rescueTeams = 3;
    
    bool windowAdvanced = gameManager.NextWindow();
    return windowAdvanced;
}

void Simulation::ClearSelectedActions(GameState& gameState) {
    // Clear the selected actions to prepare for the next turn
    // In future, we may want to archive these actions for the decision log
    gameState.selectedActions.clear();
    
    // TODO: Archive actions to decision log for player review
    // DecisionLog& log = GetDecisionLog();
    // log.AddActions(gameState.selectedActions, gameState.currentWindow);
}

void Simulation::GenerateComms(GameManager& gameManager) {
    GameState& gs = gameManager.GetGameState();
    const ThreatCenterState& threat = gameManager.GetCurrentThreatCenter();
    CommsState& comms = gameManager.GetCurrentComms();

    auto nowStamp = std::string(GameState::GetWindowDisplayTime(gs.currentWindow));

    auto setReport = [&](Report& r, ReportType type, const std::string& title,
                         const std::string& preview, const std::string& full,
                         ReportPriority pri) {
        r.type = type;
        r.title = title;
        r.preview = preview;
        r.fullReport = full;
        r.priority = pri;
        r.timestamp = nowStamp;
        r.unread = true;
    };

    // SENSOR: River level increases -> update sensor report
    if (threat.flood.riverDepth > m_prevThreat.flood.riverDepth) {
        std::ostringstream pr;
        pr << "River depth increased to " << threat.flood.riverDepth << " m";
        std::ostringstream full;
        full << "Automated sensors report river depth at " << threat.flood.riverDepth << " meters, up from " << m_prevThreat.flood.riverDepth << " meters.";
        setReport(comms.sensor, ReportType::Sensor, "River sensor: rising levels", pr.str(), full.str(), ReportPriority::High);
    }

    // WEATHER: Heavy rain begins or rainfall increases
    if (threat.flood.rainfall > m_prevThreat.flood.rainfall) {
        std::ostringstream pr;
        pr << "Rainfall: " << static_cast<int>(threat.flood.rainfall) << " mm/h";
        std::ostringstream full;
        full << "Meteorological services report increased rainfall of " << static_cast<int>(threat.flood.rainfall) << " mm/h, which may worsen flooding.";
        setReport(comms.weather, ReportType::Weather, "Rain intensifies", pr.str(), full.str(), ReportPriority::Medium);
    }

    // RESCUE: Teams deployed or people saved increased
    if (m_teamsDeployedThisTurn > 0 || gs.peopleSaved > m_prevPeopleSaved) {
        int deltaSaved = gs.peopleSaved - m_prevPeopleSaved;
        std::string preview = deltaSaved > 0 ? (std::to_string(deltaSaved) + " people reported rescued this window") : "Rescue teams active in the field";
        std::ostringstream full;
        full << "Field response teams were active this window. ";
        if (deltaSaved > 0) full << deltaSaved << " people were evacuated to safety.";
        else full << "Teams focused on reconnaissance and staging for rescues.";
        setReport(comms.rescue, ReportType::Rescue, "Rescue teams operational", preview, full.str(), ReportPriority::Medium);
    }

    // INFRASTRUCTURE: Damage increases -> update infrastructure report
    if (gs.infrastructureDamage > m_prevInfrastructureDamage) {
        int delta = gs.infrastructureDamage - m_prevInfrastructureDamage;
        std::ostringstream pr;
        pr << delta << " assets impacted";
        std::ostringstream full;
        full << "Infrastructure inspections report " << delta << " additional assets impacted this window (bridges, roads, utilities).";
        setReport(comms.infrastructure, ReportType::Infrastructure, "Infrastructure damage reported", pr.str(), full.str(), ReportPriority::High);
    }

    // BREAKING NEWS: Large situation changes -> update breaking news
    bool majorCasualtyIncrease = (gs.casualties - m_prevCasualties) > 20;
    bool majorInfrastructure = (gs.infrastructureDamage - m_prevInfrastructureDamage) > 10;
    if (majorCasualtyIncrease || majorInfrastructure) {
        std::ostringstream pr;
        if (majorCasualtyIncrease) pr << "Significant casualty reports; ";
        if (majorInfrastructure) pr << "Major infrastructure failures reported.";
        setReport(comms.breakingNews, ReportType::BreakingNews, "Situation escalates", pr.str(), pr.str(), ReportPriority::Critical);
    }

    // CITIZEN: Evacuation or shelter changes
    if (gs.sheltersOpened != m_prevSheltersOpened || gs.earlyWarningIssued != m_prevEarlyWarningIssued) {
        std::ostringstream pr;
        if (gs.sheltersOpened && !m_prevSheltersOpened) pr << "Shelters opened across affected zones.";
        if (gs.earlyWarningIssued && !m_prevEarlyWarningIssued) pr << "Early warning issued to residents.";
        std::ostringstream full;
        full << "Local residents report: " << pr.str() << " Community response teams are coordinating support.";
        setReport(comms.citizen, ReportType::Citizen, "Community update", pr.str(), full.str(), ReportPriority::Medium);
    }
}

int Simulation::GetActionBudgetCost(ActionID id, DisasterType type) {
    switch (id) {
        case ActionID::IssueWarning:
            return 1000;
        case ActionID::DeployRescue:
            return 5000;
        case ActionID::OpenShelter:
            return (type == DisasterType::Flood) ? 1000 : 5000;
        case ActionID::CloseRoad:
            return (type == DisasterType::Earthquake) ? 5000 : 1000;
        case ActionID::RequestAirSupport:
            return 10000;
        case ActionID::EvacuateResidents:
            return (type == DisasterType::Wildfire) ? 5000 : 12000;
    }
    return 0;
}

int Simulation::GetActionRescueTeamCost(ActionID id, DisasterType type) {
    if (id == ActionID::DeployRescue) {
        return 1;
    }
    if (id == ActionID::EvacuateResidents) {
        return (type == DisasterType::Wildfire) ? 1 : 2;
    }
    return 0;
}

} // namespace Game