#include "PriorityDecorator.h"

#include <iostream>
#include <sstream>
#include <stdexcept>

PriorityDecorator::PriorityDecorator(
    std::unique_ptr<WorkComponent> wrapped,
    int priorityBoost)
    : WorkDecorator(std::move(wrapped)), priorityBoost_(priorityBoost) {
    if (priorityBoost_ <= 0) {
        throw std::invalid_argument("Priority boost must be positive");
    }
}

PriorityDecorator::~PriorityDecorator() {}

std::string PriorityDecorator::description() const {
    std::ostringstream output;
    output << WorkDecorator::description() << " {priority +"
           << priorityBoost_ << "}";
    return output.str();
}

int PriorityDecorator::priority() const {
    return WorkDecorator::priority() + priorityBoost_;
}

void PriorityDecorator::executeStep() {
    std::cout << "  [priority] Dispatching " << getId()
              << " with effective priority " << priority() << "\n";
    WorkDecorator::executeStep();
}
