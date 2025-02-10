#include <iostream>
#include <set>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <atomic>
#include <chrono>
#include <string>

class Elevator {
private:
    int currentFloor;
    int totalFloors;
    std::set<int> upRequests;
    std::set<int, std::greater<int>> downRequests;
    std::atomic<bool> goingUp;
    std::atomic<bool> running;
    std::atomic<bool> emergencyStop;
    std::mutex mtx;
    std::condition_variable cv;

public:
    Elevator(int floors) : currentFloor(1), totalFloors(floors), goingUp(true), running(true), emergencyStop(false) {}

    void requestFloor(int floor) {
        std::lock_guard<std::mutex> lock(mtx);
        if (floor > 0 && floor <= totalFloors) {
            if (floor > currentFloor) {
                upRequests.insert(floor);
                std::cout << "Floor " << floor << " added to up requests.\n";
            } else if (floor < currentFloor) {
                downRequests.insert(floor);
                std::cout << "Floor " << floor << " added to down requests.\n";
            } else {
                std::cout << "Elevator is already at floor " << floor << ".\n";
            }
        } else {
            std::cout << "Invalid floor request.\n";
        }
        cv.notify_one();
    }

    void emergencyStopTrigger() {
        std::lock_guard<std::mutex> lock(mtx);
        emergencyStop = true;
        running = false;
        std::cout << "Emergency stop triggered! Elevator stopping immediately.\n";
        cv.notify_all();
    }

    void move() {
        while (running) {
            std::unique_lock<std::mutex> lock(mtx);
            cv.wait(lock, [this]() { return !upRequests.empty() || !downRequests.empty() || !running; });

            if (!running) {
                break; // Stop moving if the elevator is no longer running
            }

            if (emergencyStop) {
                std::cout << "Elevator stopped due to an emergency.\n";
                break;
            }

            if (goingUp) {
                if (upRequests.empty()) {
                    goingUp = false;
                    continue;
                }
                int nextFloor = *upRequests.begin();
                upRequests.erase(upRequests.begin());
                moveToFloor(nextFloor);
            } else {
                if (downRequests.empty()) {
                    goingUp = true;
                    continue;
                }
                int nextFloor = *downRequests.begin();
                downRequests.erase(downRequests.begin());
                moveToFloor(nextFloor);
            }
        }
        std::cout << "Elevator has stopped.\n";
    }

    void moveToFloor(int targetFloor) {
        std::cout << "Moving from floor " << currentFloor << " to floor " << targetFloor << ".\n";
        std::this_thread::sleep_for(std::chrono::seconds(1)); // Simulate time to move between floors
        currentFloor = targetFloor;
        std::cout << "Reached floor " << currentFloor << ".\n";
    }

    void currentStatus() const {
        std::cout << "Elevator is currently at floor " << currentFloor << ".\n";
        std::cout << (goingUp ? "Direction: Up\n" : "Direction: Down\n");
    }

    void resume() {
        std::lock_guard<std::mutex> lock(mtx);
        if (emergencyStop) {
            emergencyStop = false;
            running = true;
            std::cout << "Elevator resuming from emergency stop.\n";
            cv.notify_one();
        }
    }

    void stopElevator() {
        std::lock_guard<std::mutex> lock(mtx);
        running = false;
        std::cout << "Stopping elevator as requested.\n";
        cv.notify_all();
    }
};

void userInteraction(Elevator &elevator, int totalFloors) {
    std::string command;
    while (true) {
        std::cout << "\nMenu:\n";
        std::cout << "1. Request floor (request <floor>)\n";
        std::cout << "2. Check elevator status (status)\n";
        std::cout << "3. Trigger emergency stop (emergency)\n";
        std::cout << "4. Resume from emergency (resume)\n";
        std::cout << "5. Return to specific floor (return)\n";
        std::cout << "6. Quit (quit)\n";
        std::cout << "Enter command: ";
        std::cin >> command;

        if (command == "request") {
            int floor;
            std::cout << "Enter the floor number (1 to " << totalFloors << "): ";
            std::cin >> floor;
            elevator.requestFloor(floor);
        } else if (command == "status") {
            elevator.currentStatus();
        } else if (command == "emergency") {
            elevator.emergencyStopTrigger();
        } else if (command == "resume") {
            elevator.resume();
        } else if (command == "return") {
            int floor;
            std::cout << "Enter the floor number to return to (1 to " << totalFloors << "): ";
            std::cin >> floor;
            elevator.requestFloor(floor);
        } else if (command == "quit") {
            elevator.stopElevator();
            break;
        } else {
            std::cout << "Invalid command.\n";
        }
    }
}

void requestSimulation(Elevator &elevator) {
    elevator.requestFloor(4);
    std::this_thread::sleep_for(std::chrono::seconds(2));
    elevator.requestFloor(7);
    std::this_thread::sleep_for(std::chrono::seconds(3));
    elevator.requestFloor(1);
}

int main() {
    int totalFloors = 10;
    Elevator elevator(totalFloors);

    std::thread elevatorThread(&Elevator::move, &elevator);
    std::thread simulationThread(requestSimulation, std::ref(elevator));
    std::thread userThread(userInteraction, std::ref(elevator), totalFloors);

    userThread.join();
    elevatorThread.join();
    simulationThread.join();

    return 0;
}
