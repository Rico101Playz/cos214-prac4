#ifndef TASKFORGE_WORK_DECORATOR_H
#define TASKFORGE_WORK_DECORATOR_H

#include <memory>
#include <string>
#include <vector>

#include "WorkComponent.h"

class WorkDecorator : public WorkComponent
{
public:
    virtual ~WorkDecorator();

    virtual const std::string &getId() const;
    virtual const std::string &getName() const;
    virtual std::string description() const;
    virtual int plannedMinutes() const;
    virtual int priority() const;
    virtual std::string stateLabel() const;
    virtual bool canExecuteStep() const;
    virtual bool isLeaf() const;
    virtual void executeStep();
    virtual void start();
    virtual void finish();
    virtual void approveQuality();
    virtual void rejectQuality();

    virtual WorkComponent *findById(const std::string &componentId);
    virtual const WorkComponent *findById(
        const std::string &componentId) const;

protected:
    explicit WorkDecorator(std::unique_ptr<WorkComponent> wrapped);

    WorkComponent &wrapped();
    const WorkComponent &wrapped() const;

    virtual void collectDepthFirst(std::vector<WorkComponent *> &snapshot);
    virtual void collectOperationLeaves(
        std::vector<WorkComponent *> &snapshot);
    virtual void bindStructureVersion(
        const std::shared_ptr<StructureVersion> &version);

private:
    std::unique_ptr<WorkComponent> wrapped_;
};

#endif
