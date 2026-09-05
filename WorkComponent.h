#ifndef TASKFORGE_WORK_COMPONENT_H
#define TASKFORGE_WORK_COMPONENT_H

#include <memory>
#include <string>
#include <vector>

#include "StructureVersion.h"

class DepthFirstIterator;
class ProductionGroup;
class ReadyOperationIterator;
class WorkDecorator;
class WorkIterator;

class WorkComponent
{
public:
    WorkComponent(const std::string &id, const std::string &displayName);
    virtual ~WorkComponent();

    WorkComponent(const WorkComponent &) = delete;
    WorkComponent &operator=(const WorkComponent &) = delete;

    virtual const std::string &getId() const;
    virtual const std::string &getName() const;
    virtual std::string description() const = 0;
    virtual int plannedMinutes() const = 0;
    virtual int priority() const = 0;
    virtual std::string stateLabel() const = 0;
    virtual bool canExecuteStep() const = 0;

    virtual void executeStep() = 0;
    virtual void start() = 0;
    virtual void finish() = 0;
    virtual void approveQuality() = 0;
    virtual void rejectQuality() = 0;
    virtual bool isLeaf() const = 0;
    virtual WorkComponent *findById(const std::string &componentId);
    virtual const WorkComponent *findById(const std::string &componentId) const;

    std::unique_ptr<WorkIterator> createDepthFirstIterator();
    std::unique_ptr<WorkIterator> createReadyOperationIterator();

protected:
    virtual void collectDepthFirst(std::vector<WorkComponent *> &snapshot) = 0;
    virtual void collectOperationLeaves(std::vector<WorkComponent *> &snapshot) = 0;
    virtual void bindStructureVersion(
        const std::shared_ptr<StructureVersion> &version);

    const std::shared_ptr<StructureVersion> &structureVersion() const;

private:
    std::string id_;
    std::string displayName_;
    std::shared_ptr<StructureVersion> structureVersion_;

    friend class DepthFirstIterator;
    friend class ProductionGroup;
    friend class ReadyOperationIterator;
    friend class WorkDecorator;
};

#endif
