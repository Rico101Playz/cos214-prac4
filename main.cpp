#include <iomanip>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <chrono>
#include <thread>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <conio.h>
#else
#include <cerrno>
#include <sys/select.h>
#include <termios.h>
#include <unistd.h>
#endif

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

namespace
{
    // Own terminal settings so every return (including exceptions) restores them.
    class InteractiveTerminal
    {
    public:
        InteractiveTerminal() : available_(false), colors_(false)
        {
#ifdef _WIN32
            input_ = GetStdHandle(STD_INPUT_HANDLE);
            output_ = GetStdHandle(STD_OUTPUT_HANDLE);
            DWORD inputMode;
            available_ = GetConsoleMode(input_, &inputMode) != 0;
            outputSaved_ = GetConsoleMode(output_, &outputMode_) != 0;
            if (outputSaved_)
                colors_ = SetConsoleMode(output_, outputMode_ | 0x0004) != 0;
#else
            available_ = isatty(STDIN_FILENO) &&
                         tcgetattr(STDIN_FILENO, &saved_) == 0;
            if (available_)
            {
                termios immediate = saved_;
                immediate.c_lflag &= ~(ICANON | ECHO);
                immediate.c_cc[VMIN] = 1;
                immediate.c_cc[VTIME] = 0;
                available_ = tcsetattr(STDIN_FILENO, TCSANOW, &immediate) == 0;
            }
            colors_ = isatty(STDOUT_FILENO);
#endif
        }

        ~InteractiveTerminal()
        {
#ifdef _WIN32
            if (outputSaved_)
                SetConsoleMode(output_, outputMode_);
#else
            if (available_)
                tcsetattr(STDIN_FILENO, TCSANOW, &saved_);
#endif
        }

        InteractiveTerminal(const InteractiveTerminal &) = delete;
        InteractiveTerminal &operator=(const InteractiveTerminal &) = delete;

        const char *color(const char *code) const { return colors_ ? code : ""; }

        void page(const std::string &title) const
        {
            std::cout << color("\033[2J\033[H\033[1;36m")
                      << "  +------------------------------------------------------------+\n"
                      << "  |                      T A S K F O R G E                     |\n"
                      << "  |                   PRODUCTION CONTROL                     |\n"
                      << "  +------------------------------------------------------------+\n"
                      << color("\033[0m") << "\n  " << title << "\n\n";
        }

        // -1 means timeout; -2 means unavailable input / EOF.
        int key(int timeoutMs)
        {
            if (!available_)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(timeoutMs));
                return -2;
            }
            const auto deadline = std::chrono::steady_clock::now() +
                                  std::chrono::milliseconds(timeoutMs);
            do
            {
#ifdef _WIN32
                if (_kbhit())
                {
                    int value = _getch();
                    if (value == 0 || value == 224)
                    {
                        _getch(); // Consume the second byte of a special key.
                        return 0;
                    }
                    return value;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
#else
                fd_set input;
                FD_ZERO(&input);
                FD_SET(STDIN_FILENO, &input);
                timeval interval = {0, 10000};
                int ready = select(STDIN_FILENO + 1, &input, 0, 0, &interval);
                if (ready > 0)
                {
                    unsigned char value;
                    return read(STDIN_FILENO, &value, 1) == 1 ? value : -2;
                }
                if (ready < 0 && errno != EINTR)
                    return -2;
#endif
            } while (std::chrono::steady_clock::now() < deadline);
            return -1;
        }

        int waitKey()
        {
            std::cout << std::flush;
            int value;
            do { value = key(100); } while (value == -1);
            return value;
        }

    private:
        bool available_;
        bool colors_;
#ifdef _WIN32
        HANDLE input_, output_;
        DWORD outputMode_;
        bool outputSaved_;
#else
        termios saved_;
#endif
    };

    const char *stateColor(const WorkComponent &operation)
    {
        const std::string state = operation.stateLabel();
        if (state == "Completed") return "\033[32m";
        if (state == "Rework") return "\033[31m";
        if (state == "Quality Check") return "\033[33m";
        if (state == "In Progress") return "\033[36m";
        return "\033[90m";
    }

    void productionDashboard(InteractiveTerminal &terminal)
    {
        std::unique_ptr<ProductionGroup> order;
        ProductionGroup *machining = 0, *overflow = 0, *assembly = 0;
        buildProductionOrder(order, machining, overflow, assembly,
                             "ORDER-LIVE", "Interactive Gearbox Order");
        const char *ids[] = {"OP-MILL", "OP-DRILL", "OP-DEBURR", "OP-FIT", "OP-TORQUE"};
        int selected = 0;
        std::string message = "Select an operation with 1-5, then choose an action.";
        for (;;)
        {
            terminal.page("LIVE ORDER  /  operation dashboard");
            std::cout << "  Planned work: " << order->plannedMinutes()
                      << " min    Highest priority: " << order->priority() << "\n\n";
            for (int i = 0; i < 5; ++i)
            {
                WorkComponent &operation = requireComponent(*order, ids[i]);
                std::cout << (i == selected ? " > " : "   ") << i + 1 << "  "
                          << std::left << std::setw(12) << ids[i]
                          << terminal.color(stateColor(operation))
                          << std::setw(16) << operation.stateLabel()
                          << terminal.color("\033[0m") << operation.getName() << "\n";
            }
            std::cout << terminal.color("\033[1;36m")
                      << "\n  [S] Start   [E] Execute step   [F] Finish\n"
                      << "  [A] Approve quality   [R] Reject quality\n"
                      << "  [B] Back to menu\n" << terminal.color("\033[0m")
                      << "\n  " << message << "\n\n  Action > ";
            int key = terminal.waitKey();
            if (key == -2 || key == 'b' || key == 'B') return;
            if (key >= '1' && key <= '5')
            {
                selected = key - '1';
                message = std::string("Selected ") + ids[selected];
                continue;
            }
            WorkComponent &operation = requireComponent(*order, ids[selected]);
            try
            {
                switch (key)
                {
                case 's': case 'S': operation.start(); break;
                case 'e': case 'E': operation.executeStep(); break;
                case 'f': case 'F': operation.finish(); break;
                case 'a': case 'A': operation.approveQuality(); break;
                case 'r': case 'R': operation.rejectQuality(); break;
                default: message = "Choose 1-5 or one of the action keys above."; continue;
                }
                message = operation.getId() + ": action complete; state = " + operation.stateLabel();
            }
            catch (const std::logic_error &error) { message = error.what(); }
        }
    }

    // Returning true falls through to the untouched original demonstration.
    bool interactiveStartup()
    {
        InteractiveTerminal terminal;
        const int startupWaitSeconds = 0.5;
        std::cout << terminal.color("\033[1;36m")
                  << "\n  TASKFORGE  |  Press any key within " << startupWaitSeconds
                  << " seconds for production control.\n"
                  << terminal.color("\033[0m")
                  << "  Otherwise, the original demonstration runs automatically.\n" << std::flush;
        if (terminal.key(startupWaitSeconds * 1000) < 0) return true;

        std::string message;
        for (;;)
        {
            terminal.page("CONTROL ROOM  /  choose a workspace");
            std::cout << terminal.color("\033[1;33m")
                      << "  [1] Live production dashboard\n" << terminal.color("\033[0m")
                      << "      Select operations and control their lifecycle.\n\n"
                      << "  [2] Normal gearbox production\n"
                      << "  [3] CNC disruption and recovery\n"
                      << "  [4] Rush order and assembly rework\n"
                      << "  [5] Inspect hierarchy and independent iterators\n\n"
                      << terminal.color("\033[1;36m")
                      << "  [D] Run the full original demonstration\n"
                      << "  [Q] Quit\n" << terminal.color("\033[0m")
                      << "\n  Scenarios and dashboard sessions start with a fresh order.\n"
                      << "  Press a highlighted key; no Enter needed.\n"
                      << "  " << message << "\n\n  Choice > ";
            int choice = terminal.waitKey();
            if (choice == -2 || choice == 'q' || choice == 'Q') return false;
            if (choice == 'd' || choice == 'D') return true;
            if (choice == '1') { productionDashboard(terminal); continue; }
            if (choice < '2' || choice > '5')
            {
                message = "Please choose 1-5, D or Q.";
                continue;
            }
            message.clear();
            terminal.page("SCENARIO OUTPUT");
            try
            {
                std::unique_ptr<ProductionGroup> order;
                ProductionGroup *machining = 0, *overflow = 0, *assembly = 0;
                buildProductionOrder(order, machining, overflow, assembly,
                                     "ORDER-UI", "Interactive Gearbox Order");
                if (choice == '2') runScenarioOne(*order);
                if (choice == '3') runScenarioTwo(*order, *machining, *overflow);
                if (choice == '4') runScenarioThree(*order, *assembly);
                if (choice == '5') demonstrateIndependentTraversals(*order);
                printCompleteHierarchy(*order);
            }
            catch (const std::exception &error)
            {
                std::cout << terminal.color("\033[31m") << "\n  " << error.what()
                          << terminal.color("\033[0m") << "\n";
            }
            std::cout << terminal.color("\033[1;36m") << "\n  Press any key to return to the menu."
                      << terminal.color("\033[0m") << std::flush;
            if (terminal.waitKey() == -2) return false;
        }
    }
}

// new main()
int main()
{
    try
    {
        if (!interactiveStartup())
            return 0;

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
