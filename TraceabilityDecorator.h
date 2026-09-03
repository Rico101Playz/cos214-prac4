#ifndef TASKFORGE_TRACEABILITY_DECORATOR_H
#define TASKFORGE_TRACEABILITY_DECORATOR_H

#include <exception>
#include <memory>
#include <string>
#include <vector>

#include "WorkDecorator.h"

class TraceabilityDecorator : public WorkDecorator {
public:
    TraceabilityDecorator(std::unique_ptr<WorkComponent> wrapped,
                          const std::string& stationId);
    virtual ~TraceabilityDecorator();

    virtual std::string description() const;
    virtual void executeStep();
    virtual void start();
    virtual void finish();
    virtual void approveQuality();
    virtual void rejectQuality();

    const std::vector<std::string>& getAuditTrail() const;

private:
    void record(const std::string& event);
    void recordFailure(const std::string& action, const std::exception& error);

    std::string stationId_;
    std::vector<std::string> auditTrail_;
};

#endif
