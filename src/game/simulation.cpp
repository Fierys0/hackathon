#include "simulation.hpp"
#include <string>
#include <sstream>

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
    // PLACEHOLDER: Basic validation that will be expanded in future implementation
    
    // In future implementation, this will:
    // - Check if budget is sufficient for all selected actions
    // - Check if rescue teams are available
    // - Validate action combinations (some actions may conflict)
    // - Check time constraints for certain actions
    // - Validate against current disaster conditions
    
    // For now, implement a simple validation:
    // 1. Check that we don't exceed maximum budget per window (placeholder value)
    const int MAX_BUDGET_PER_WINDOW = 10000;
    int estimatedCost = static_cast<int>(gameState.selectedActions.size()) * 2000; // Placeholder cost
    
    if (gameState.budget < estimatedCost) {
        // TODO: Log validation failure - insufficient budget
        return false;
    }
    
    // 2. Check that we don't exceed available rescue teams
    int rescueActionsCount = 0;
    for (ActionID action : gameState.selectedActions) {
        // In future, we'll have a mapping of which actions require rescue teams
        // For now, just count a few specific actions
        if (action == ActionID::DeployRescue || action == ActionID::RequestAirSupport) {
            rescueActionsCount++;
        }
    }
    
    if (rescueActionsCount > gameState.rescueTeams) {
        // TODO: Log validation failure - insufficient rescue teams
        return false;
    }
    
    // TODO: Add more validation checks as needed
    
    return true; // All validations passed (placeholder)
}

void Simulation::EvaluateOutcomes(GameManager& gameManager) {
    GameState& gs = gameManager.GetGameState();
    const ThreatCenterState& threat = gameManager.GetCurrentThreatCenter();
    const Scenario& scenario = gameManager.GetScenario();

    // Delegate to disaster-specific evaluators
    // Default adjustments
    switch (scenario.disasterType) {
        case DisasterType::Flood:
            {
                // Simple flood rules
                // If river is deep and no early warning issued, heavy casualties
                EvaluateFlood(gs, threat);
            }
            break;
        case DisasterType::Wildfire:
            EvaluateWildfire(gs, threat);
            break;
        case DisasterType::Earthquake:
            EvaluateEarthquake(gs, threat);
            break;
    }

    // Clamp public trust into reasonable bounds [0,100]
    if (gs.publicTrust < 0) gs.publicTrust = 0;
    if (gs.publicTrust > 100) gs.publicTrust = 100;
}

void Simulation::EvaluateFlood(GameState& gs, const ThreatCenterState& threat) {
    // Balancing knobs can be adjusted here
    const float riverDepth = threat.flood.riverDepth;

    if (riverDepth > 5.0f && !gs.earlyWarningIssued) {
        gs.casualties += 200; // major event when no warning
        gs.publicTrust -= 8;
    }

    if (gs.sheltersOpened) {
        gs.peopleSaved += 150; // shelters provide large safety
        gs.publicTrust += 3;
    }

    // Rescue teams deployed this turn save additional people
    gs.peopleSaved += m_teamsDeployedThisTurn * 20;

    // Doing nothing penalty
    if (m_teamsDeployedThisTurn == 0 && !gs.sheltersOpened && !gs.earlyWarningIssued) {
        gs.casualties += 50;
        gs.publicTrust -= 4;
    }
}

void Simulation::EvaluateWildfire(GameState& gs, const ThreatCenterState& threat) {
    // Simple wildfire rules
    if (m_teamsDeployedThisTurn > 0) {
        gs.casualties = std::max(0, gs.casualties - m_teamsDeployedThisTurn * 15);
        gs.publicTrust += 2 * m_teamsDeployedThisTurn;
    }

    if (m_airSupportSpentThisTurn > 0) {
        gs.infrastructureDamage = std::max(0, gs.infrastructureDamage - 5);
        gs.publicTrust += 1;
    }

    if (m_roadsClosedThisTurn > 0) {
        gs.casualties = std::max(0, gs.casualties - m_roadsClosedThisTurn * 5);
    }
}

void Simulation::EvaluateEarthquake(GameState& gs, const ThreatCenterState& threat) {
    // Simple earthquake rules
    if (m_teamsDeployedThisTurn > 0) {
        gs.peopleSaved += m_teamsDeployedThisTurn * 25;
        gs.publicTrust += 2 * m_teamsDeployedThisTurn;
    }

    if (gs.sheltersOpened) {
        gs.peopleSaved += 80;
        gs.publicTrust += 2;
    }

    if (!gs.earlyWarningIssued) {
        // small benefit if warning was issued
        gs.casualties += 30;
    }
}

void Simulation::ProcessActions(GameManager& gameManager) {
    GameState& gs = gameManager.GetGameState();

    // Placeholder cost constants — these will be moved to a catalog later
    const int AIR_SUPPORT_COST = 5000;

    for (ActionID action : gs.selectedActions) {
        switch (action) {
            case ActionID::IssueWarning:
                // Immediate persistent effect: mark that early warning has been issued
                gs.earlyWarningIssued = true;
                break;

            case ActionID::DeployRescue:
                // Consume one rescue team if available; otherwise skip
                if (gs.rescueTeams > 0) {
                    gs.rescueTeams -= 1;
                    m_teamsDeployedThisTurn += 1;
                } else {
                    // Not enough teams — action ignored
                }
                break;

            case ActionID::OpenShelter:
                // Persistent effect for the shift
                gs.sheltersOpened = true;
                break;

            case ActionID::CloseRoad:
                // Simple per-turn counter to indicate roads were closed this window
                m_roadsClosedThisTurn += 1;
                break;

            case ActionID::RequestAirSupport:
                // Spend budget only if available; otherwise skip
                if (gs.budget >= AIR_SUPPORT_COST) {
                    gs.budget -= AIR_SUPPORT_COST;
                    m_airSupportSpentThisTurn += 1;
                } else {
                    // Insufficient budget: action ignored
                }
                break;

            default:
                // Unknown or unhandled action — add TODOs for future action types
                // e.g., ActionID::EvacuateResidents
                break;
        }
    }
}

bool Simulation::AdvanceWindow(GameManager& gameManager) {
    // PLACEHOLDER: Window advancement with future transition logic
    
    // In future implementation, this will:
    // - Apply any time-based effects (disaster progression, public trust changes)
    // - Handle window transition animations/events
    // - Update any time-sensitive game state
    
    // For now, simply delegate to GameManager::NextWindow()
    bool windowAdvanced = gameManager.NextWindow();
    
    if (windowAdvanced) {
        // TODO: Apply time-based effects here in future
        // - Reduce budget for ongoing operations
        // - Update public trust based on time passing
        // - Progress disaster states
        
        // Example future code:
        // TimeBasedEffects timeEffects;
        // timeEffects.ApplyWindowTransition(gameManager);
    } else {
        // Shift has ended
        // TODO: Trigger shift summary and prepare for next shift
        // - Calculate final shift statistics
        // - Generate shift summary report
        // - Reset window counter for next shift
        // - Increment shift number
        
        // Example future code:
        // ShiftSummary summary = CalculateShiftSummary(gameManager);
        // ShowShiftSummaryScreen(summary);
        // PrepareForNextShift(gameManager);
    }
    
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

} // namespace Game