#ifndef TASKFORGE_DEPTH_FIRST_ITERATOR_H
#define TASKFORGE_DEPTH_FIRST_ITERATOR_H

#include <cstddef>
#include <memory>
#include <vector>

#include "StructureVersion.h"
#include "WorkIterator.h"

class WorkComponent;

class DepthFirstIterator : public WorkIterator {
public:
    explicit DepthFirstIterator(WorkComponent& root);
    virtual ~DepthFirstIterator();

    virtual bool hasNext() const;
    virtual WorkComponent& next();
    virtual void reset();

private:
    void checkValid() const;

    std::vector<WorkComponent*> items_;
    std::size_t cursor_;
    std::shared_ptr<StructureVersion> structureVersion_;
    std::size_t expectedRevision_;
};

#endif
