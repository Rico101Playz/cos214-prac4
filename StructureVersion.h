#ifndef TASKFORGE_STRUCTURE_VERSION_H
#define TASKFORGE_STRUCTURE_VERSION_H

#include <cstddef>

class StructureVersion {
public:
    StructureVersion() : revision_(0U) {}

    std::size_t current() const {
        return revision_;
    }

    void markChanged() {
        ++revision_;
    }

private:
    std::size_t revision_;
};

#endif
