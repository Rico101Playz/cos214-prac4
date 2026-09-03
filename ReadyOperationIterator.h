#ifndef TASKFORGE_READY_OPERATION_ITERATOR_H
#define TASKFORGE_READY_OPERATION_ITERATOR_H

#include <cstddef>
#include <memory>
#include <vector>

#include "StructureVersion.h"
#include "WorkIterator.h"

class WorkComponent;

class ReadyOperationIterator : public WorkIterator {
public:
    explicit ReadyOperationIterator(WorkComponent& root);
    virtual ~ReadyOperationIterator();

    virtual bool hasNext() const;
    virtual WorkComponent& next();
    virtual void reset();

private:
    void checkValid() const;
    std::size_t nextReadyIndex() const;

    std::vector<WorkComponent*> items_;
    std::size_t cursor_;
    std::shared_ptr<StructureVersion> structureVersion_;
    std::size_t expectedRevision_;
};

#endif
