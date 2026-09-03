#include "OperationState.h"

#include <iostream>
#include <memory>
#include <stdexcept>

#include "WorkOperation.h"

OperationState::~OperationState() {}

void OperationState::executeStep(WorkOperation& context) {
    invalidAction(context, "execute a step");
}

void OperationState::start(WorkOperation& context) {
    invalidAction(context, "start");
}

void OperationState::finish(WorkOperation& context) {
    invalidAction(context, "finish");
}

void OperationState::approveQuality(WorkOperation& context) {
    invalidAction(context, "approve quality for");
}

void OperationState::rejectQuality(WorkOperation& context) {
    invalidAction(context, "reject quality for");
}

void OperationState::invalidAction(const WorkOperation& context,
                                   const std::string& action) const {
    throw std::logic_error("Cannot " + action + " operation " +
                           context.getId() + " while it is " + name());
}

std::string QueuedState::name() const {
    return "Queued";
}

bool QueuedState::canExecuteStep() const {
    return false;
}

void QueuedState::start(WorkOperation& context) {
    std::cout << "  [state] " << context.getId()
              << ": Queued -> In Progress\n";
    context.transitionTo(
        std::unique_ptr<OperationState>(new InProgressState()));
}

std::string InProgressState::name() const {
    return "In Progress";
}

bool InProgressState::canExecuteStep() const {
    return true;
}

void InProgressState::executeStep(WorkOperation& context) {
    std::cout << "  [production] Performing " << context.getName()
              << " (" << context.plannedMinutes() << " minutes planned)\n";
}

void InProgressState::finish(WorkOperation& context) {
    std::cout << "  [state] " << context.getId()
              << ": In Progress -> Quality Check\n";
    context.transitionTo(
        std::unique_ptr<OperationState>(new QualityCheckState()));
}

std::string QualityCheckState::name() const {
    return "Quality Check";
}

bool QualityCheckState::canExecuteStep() const {
    return true;
}

void QualityCheckState::executeStep(WorkOperation& context) {
    std::cout << "  [quality] Inspecting " << context.getName()
              << " against its manufacturing tolerances\n";
}

void QualityCheckState::approveQuality(WorkOperation& context) {
    std::cout << "  [state] " << context.getId()
              << ": Quality Check -> Completed\n";
    context.transitionTo(
        std::unique_ptr<OperationState>(new CompletedState()));
}

void QualityCheckState::rejectQuality(WorkOperation& context) {
    std::cout << "  [state] " << context.getId()
              << ": Quality Check -> Rework\n";
    context.transitionTo(std::unique_ptr<OperationState>(new ReworkState()));
}

std::string ReworkState::name() const {
    return "Rework";
}

bool ReworkState::canExecuteStep() const {
    return true;
}

void ReworkState::executeStep(WorkOperation& context) {
    std::cout << "  [rework] Correcting defects in " << context.getName()
              << "\n";
}

void ReworkState::finish(WorkOperation& context) {
    std::cout << "  [state] " << context.getId()
              << ": Rework -> Quality Check\n";
    context.transitionTo(
        std::unique_ptr<OperationState>(new QualityCheckState()));
}

std::string CompletedState::name() const {
    return "Completed";
}

bool CompletedState::canExecuteStep() const {
    return false;
}
