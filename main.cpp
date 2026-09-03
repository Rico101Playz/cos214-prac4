#include <iomanip>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

#include "PriorityDecorator.h"
#include "ProductionGroup.h"
#include "TraceabilityDecorator.h"
#include "WorkIterator.h"
#include "WorkOperation.h"

namespace {
void heading(const std::string& title) {
    std::cout << "\n=== " << title << " ===\n";
}

void printComponent(const WorkComponent& component) {
    std::cout << "  " << std::left << std::setw(13) << component.getId()
              << " | " << std::setw(56) << component.description()
              << " | priority=" << std::setw(2) << component.priority()
              << " | " << component.stateLabel() << "\n";
}

WorkComponent& requireComponent(ProductionGroup& root,
                                const std::string& componentId) {
    WorkComponent* component = root.findById(componentId);
    if (component == 0) {
        throw std::logic_error("Required component not found: " + componentId);
    }
    return *component;
}

void demonstrateIndependentTraversals(ProductionGroup& root) {
    heading("Independent depth-first traversals");
    std::unique_ptr<WorkIterator> first = root.createDepthFirstIterator();
    std::unique_ptr<WorkIterator> second = root.createDepthFirstIterator();

    std::cout << "First iterator advances twice:\n";
    printComponent(first->next());
    printComponent(first->next());

    std::cout << "Second iterator still begins at the root:\n";
    printComponent(second->next());

    std::cout << "First iterator continues from its own cursor:\n";
    printComponent(first->next());
}

void printCompleteHierarchy(ProductionGroup& root) {
    heading("Complete depth-first hierarchy");
    std::unique_ptr<WorkIterator> iterator = root.createDepthFirstIterator();
    while (iterator->hasNext()) {
        printComponent(iterator->next());
    }
}

void executeReadyOperations(ProductionGroup& root) {
    heading("Ready-operation traversal");
    std::unique_ptr<WorkIterator> iterator =
        root.createReadyOperationIterator();
    while (iterator->hasNext()) {
        WorkComponent& operation = iterator->next();
        std::cout << "Selected " << operation.getId() << " in state "
                  << operation.stateLabel() << "\n";
        operation.executeStep();
    }
}
}

int main() {
    try {
        std::unique_ptr<ProductionGroup> order(
            new ProductionGroup("ORDER-1042", "Gearbox Order 1042",
                                GroupKind::ORDER));

        std::unique_ptr<ProductionGroup> housingPhase(
            new ProductionGroup("PHASE-HOUSING", "Housing Manufacture",
                                GroupKind::PHASE));
        std::unique_ptr<ProductionGroup> cncLine(
            new ProductionGroup("LINE-CNC", "CNC Line A", GroupKind::LINE));
        std::unique_ptr<ProductionGroup> machiningCell(
            new ProductionGroup("CELL-MACHINE", "Machining Cell",
                                GroupKind::CELL));
        std::unique_ptr<ProductionGroup> overflowCell(
            new ProductionGroup("CELL-OVERFLOW", "Overflow CNC Cell",
                                GroupKind::CELL));

        ProductionGroup* machiningCellView = machiningCell.get();
        ProductionGroup* overflowCellView = overflowCell.get();

        machiningCell->add(std::unique_ptr<WorkComponent>(
            new WorkOperation("OP-MILL", "Mill gearbox housing", 90, 3)));

        machiningCell->add(std::unique_ptr<WorkComponent>(
            new WorkOperation("OP-DRILL", "Drill mounting holes", 45, 2)));

        overflowCell->add(std::unique_ptr<WorkComponent>(
            new WorkOperation("OP-DEBURR", "Deburr housing", 25, 1)));
        cncLine->add(std::move(machiningCell));
        cncLine->add(std::move(overflowCell));
        housingPhase->add(std::move(cncLine));

        std::unique_ptr<ProductionGroup> assemblyPhase(
            new ProductionGroup("PHASE-ASSEMBLY", "Gear Assembly",
                                GroupKind::PHASE));
        std::unique_ptr<ProductionGroup> assemblyLine(
            new ProductionGroup("LINE-ASSEMBLY", "Assembly Line 2",
                                GroupKind::LINE));
        std::unique_ptr<ProductionGroup> assemblyCell(
            new ProductionGroup("CELL-ASSEMBLY", "Gear Fitment Cell",
                                GroupKind::CELL));
        assemblyCell->add(std::unique_ptr<WorkComponent>(
            new WorkOperation("OP-FIT", "Fit gear train", 70, 4)));
        assemblyCell->add(std::unique_ptr<WorkComponent>(
            new WorkOperation("OP-TORQUE", "Torque housing bolts", 30, 3)));
        assemblyLine->add(std::move(assemblyCell));
        assemblyPhase->add(std::move(assemblyLine));

        order->add(std::move(housingPhase));
        order->add(std::move(assemblyPhase));

        heading("TaskForge manufacturing model");
        std::cout << order->description() << "\n"
                  << "Total planned work: " << order->plannedMinutes()
                  << " minutes\n"
                  << "Highest effective priority: " << order->priority()
                  << "\n";

        demonstrateIndependentTraversals(*order);
        printCompleteHierarchy(*order);

        heading("Structural change and fail-fast traversal policy");
        std::unique_ptr<WorkIterator> staleIterator =
            order->createDepthFirstIterator();
        printComponent(staleIterator->next());
        std::cout << "Moving and decorating OP-DRILL at runtime\n";
        std::unique_ptr<WorkComponent> moved =
            machiningCellView->remove("OP-DRILL");
        std::unique_ptr<TraceabilityDecorator> tracedDrill(
            new TraceabilityDecorator(std::move(moved), "CNC-A-07"));
        TraceabilityDecorator* drillAuditView = tracedDrill.get();
        std::unique_ptr<WorkComponent> tracedAsComponent(std::move(tracedDrill));
        std::unique_ptr<WorkComponent> expeditedDrill(
            new PriorityDecorator(std::move(tracedAsComponent), 5));
        overflowCellView->add(std::move(expeditedDrill));
        try {
            staleIterator->hasNext();
            throw std::logic_error("Iterator invalidation was not detected");
        } catch (const std::logic_error& error) {
            std::cout << "Expected traversal result: " << error.what() << "\n";
        }

        heading("State transitions and stacked decorators");
        WorkComponent& drill = requireComponent(*order, "OP-DRILL");
        WorkComponent& mill = requireComponent(*order, "OP-MILL");

        std::cout << "Decorated operation: " << drill.description() << "\n";
        try {
            drill.approveQuality();
        } catch (const std::logic_error& error) {
            std::cout << "Expected invalid transition: " << error.what()
                      << "\n";
        }

        drill.start();
        mill.start();
        executeReadyOperations(*order);

        drill.finish();
        drill.executeStep();
        drill.rejectQuality();
        drill.executeStep();
        drill.finish();
        drill.executeStep();
        drill.approveQuality();

        try {
            drill.executeStep();
        } catch (const std::logic_error& error) {
            std::cout << "Expected completed-state rejection: " << error.what()
                      << "\n";
        }

        std::cout << "Traceability entries captured for OP-DRILL: "
                  << drillAuditView->getAuditTrail().size() << "\n";

        printCompleteHierarchy(*order);
        heading("Task 2 demonstration complete");
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "TaskForge failed: " << error.what() << "\n";
        return 1;
    }
}
