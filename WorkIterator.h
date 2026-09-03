#ifndef TASKFORGE_WORK_ITERATOR_H
#define TASKFORGE_WORK_ITERATOR_H

class WorkComponent;

class WorkIterator {
public:
    virtual ~WorkIterator() {}

    virtual bool hasNext() const = 0;
    virtual WorkComponent& next() = 0;
    virtual void reset() = 0;
};

#endif
