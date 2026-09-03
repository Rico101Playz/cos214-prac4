#include "WorkOperation.h"

#include <sstream>
#include <stdexcept>

#include "OperationState.h"

WorkOperation::WorkOperation(const std::string& id,
                             const std::string& name,
                             int baseMinutes,
                             int basePriority)
    : WorkComponent(id, name),
      baseMinutes_(baseMinutes),
      basePriority_(basePriority),
      currentState_(new QueuedState()) {
    if (baseMinutes_ <= 0) {
        throw std::invalid_argument("Planned minutes must be positive");
    }
    if (basePriority_ < 0) {
        throw std::invalid_argument("Base priority cannot be negative");
    }
}

WorkOperation::~WorkOperation() {}

std::string WorkOperation::description() const {
    std::ostringstream output;
    output << "Operation " << getName() << " [" << stateLabel() << "]";
    return output.str();
}

int WorkOperation::plannedMinutes() const {
    return baseMinutes_;
}

int WorkOperation::priority() const {
    return basePriority_;
}

std::string WorkOperation::stateLabel() const {
    return currentState_->name();
}

bool WorkOperation::canExecuteStep() const {
    return currentState_->canExecuteStep();
}

void WorkOperation::executeStep() {
    currentState_->executeStep(*this);
}

void WorkOperation::start() {
    currentState_->start(*this);
}

void WorkOperation::finish() {
    currentState_->finish(*this);
}

void WorkOperation::approveQuality() {
    currentState_->approveQuality(*this);
}

void WorkOperation::rejectQuality() {
    currentState_->rejectQuality(*this);
}

void WorkOperation::collectDepthFirst(
    std::vector<WorkComponent*>& snapshot) {
    snapshot.push_back(this);
}

void WorkOperation::collectOperationLeaves(
    std::vector<WorkComponent*>& snapshot) {
    snapshot.push_back(this);
}

void WorkOperation::transitionTo(
    std::unique_ptr<OperationState> nextState) {
    if (!nextState.get()) {
        throw std::invalid_argument("An operation state cannot be null");
    }
    currentState_ = std::move(nextState);
}
