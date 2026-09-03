#include "TraceabilityDecorator.h"

#include <ctime>
#include <exception>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>

TraceabilityDecorator::TraceabilityDecorator(
    std::unique_ptr<WorkComponent> wrapped,
    const std::string& stationId)
    : WorkDecorator(std::move(wrapped)),
      stationId_(stationId),
      auditTrail_() {
    if (stationId_.empty()) {
        throw std::invalid_argument("Traceability station ID cannot be empty");
    }
}

TraceabilityDecorator::~TraceabilityDecorator() {}

std::string TraceabilityDecorator::description() const {
    return WorkDecorator::description() + " {traceable at " + stationId_ +
           "}";
}

void TraceabilityDecorator::executeStep() {
    record("executeStep requested");
    try {
        WorkDecorator::executeStep();
        record("executeStep completed");
    } catch (const std::exception& error) {
        recordFailure("executeStep", error);
        throw;
    }
}

void TraceabilityDecorator::start() {
    record("start requested");
    try {
        WorkDecorator::start();
        record("start completed; state=" + stateLabel());
    } catch (const std::exception& error) {
        recordFailure("start", error);
        throw;
    }
}

void TraceabilityDecorator::finish() {
    record("finish requested");
    try {
        WorkDecorator::finish();
        record("finish completed; state=" + stateLabel());
    } catch (const std::exception& error) {
        recordFailure("finish", error);
        throw;
    }
}

void TraceabilityDecorator::approveQuality() {
    record("approveQuality requested");
    try {
        WorkDecorator::approveQuality();
        record("approveQuality completed; state=" + stateLabel());
    } catch (const std::exception& error) {
        recordFailure("approveQuality", error);
        throw;
    }
}

void TraceabilityDecorator::rejectQuality() {
    record("rejectQuality requested");
    try {
        WorkDecorator::rejectQuality();
        record("rejectQuality completed; state=" + stateLabel());
    } catch (const std::exception& error) {
        recordFailure("rejectQuality", error);
        throw;
    }
}

const std::vector<std::string>&
TraceabilityDecorator::getAuditTrail() const {
    return auditTrail_;
}

void TraceabilityDecorator::record(const std::string& event) {
    const std::time_t now = std::time(0);
    const std::tm* local = std::localtime(&now);
    char timeBuffer[20] = {0};
    if (local != 0) {
        std::strftime(timeBuffer, sizeof(timeBuffer), "%Y-%m-%d %H:%M:%S",
                      local);
    } else {
        std::string fallback("time-unavailable");
        fallback.copy(timeBuffer, sizeof(timeBuffer) - 1U);
    }

    std::ostringstream entry;
    entry << timeBuffer << " | station=" << stationId_ << " | operation="
          << getId() << " | " << event;
    auditTrail_.push_back(entry.str());
    std::cout << "  [trace] " << entry.str() << "\n";
}

void TraceabilityDecorator::recordFailure(const std::string& action,
                                          const std::exception& error) {
    record(action + " rejected: " + error.what());
}
