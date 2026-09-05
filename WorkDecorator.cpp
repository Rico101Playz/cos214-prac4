#include "WorkDecorator.h"

#include <stdexcept>

namespace
{
    std::string wrappedId(const std::unique_ptr<WorkComponent> &wrapped)
    {
        if (!wrapped.get())
        {
            throw std::invalid_argument("A decorator cannot wrap null");
        }
        return wrapped->getId();
    }

    std::string wrappedName(const std::unique_ptr<WorkComponent> &wrapped)
    {
        if (!wrapped.get())
        {
            throw std::invalid_argument("A decorator cannot wrap null");
        }
        return wrapped->getName();
    }
}
namespace
{
    void requireLeaf(const std::unique_ptr<WorkComponent> &wrapped)
    {
        if (!wrapped->isLeaf())
        {
            throw std::invalid_argument(
                "A decorator may only wrap an individual work item, not a "
                "group; decorate the group's members instead");
        }
    }
}
WorkDecorator::WorkDecorator(std::unique_ptr<WorkComponent> wrapped)
    : WorkComponent(wrappedId(wrapped), wrappedName(wrapped)),
      wrapped_((requireLeaf(wrapped), std::move(wrapped))) {}

WorkDecorator::~WorkDecorator() {}

const std::string &WorkDecorator::getId() const
{
    return wrapped_->getId();
}

const std::string &WorkDecorator::getName() const
{
    return wrapped_->getName();
}

std::string WorkDecorator::description() const
{
    return wrapped_->description();
}

int WorkDecorator::plannedMinutes() const
{
    return wrapped_->plannedMinutes();
}

int WorkDecorator::priority() const
{
    return wrapped_->priority();
}

std::string WorkDecorator::stateLabel() const
{
    return wrapped_->stateLabel();
}

bool WorkDecorator::canExecuteStep() const
{
    return wrapped_->canExecuteStep();
}
bool WorkDecorator::isLeaf() const
{
    return wrapped_->isLeaf();
}
void WorkDecorator::executeStep()
{
    wrapped_->executeStep();
}

void WorkDecorator::start()
{
    wrapped_->start();
}

void WorkDecorator::finish()
{
    wrapped_->finish();
}

void WorkDecorator::approveQuality()
{
    wrapped_->approveQuality();
}

void WorkDecorator::rejectQuality()
{
    wrapped_->rejectQuality();
}

WorkComponent *WorkDecorator::findById(const std::string &componentId)
{
    if (getId() == componentId)
    {
        return this;
    }
    return wrapped_->findById(componentId);
}

const WorkComponent *WorkDecorator::findById(
    const std::string &componentId) const
{
    if (getId() == componentId)
    {
        return this;
    }
    return wrapped_->findById(componentId);
}

WorkComponent &WorkDecorator::wrapped()
{
    return *wrapped_;
}

const WorkComponent &WorkDecorator::wrapped() const
{
    return *wrapped_;
}

void WorkDecorator::collectDepthFirst(
    std::vector<WorkComponent *> &snapshot)
{
    snapshot.push_back(this);
}

void WorkDecorator::collectOperationLeaves(
    std::vector<WorkComponent *> &snapshot)
{
    snapshot.push_back(this);
}

void WorkDecorator::bindStructureVersion(
    const std::shared_ptr<StructureVersion> &version)
{
    WorkComponent::bindStructureVersion(version);
    wrapped_->bindStructureVersion(version);
}
