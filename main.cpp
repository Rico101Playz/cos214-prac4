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

namespace
{
    void heading(const std::string &title)
    {
        // print a simple section label.
        std::cout << "\n=========================================================\n";
        std::cout << "\n"
                  << title << "\n";
        std::cout << "\n=========================================================\n";
    }

    void printComponent(const WorkComponent &component)
    {
        // render one component in the same readable format each time.
        std::cout << "  " << std::left << std::setw(16) << component.getId()
                  << " | " << std::setw(64) << component.description()
                  << " | priority=" << std::setw(2) << component.priority()
                  << " | " << component.stateLabel() << "\n";
    }

    WorkComponent &requireComponent(ProductionGroup &root,
                                    const std::string &componentId)
    {
        // fetch a component or fail fast.
        WorkComponent *component = root.findById(componentId);
        if (component == 0)
        {
            throw std::logic_error("Required component not found: " + componentId);
        }
        return *component;
    }

    void buildProductionOrder(std::unique_ptr<ProductionGroup> &order,
                              ProductionGroup *&machiningCellView,
                              ProductionGroup *&overflowCellView,
                              ProductionGroup *&assemblyCellView,
                              const std::string &orderId,
                              const std::string &orderName)
    {
        order.reset(new ProductionGroup(
            orderId,
            orderName,
            GroupKind::ORDER));

        std::unique_ptr<ProductionGroup> housingPhase(
            new ProductionGroup(
                "PHASE-HOUSING",
                "Housing Manufacture",
                GroupKind::PHASE));

        std::unique_ptr<ProductionGroup> cncLine(
            new ProductionGroup(
                "LINE-CNC",
                "CNC Line A",
                GroupKind::LINE));

        std::unique_ptr<ProductionGroup> machiningCell(
            new ProductionGroup(
                "CELL-MACHINE",
                "Machining Cell",
                GroupKind::CELL));

        std::unique_ptr<ProductionGroup> overflowCell(
            new ProductionGroup(
                "CELL-OVERFLOW",
                "Overflow CNC Cell",
                GroupKind::CELL));

        machiningCellView = machiningCell.get();
        overflowCellView = overflowCell.get();

        machiningCell->add(std::unique_ptr<WorkComponent>(
            new WorkOperation(
                "OP-MILL",
                "Mill gearbox housing",
                90,
                3)));

        machiningCell->add(std::unique_ptr<WorkComponent>(
            new WorkOperation(
                "OP-DRILL",
                "Drill mounting holes",
                45,
                2)));

        overflowCell->add(std::unique_ptr<WorkComponent>(
            new WorkOperation(
                "OP-DEBURR",
                "Deburr housing",
                25,
                1)));

        cncLine->add(std::move(machiningCell));
        cncLine->add(std::move(overflowCell));
        housingPhase->add(std::move(cncLine));

        std::unique_ptr<ProductionGroup> assemblyPhase(
            new ProductionGroup(
                "PHASE-ASSEMBLY",
                "Gear Assembly",
                GroupKind::PHASE));

        std::unique_ptr<ProductionGroup> assemblyLine(
            new ProductionGroup(
                "LINE-ASSEMBLY",
                "Assembly Line 2",
                GroupKind::LINE));

        std::unique_ptr<ProductionGroup> assemblyCell(
            new ProductionGroup(
                "CELL-ASSEMBLY",
                "Gear Fitment Cell",
                GroupKind::CELL));

        assemblyCellView = assemblyCell.get();

        assemblyCell->add(std::unique_ptr<WorkComponent>(
            new WorkOperation(
                "OP-FIT",
                "Fit gear train",
                70,
                4)));

        assemblyCell->add(std::unique_ptr<WorkComponent>(
            new WorkOperation(
                "OP-TORQUE",
                "Torque housing bolts",
                30,
                3)));

        assemblyLine->add(std::move(assemblyCell));
        assemblyPhase->add(std::move(assemblyLine));

        order->add(std::move(housingPhase));
        order->add(std::move(assemblyPhase));
    }

    void demonstrateIndependentTraversals(ProductionGroup &root)
    {
        // each iterator should keep its own position.
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

    void printCompleteHierarchy(ProductionGroup &root)
    {
        // walk the full structure and print every node.
        heading("Complete depth-first hierarchy");
        std::unique_ptr<WorkIterator> iterator = root.createDepthFirstIterator();
        while (iterator->hasNext())
        {
            printComponent(iterator->next());
        }
    }

    void executeReadyOperations(ProductionGroup &root)
    {
        // run only the operations that are ready to execute.
        heading("Ready-operation traversal");
        std::unique_ptr<WorkIterator> iterator =
            root.createReadyOperationIterator();
        while (iterator->hasNext())
        {
            WorkComponent &operation = iterator->next();
            std::cout << "Selected " << operation.getId() << " in state "
                      << operation.stateLabel() << "\n";
            operation.executeStep();
        }
    }
}

void completeOperation(WorkComponent &operation)
{
    operation.finish();

    try
    {
        operation.approveQuality();
    }
    catch (const std::logic_error &error)
    {
        std::cout << "Quality decision rejected: "
                  << error.what() << "\n";
        throw;
    }
}

// Normal Gearbox Production where nothing goes wrong
void runScenarioOne(ProductionGroup &order)
{
    heading("Scenario 1: Normal Gearbox Production");

    WorkComponent &mill = requireComponent(order, "OP-MILL");
    WorkComponent &fit = requireComponent(order, "OP-FIT");
    WorkComponent &torque = requireComponent(order, "OP-TORQUE");

    std::cout << "Starting the housing and assembly work streams.\n";

    mill.start();
    fit.start();

    std::cout << "\nReady-operation traversal dispatches executable work:\n";
    executeReadyOperations(order);

    std::cout << "\nThe completed manufacturing steps now enter quality control.\n";

    completeOperation(mill);
    completeOperation(fit);

    std::cout << "\nThe assembly line continues with the torque operation.\n";

    torque.start();
    executeReadyOperations(order);
    completeOperation(torque);

    heading("Scenario 1 result");

    printComponent(mill);
    printComponent(fit);
    printComponent(torque);

    std::cout << "\nNormal production work has completed successfully.\n";
}

// Scenario 2: Gearbox Production with CNC disruption, cell failure and reassignment
void runScenarioTwo(
    ProductionGroup &order,
    ProductionGroup &machiningCell,
    ProductionGroup &overflowCell)
{

    heading("SCENARIO 2: CNC disruption, reassignment and quality failure");

    // Normal traversal until CNC reassignment
    std::unique_ptr<WorkIterator> activeTraversal =
        order.createDepthFirstIterator();

    std::cout << "An active traversal begins inspecting the order:\n";

    printComponent(activeTraversal->next());
    printComponent(activeTraversal->next());

    std::cout
        << "\nThe mounting-hole operation is reassigned because..."
        << "\n The main CNC cell is unavailable.\n";

    std::unique_ptr<WorkComponent> moved =
        machiningCell.remove("OP-DRILL");

    // Add traceability so the decorated operation remains identifiable and auditable.
    std::unique_ptr<TraceabilityDecorator> tracedDrill(
        new TraceabilityDecorator(std::move(moved), "CNC-A-07"));

    TraceabilityDecorator *drillAuditView = tracedDrill.get();

    // Stack a priority decorator around the traceability decorator
    std::unique_ptr<WorkComponent> tracedAsComponent(
        std::move(tracedDrill));

    std::unique_ptr<WorkComponent> expeditedDrill(
        new PriorityDecorator(
            std::move(tracedAsComponent),
            5));

    overflowCell.add(std::move(expeditedDrill));

    WorkComponent &currentDrill =
        requireComponent(order, "OP-DRILL");

    std::cout
        << "  OP-DRILL was moved to the overflow CNC cell.\n";

    std::cout
        << "  Effective priority is now "
        << currentDrill.priority()
        << ".\n";

    std::cout
        << "\nChecking the traversal that was active during the reassignment:\n";

    try
    {
        activeTraversal->hasNext();

        throw std::logic_error(
            "Iterator invalidation was not detected");
    }
    catch (const std::logic_error &error)
    {
        std::cout
            << "  Expected result: "
            << error.what()
            << "\n";
    }

    std::cout
        << "\nThe reassigned operation is now prepared for expedited production:\n";

    printComponent(currentDrill);

    // Invalid lifecycle action
    std::cout
        << "\nAttempting an invalid quality approval before production begins:\n";

    try
    {
        currentDrill.approveQuality();
    }
    catch (const std::logic_error &error)
    {
        std::cout
            << "  Expected lifecycle rejection: "
            << error.what()
            << "\n";
    }

    // Decorated operation
    currentDrill.start();

    std::cout
        << "\nThe ready-operation traversal now dispatches the prioritised, traceable operation:\n";

    executeReadyOperations(order);

    // Quality control
    currentDrill.finish();

    std::cout
        << "\nQuality control rejects the drilling operation.\n";

    currentDrill.executeStep();
    currentDrill.rejectQuality();

    std::cout
        << "\nThe operation enters rework and is redispatched:\n";

    executeReadyOperations(order);

    currentDrill.finish();

    std::cout
        << "\nThe repaired operation is inspected again:\n";

    currentDrill.executeStep();
    currentDrill.approveQuality();

    // Showing that the completed operations can no longer execute
    std::cout
        << "\nAttempting to execute the completed operation:\n";

    try
    {
        currentDrill.executeStep();
    }
    catch (const std::logic_error &error)
    {
        std::cout
            << "  Expected completed-state rejection: "
            << error.what()
            << "\n";
    }

    std::cout
        << "\nTraceability recorded "
        << drillAuditView->getAuditTrail().size()
        << " events for OP-DRILL.\n";

    heading("Scenario 2 result");

    printComponent(currentDrill);

    std::cout
        << "\nThe operation has recovered from the disruption "
        << "and completed successfully.\n";
}

// Scenario 3: Rush order with defective assembly and independent production streams
void runScenarioThree(
    ProductionGroup &order,
    ProductionGroup &assemblyCell)
{

    heading("SCENARIO 3: Rush order with defective assembly");

    std::cout
        << "A customer requests this gearbox as a rush order.\n"
        << "Housing manufacture and gearbox assembly proceed as "
        << "independent production streams.\n"
        << "The assembly operation receives additional priority "
        << "and traceability responsibilities.\n";

    WorkComponent &deburr =
        requireComponent(order, "OP-DEBURR");

    // OP-FIT is removed from its cell so that runtime decorators can be applied.
    std::unique_ptr<WorkComponent> moved =
        assemblyCell.remove("OP-FIT");

    // Add traceability to the rush assembly operation.
    std::unique_ptr<TraceabilityDecorator> tracedFit(
        new TraceabilityDecorator(
            std::move(moved),
            "ASSEMBLY-B-02"));

    // Keep a non-owning reference for later audit inspection.
    TraceabilityDecorator *fitAuditView =
        tracedFit.get();

    // Stack priority on top of the traceability responsibility.
    std::unique_ptr<WorkComponent> tracedAsComponent(
        std::move(tracedFit));

    std::unique_ptr<WorkComponent> rushFit(
        new PriorityDecorator(
            std::move(tracedAsComponent),
            5));

    // Return the decorated operation to its assembly cell.
    assemblyCell.add(std::move(rushFit));

    // Retrieve the current decorated operation through the common WorkComponent abstraction.
    WorkComponent &currentFit =
        requireComponent(order, "OP-FIT");

    std::cout
        << "\nRush configuration applied:\n";

    printComponent(currentFit);

    std::cout
        << "\nStarting the independent production streams:\n";

    deburr.start();

    currentFit.start();

    std::cout
        << "\nThe ready-operation traversal dispatches executable "
        << "work from both streams:\n";

    executeReadyOperations(order);

    std::cout
        << "\nThe housing stream completes successfully while "
        << "the rush assembly proceeds to quality control.\n";

    completeOperation(deburr);

    // The rush assembly operation now enters quality control.
    currentFit.finish();

    // A defect is detected during the first inspection.
    std::cout
        << "\nQuality inspection detects a critical assembly defect "
        << "in the rush operation.\n";

    currentFit.rejectQuality();

    std::cout
        << "\nOP-FIT enters rework while retaining its rush "
        << "priority and traceability responsibilities.\n";

    executeReadyOperations(order);

    // Rework is complete and the operation returns to quality control.
    currentFit.finish();

    std::cout
        << "\nThe corrected assembly is inspected for a second time:\n";

    // Execute the second inspection.
    currentFit.executeStep();

    currentFit.approveQuality();
    WorkComponent &rushTorque = requireComponent(order, "OP-TORQUE");

    // Start the remaining assembly operation after the defective rush fitting has been successfully approved.
    rushTorque.start();
    std::cout << "\nThe remaining assembly operation is now processed:\n";
    executeReadyOperations(order);
    completeOperation(rushTorque);

    std::cout
        << "\nThe rush assembly passes quality control.\n";

    std::cout
        << "\nBoth the housing and rush assembly streams have now completed successfully.\n";

    // Show that the completed rush operation cannot execute again.
    std::cout
        << "\nAttempting to execute the completed rush operation:\n";

    try
    {
        currentFit.executeStep();
    }
    catch (const std::logic_error &error)
    {
        std::cout
            << "  Expected completed-state rejection: "
            << error.what()
            << "\n";
    }

    // Show the accumulated traceability information.
    std::cout
        << "\nTraceability entries recorded for rush OP-FIT: "
        << fitAuditView->getAuditTrail().size()
        << "\n";

    heading("Scenario 3 result");

    printComponent(deburr);
    printComponent(currentFit);
    printComponent(rushTorque);

    std::cout
        << "\nThe defective rush operation has been repaired and "
        << "the order can proceed beyond the joined production streams.\n";
}

// new main()
int main()
{
    try
    {
        std::unique_ptr<ProductionGroup> order;
        ProductionGroup *machiningCellView = 0;
        ProductionGroup *overflowCellView = 0;
        ProductionGroup *assemblyCellView = 0;

        buildProductionOrder(
            order,
            machiningCellView,
            overflowCellView,
            assemblyCellView,
            "ORDER-1042",
            "Gearbox Order 1042");

        heading("TaskForge manufacturing system");
        std::cout << "Production order: " << order->description() << "\n";
        std::cout << "Total planned work: " << order->plannedMinutes() << " minutes\n";
        std::cout << "Highest effective priority before runtime " << "changes: " << order->priority() << "\n";

        // Task2 Demonstration
        demonstrateIndependentTraversals(*order);
        printCompleteHierarchy(*order);

        // Scenario 1: Normal Production
        runScenarioOne(*order);

        // Scenario 2: Gearbox Production with CNC disruption, cell faulure and reassignment
        runScenarioTwo(
            *order,
            *machiningCellView,
            *overflowCellView);

        std::unique_ptr<ProductionGroup> rushOrder;
        ProductionGroup *rushAssemblyCellView = 0;
        ProductionGroup *rushMachiningCellView = 0;
        ProductionGroup *rushOverflowCellView = 0;

        buildProductionOrder(
            rushOrder,
            rushMachiningCellView,
            rushOverflowCellView,
            rushAssemblyCellView,
            "ORDER-RUSH-2048",
            "Rush Gearbox Order 2048");

        // Scenario 3: Rush Order with defective assembly
        runScenarioThree(
            *rushOrder,
            *rushAssemblyCellView);
        heading("Final production state");

        std::cout << "\nOriginal order after normal production and CNC recovery:\n";
        printCompleteHierarchy(*order);

        std::cout << "\nRush order after assembly recovery:\n";
        printCompleteHierarchy(*rushOrder);
        heading("Task 3 demonstration complete");

        return 0;
    }
    catch (const std::exception &error)
    {
        std::cerr
            << "TaskForge failed: " << error.what() << "\n";
        return 1;
    }
}
// old main
//  int main() {
//      try {
//          // build the order tree for the gearbox job.
//          std::unique_ptr<ProductionGroup> order(
//              new ProductionGroup("ORDER-1042", "Gearbox Order 1042",
//                                  GroupKind::ORDER));

//         std::unique_ptr<ProductionGroup> housingPhase(
//             new ProductionGroup("PHASE-HOUSING", "Housing Manufacture",
//                                 GroupKind::PHASE));
//         std::unique_ptr<ProductionGroup> cncLine(
//             new ProductionGroup("LINE-CNC", "CNC Line A", GroupKind::LINE));
//         std::unique_ptr<ProductionGroup> machiningCell(
//             new ProductionGroup("CELL-MACHINE", "Machining Cell",
//                                 GroupKind::CELL));
//         std::unique_ptr<ProductionGroup> overflowCell(
//             new ProductionGroup("CELL-OVERFLOW", "Overflow CNC Cell",
//                                 GroupKind::CELL));

//         ProductionGroup* machiningCellView = machiningCell.get();
//         ProductionGroup* overflowCellView = overflowCell.get();

//         machiningCell->add(std::unique_ptr<WorkComponent>(
//             new WorkOperation("OP-MILL", "Mill gearbox housing", 90, 3)));

//         machiningCell->add(std::unique_ptr<WorkComponent>(
//             new WorkOperation("OP-DRILL", "Drill mounting holes", 45, 2)));

//         overflowCell->add(std::unique_ptr<WorkComponent>(
//             new WorkOperation("OP-DEBURR", "Deburr housing", 25, 1)));
//         cncLine->add(std::move(machiningCell));
//         cncLine->add(std::move(overflowCell));
//         housingPhase->add(std::move(cncLine));

//         std::unique_ptr<ProductionGroup> assemblyPhase(
//             new ProductionGroup("PHASE-ASSEMBLY", "Gear Assembly",
//                                 GroupKind::PHASE));
//         std::unique_ptr<ProductionGroup> assemblyLine(
//             new ProductionGroup("LINE-ASSEMBLY", "Assembly Line 2",
//                                 GroupKind::LINE));
//         std::unique_ptr<ProductionGroup> assemblyCell(
//             new ProductionGroup("CELL-ASSEMBLY", "Gear Fitment Cell",
//                                 GroupKind::CELL));
//         assemblyCell->add(std::unique_ptr<WorkComponent>(
//             new WorkOperation("OP-FIT", "Fit gear train", 70, 4)));
//         assemblyCell->add(std::unique_ptr<WorkComponent>(
//             new WorkOperation("OP-TORQUE", "Torque housing bolts", 30, 3)));
//         assemblyLine->add(std::move(assemblyCell));
//         assemblyPhase->add(std::move(assemblyLine));

//         order->add(std::move(housingPhase));
//         order->add(std::move(assemblyPhase));

//         heading("TaskForge manufacturing model");
//         std::cout << order->description() << "\n"
//                   << "Total planned work: " << order->plannedMinutes()
//                   << " minutes\n"
//                   << "Highest effective priority: " << order->priority()
//                   << "\n";

//         demonstrateIndependentTraversals(*order);
//         printCompleteHierarchy(*order);

//         heading("Structural change and fail-fast traversal policy");
//         std::unique_ptr<WorkIterator> staleIterator =
//             order->createDepthFirstIterator();
//         printComponent(staleIterator->next());
//         std::cout << "Moving and decorating OP-DRILL at runtime\n";
//         std::unique_ptr<WorkComponent> moved =
//             machiningCellView->remove("OP-DRILL");
//         std::unique_ptr<TraceabilityDecorator> tracedDrill(
//             new TraceabilityDecorator(std::move(moved), "CNC-A-07"));
//         TraceabilityDecorator* drillAuditView = tracedDrill.get();
//         std::unique_ptr<WorkComponent> tracedAsComponent(std::move(tracedDrill));
//         std::unique_ptr<WorkComponent> expeditedDrill(
//             new PriorityDecorator(std::move(tracedAsComponent), 5));
//         overflowCellView->add(std::move(expeditedDrill));
//         try {
//             staleIterator->hasNext();
//             throw std::logic_error("Iterator invalidation was not detected");
//         } catch (const std::logic_error& error) {
//             std::cout << "Expected traversal result: " << error.what() << "\n";
//         }

//         heading("State transitions and stacked decorators");
//         WorkComponent& drill = requireComponent(*order, "OP-DRILL");
//         WorkComponent& mill = requireComponent(*order, "OP-MILL");

//         std::cout << "Decorated operation: " << drill.description() << "\n";
//         try {
//             drill.approveQuality();
//         } catch (const std::logic_error& error) {
//             std::cout << "Expected invalid transition: " << error.what()
//                       << "\n";
//         }

//         drill.start();
//         mill.start();
//         executeReadyOperations(*order);

//         drill.finish();
//         drill.executeStep();
//         drill.rejectQuality();
//         drill.executeStep();
//         drill.finish();
//         drill.executeStep();
//         drill.approveQuality();

//         try {
//             drill.executeStep();
//         } catch (const std::logic_error& error) {
//             std::cout << "Expected completed-state rejection: " << error.what()
//                       << "\n";
//         }

//         std::cout << "Traceability entries captured for OP-DRILL: "
//                   << drillAuditView->getAuditTrail().size() << "\n";

//         printCompleteHierarchy(*order);
//         heading("Task 2 demonstration complete");
//         return 0;
//     } catch (const std::exception& error) {
//         std::cerr << "TaskForge failed: " << error.what() << "\n";
//         return 1;
//     }
// }
