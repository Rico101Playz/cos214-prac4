#ifndef TASKFORGE_PRIORITY_DECORATOR_H
#define TASKFORGE_PRIORITY_DECORATOR_H

#include <memory>
#include <string>

#include "WorkDecorator.h"

class PriorityDecorator : public WorkDecorator {
public:
    PriorityDecorator(std::unique_ptr<WorkComponent> wrapped,
                      int priorityBoost);
    virtual ~PriorityDecorator();

    virtual std::string description() const;
    virtual int priority() const;
    virtual void executeStep();

private:
    int priorityBoost_;
};

#endif
