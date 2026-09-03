#ifndef TASKFORGE_OPERATION_STATE_H
#define TASKFORGE_OPERATION_STATE_H

#include <string>

class WorkOperation;

class OperationState {
public:
    virtual ~OperationState();

    virtual std::string name() const = 0;
    virtual bool canExecuteStep() const = 0;

    virtual void executeStep(WorkOperation& context);
    virtual void start(WorkOperation& context);
    virtual void finish(WorkOperation& context);
    virtual void approveQuality(WorkOperation& context);
    virtual void rejectQuality(WorkOperation& context);

protected:
    void invalidAction(const WorkOperation& context,
                       const std::string& action) const;
};

class QueuedState : public OperationState {
public:
    virtual std::string name() const;
    virtual bool canExecuteStep() const;
    virtual void start(WorkOperation& context);
};

class InProgressState : public OperationState {
public:
    virtual std::string name() const;
    virtual bool canExecuteStep() const;
    virtual void executeStep(WorkOperation& context);
    virtual void finish(WorkOperation& context);
};

class QualityCheckState : public OperationState {
public:
    virtual std::string name() const;
    virtual bool canExecuteStep() const;
    virtual void executeStep(WorkOperation& context);
    virtual void approveQuality(WorkOperation& context);
    virtual void rejectQuality(WorkOperation& context);
};

class ReworkState : public OperationState {
public:
    virtual std::string name() const;
    virtual bool canExecuteStep() const;
    virtual void executeStep(WorkOperation& context);
    virtual void finish(WorkOperation& context);
};

class CompletedState : public OperationState {
public:
    virtual std::string name() const;
    virtual bool canExecuteStep() const;
};

#endif
