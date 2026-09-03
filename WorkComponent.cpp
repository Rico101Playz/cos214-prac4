#include "WorkComponent.h"

#include "DepthFirstIterator.h"
#include "ReadyOperationIterator.h"
#include "WorkIterator.h"

WorkComponent::WorkComponent(const std::string& id,
                             const std::string& displayName)
    : id_(id),
      displayName_(displayName),
      structureVersion_(new StructureVersion()) {}

WorkComponent::~WorkComponent() {}

const std::string& WorkComponent::getId() const {
    return id_;
}

const std::string& WorkComponent::getName() const {
    return displayName_;
}

WorkComponent* WorkComponent::findById(const std::string& componentId) {
    return id_ == componentId ? this : 0;
}

const WorkComponent* WorkComponent::findById(
    const std::string& componentId) const {
    return id_ == componentId ? this : 0;
}

std::unique_ptr<WorkIterator> WorkComponent::createDepthFirstIterator() {
    return std::unique_ptr<WorkIterator>(new DepthFirstIterator(*this));
}

std::unique_ptr<WorkIterator> WorkComponent::createReadyOperationIterator() {
    return std::unique_ptr<WorkIterator>(new ReadyOperationIterator(*this));
}

void WorkComponent::bindStructureVersion(
    const std::shared_ptr<StructureVersion>& version) {
    structureVersion_ = version;
}

const std::shared_ptr<StructureVersion>&
WorkComponent::structureVersion() const {
    return structureVersion_;
}
