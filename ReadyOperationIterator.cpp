#include "ReadyOperationIterator.h"

#include <stdexcept>

#include "WorkComponent.h"

ReadyOperationIterator::ReadyOperationIterator(WorkComponent& root)
    : items_(),
      cursor_(0U),
      structureVersion_(root.structureVersion()),
      expectedRevision_(structureVersion_->current()) {
    root.collectOperationLeaves(items_);
}

ReadyOperationIterator::~ReadyOperationIterator() {}

bool ReadyOperationIterator::hasNext() const {
    checkValid();
    return nextReadyIndex() < items_.size();
}

WorkComponent& ReadyOperationIterator::next() {
    checkValid();
    const std::size_t readyIndex = nextReadyIndex();
    if (readyIndex >= items_.size()) {
        throw std::out_of_range("Ready-operation iterator is exhausted");
    }
    cursor_ = readyIndex + 1U;
    return *items_[readyIndex];
}

void ReadyOperationIterator::reset() {
    checkValid();
    cursor_ = 0U;
}

void ReadyOperationIterator::checkValid() const {
    if (structureVersion_->current() != expectedRevision_) {
        throw std::logic_error(
            "Ready-operation iterator invalidated by a structural change");
    }
}

std::size_t ReadyOperationIterator::nextReadyIndex() const {
    std::size_t index = cursor_;
    while (index < items_.size() && !items_[index]->canExecuteStep()) {
        ++index;
    }
    return index;
}
