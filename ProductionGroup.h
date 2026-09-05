#ifndef TASKFORGE_PRODUCTION_GROUP_H
#define TASKFORGE_PRODUCTION_GROUP_H

#include <memory>
#include <string>
#include <vector>

#include "WorkComponent.h"

enum class GroupKind
{
    ORDER,
    PHASE,
    LINE,
    CELL
};

class ProductionGroup : public WorkComponent
{
public:
    ProductionGroup(const std::string &id,
                    const std::string &name,
                    GroupKind kind);
    virtual ~ProductionGroup();

    void add(std::unique_ptr<WorkComponent> component);
    std::unique_ptr<WorkComponent> remove(const std::string &componentId);
    std::size_t childCount() const;

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
    virtual void collectDepthFirst(std::vector<WorkComponent *> &snapshot);
    virtual void collectOperationLeaves(
        std::vector<WorkComponent *> &snapshot);
    virtual void bindStructureVersion(
        const std::shared_ptr<StructureVersion> &version);

private:
    static std::string kindName(GroupKind kind);
    void rejectGroupLifecycleAction(const std::string &action) const;

    GroupKind kind_;
    std::vector<std::unique_ptr<WorkComponent>> children_;
};

#endif
