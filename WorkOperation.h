#ifndef TASKFORGE_WORK_OPERATION_H
#define TASKFORGE_WORK_OPERATION_H

#include <memory>
#include <string>
#include <vector>

#include "WorkComponent.h"

class CompletedState;
class InProgressState;
class OperationState;
class QualityCheckState;
class QueuedState;
class ReworkState;

class WorkOperation : public WorkComponent {
public:
    WorkOperation(const std::string& id,
                  const std::string& name,
                  int baseMinutes,
                  int basePriority);
    virtual ~WorkOperation();

    virtual std::string description() const;
    virtual int plannedMinutes() const;
    virtual int priority() const;
    virtual std::string stateLabel() const;
    virtual bool canExecuteStep() const;

    virtual void executeStep();
    virtual void start();
    virtual void finish();
    virtual void approveQuality();
    virtual void rejectQuality();

protected:
    virtual void collectDepthFirst(std::vector<WorkComponent*>& snapshot);
    virtual void collectOperationLeaves(
        std::vector<WorkComponent*>& snapshot);

private:
    void transitionTo(std::unique_ptr<OperationState> nextState);

    int baseMinutes_;
    int basePriority_;
    std::unique_ptr<OperationState> currentState_;

    friend class CompletedState;
    friend class InProgressState;
    friend class QualityCheckState;
    friend class QueuedState;
    friend class ReworkState;
};

#endif
