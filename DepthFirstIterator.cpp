#include "DepthFirstIterator.h"

#include <stdexcept>

#include "WorkComponent.h"

DepthFirstIterator::DepthFirstIterator(WorkComponent& root)
    : items_(),
      cursor_(0U),
      structureVersion_(root.structureVersion()),
      expectedRevision_(structureVersion_->current()) {
    root.collectDepthFirst(items_);
}

DepthFirstIterator::~DepthFirstIterator() {}

bool DepthFirstIterator::hasNext() const {
    checkValid();
    return cursor_ < items_.size();
}

WorkComponent& DepthFirstIterator::next() {
    checkValid();
    if (cursor_ >= items_.size()) {
        throw std::out_of_range("Depth-first iterator is exhausted");
    }
    return *items_[cursor_++];
}

void DepthFirstIterator::reset() {
    checkValid();
    cursor_ = 0U;
}

void DepthFirstIterator::checkValid() const {
    if (structureVersion_->current() != expectedRevision_) {
        throw std::logic_error(
            "Depth-first iterator invalidated by a structural change");
    }
}
