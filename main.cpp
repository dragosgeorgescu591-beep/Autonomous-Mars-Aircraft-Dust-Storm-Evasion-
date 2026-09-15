#include <iostream>

// 1. Data Structures
struct Position {
    float x;
    float y;
};

struct TelemetryData {
    float battery_pct;
    Position position;
};

struct StormData {
    float eta_minutes;
};

struct Route {
    bool is_possible;
    float cost_pct;
    Position waypoint;
};

struct LandingSite {
    bool is_found;
    float cost_pct;
    Position coordinates;
};

// 2. Interface for Map Lookups
class MapService {
public:
    virtual Route get_detour_route(const Position& pos, const StormData& storm) = 0;
    virtual LandingSite find_safe_site(const Position& pos, float max_time_mins) = 0;
    virtual ~MapService() = default;
};

enum class FlightAction {
    BYPASS,
    SAFE_LANDING,
    EMERGENCY_ABORT
};

struct ExecutionPlan {
    FlightAction action;
    Position target_coordinates;
};

// 3. Core Decision Module
class StormDecisionModule {
private:
    const float BATTERY_RESERVE_PCT = 20.0f; // Minimum safety margin
    const float LANDING_BUFFER_MINS = 2.0f;  // Buffer before storm impact

public:
    ExecutionPlan evaluate_hazard(const TelemetryData& status, const StormData& storm, MapService& map) {
        float remaining_battery = status.battery_pct;
        float time_to_storm = storm.eta_minutes;

        // Tier 1: Try Lateral Bypass
        Route detour = map.get_detour_route(status.position, storm);
        if (detour.is_possible && (remaining_battery - detour.cost_pct) >= BATTERY_RESERVE_PCT) {
            return ExecutionPlan{ FlightAction::BYPASS, detour.waypoint };
        }

        // Tier 2: Try Preemptive Safe Landing
        float max_flight_time = time_to_storm - LANDING_BUFFER_MINS;
        if (max_flight_time > 0.0f) {
            LandingSite safe_site = map.find_safe_site(status.position, max_flight_time);
            if (safe_site.is_found && (remaining_battery - safe_site.cost_pct) >= BATTERY_RESERVE_PCT) {
                return ExecutionPlan{ FlightAction::SAFE_LANDING, safe_site.coordinates };
            }
        }

        // Tier 3: Emergency Abort (Touchdown immediately)
        return ExecutionPlan{ FlightAction::EMERGENCY_ABORT, status.position };
    }
};

// 4. Mock Map Service for Testing
class MockMapService : public MapService {
public:
    Route get_detour_route(const Position& pos, const StormData& storm) override {
        // Detour takes 45% battery
        return Route{ true, 45.0f, Position{ 10.0f, 20.0f } };
    }

    LandingSite find_safe_site(const Position& pos, float max_time_mins) override {
        // Safe site found nearby requiring 15% battery
        return LandingSite{ true, 15.0f, Position{ 4.0f, 7.0f } };
    }
};

// 5. Execution Entry Point
int main() {
    StormDecisionModule engine;
    MockMapService map_service;

    // Test scenario: Battery 50%, storm 12 mins out
    // Detour costs 45% (50 - 45 = 5% left < 20% reserve) -> Rejected
    // Safe site costs 15% (50 - 15 = 35% left >= 20% reserve) -> Selected (SAFE_LANDING)
    TelemetryData current_status = { 50.0f, Position{ 0.0f, 0.0f } };
    StormData incoming_storm = { 12.0f };

    ExecutionPlan plan = engine.evaluate_hazard(current_status, incoming_storm, map_service);

    std::cout << "--- Decision Engine Output ---\n";
    if (plan.action == FlightAction::BYPASS) {
        std::cout << "Action: Lateral Bypass\n";
    } else if (plan.action == FlightAction::SAFE_LANDING) {
        std::cout << "Action: Safe Landing at (" 
                  << plan.target_coordinates.x << ", " 
                  << plan.target_coordinates.y << ")\n";
    } else {
        std::cout << "Action: Emergency Abort\n";
    }

    return 0;
}
