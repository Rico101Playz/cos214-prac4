#include "ProductionGroup.h"

#include <algorithm>
#include <sstream>
#include <stdexcept>

ProductionGroup::ProductionGroup(const std::string &id,
                                 const std::string &name,
                                 GroupKind kind)
    : WorkComponent(id, name), kind_(kind), children_() {}

ProductionGroup::~ProductionGroup() {}

void ProductionGroup::add(std::unique_ptr<WorkComponent> component)
{
    if (!component.get())
    {
        throw std::invalid_argument("Cannot add a null work component");
    }
    if (component->findById(getId()) != 0)
    {
        throw std::logic_error("Adding this component would create a cycle");
    }
    if (findById(component->getId()) != 0)
    {
        throw std::logic_error("Duplicate component ID: " + component->getId());
    }

    structureVersion()->markChanged();
    component->bindStructureVersion(structureVersion());
    children_.push_back(std::move(component));
}

std::unique_ptr<WorkComponent> ProductionGroup::remove(
    const std::string &componentId)
{
    for (std::vector<std::unique_ptr<WorkComponent>>::iterator it =
             children_.begin();
         it != children_.end();
         ++it)
    {
        if ((*it)->getId() == componentId)
        {
            structureVersion()->markChanged();
            std::unique_ptr<WorkComponent> removed = std::move(*it);
            children_.erase(it);
            return removed;
        }
    }
    throw std::out_of_range("No direct child with ID: " + componentId);
}

std::size_t ProductionGroup::childCount() const
{
    return children_.size();
}

std::string ProductionGroup::description() const
{
    std::ostringstream output;
    output << kindName(kind_) << " " << getName() << " ("
           << children_.size() << " direct children)";
    return output.str();
}

int ProductionGroup::plannedMinutes() const
{
    int total = 0;
    for (std::vector<std::unique_ptr<WorkComponent>>::const_iterator it =
             children_.begin();
         it != children_.end();
         ++it)
    {
        total += (*it)->plannedMinutes();
    }
    return total;
}

int ProductionGroup::priority() const
{
    int highest = 0;
    for (std::vector<std::unique_ptr<WorkComponent>>::const_iterator it =
             children_.begin();
         it != children_.end();
         ++it)
    {
        highest = std::max(highest, (*it)->priority());
    }
    return highest;
}

std::string ProductionGroup::stateLabel() const
{
    std::ostringstream output;
    output << "Group with " << children_.size() << " direct components";
    return output.str();
}
bool ProductionGroup::isLeaf() const
{
    return false;
}
bool ProductionGroup::canExecuteStep() const
{
    for (std::vector<std::unique_ptr<WorkComponent>>::const_iterator it =
             children_.begin();
         it != children_.end();
         ++it)
    {
        if ((*it)->canExecuteStep())
        {
            return true;
        }
    }
    return false;
}

void ProductionGroup::executeStep()
{
    if (!canExecuteStep())
    {
        throw std::logic_error("Group " + getId() +
                               " contains no executable operations");
    }
    for (std::vector<std::unique_ptr<WorkComponent>>::iterator it =
             children_.begin();
         it != children_.end();
         ++it)
    {
        if ((*it)->canExecuteStep())
        {
            (*it)->executeStep();
        }
    }
}

void ProductionGroup::start()
{
    rejectGroupLifecycleAction("start");
}

void ProductionGroup::finish()
{
    rejectGroupLifecycleAction("finish");
}

void ProductionGroup::approveQuality()
{
    rejectGroupLifecycleAction("approve quality for");
}

void ProductionGroup::rejectQuality()
{
    rejectGroupLifecycleAction("reject quality for");
}

WorkComponent *ProductionGroup::findById(const std::string &componentId)
{
    WorkComponent *self = WorkComponent::findById(componentId);
    if (self != 0)
    {
        return self;
    }
    for (std::vector<std::unique_ptr<WorkComponent>>::iterator it =
             children_.begin();
         it != children_.end();
         ++it)
    {
        WorkComponent *found = (*it)->findById(componentId);
        if (found != 0)
        {
            return found;
        }
    }
    return 0;
}

const WorkComponent *ProductionGroup::findById(
    const std::string &componentId) const
{
    const WorkComponent *self = WorkComponent::findById(componentId);
    if (self != 0)
    {
        return self;
    }
    for (std::vector<std::unique_ptr<WorkComponent>>::const_iterator it =
             children_.begin();
         it != children_.end();
         ++it)
    {
        const WorkComponent *found = (*it)->findById(componentId);
        if (found != 0)
        {
            return found;
        }
    }
    return 0;
}

void ProductionGroup::collectDepthFirst(
    std::vector<WorkComponent *> &snapshot)
{
    snapshot.push_back(this);
    for (std::vector<std::unique_ptr<WorkComponent>>::iterator it =
             children_.begin();
         it != children_.end();
         ++it)
    {
        (*it)->collectDepthFirst(snapshot);
    }
}

void ProductionGroup::collectOperationLeaves(
    std::vector<WorkComponent *> &snapshot)
{
    for (std::vector<std::unique_ptr<WorkComponent>>::iterator it =
             children_.begin();
         it != children_.end();
         ++it)
    {
        (*it)->collectOperationLeaves(snapshot);
    }
}

void ProductionGroup::bindStructureVersion(
    const std::shared_ptr<StructureVersion> &version)
{
    WorkComponent::bindStructureVersion(version);
    for (std::vector<std::unique_ptr<WorkComponent>>::iterator it =
             children_.begin();
         it != children_.end();
         ++it)
    {
        (*it)->bindStructureVersion(version);
    }
}

std::string ProductionGroup::kindName(GroupKind kind)
{
    switch (kind)
    {
    case GroupKind::ORDER:
        return "Production order";
    case GroupKind::PHASE:
        return "Production phase";
    case GroupKind::LINE:
        return "Production line";
    case GroupKind::CELL:
        return "Work cell";
    }
    return "Production group";
}

void ProductionGroup::rejectGroupLifecycleAction(
    const std::string &action) const
{
    throw std::logic_error("Cannot " + action + " group " + getId() +
                           "; target an individual operation");
}
