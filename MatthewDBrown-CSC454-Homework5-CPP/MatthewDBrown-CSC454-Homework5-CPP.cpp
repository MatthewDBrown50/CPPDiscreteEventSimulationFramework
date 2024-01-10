#include <iostream>
#include <deque>
#include <map>
#include <sstream>
#include <limits>
#include <cmath>
#include <tuple>
#include <algorithm>

//---------------------------------------------------
// FRAMEWORK
//---------------------------------------------------

class Time {
private:
    double r;
    int c;

public:
    Time(double r = 0.0, int c = 0) : r(r), c(c) {}

    double getR() const {
        return r;
    }

    int getC() const {
        return c;
    }

    int compareTo(const Time& other){
        int rComparison = std::signbit(this->r - other.r) ? -1 : (this->r == other.r ? 0 : 1);

        if (rComparison == 0) {
            return this->c - other.c;
        }

        return rComparison;
    }

    bool operator==(const Time& other) const {
        return this->r == other.r && this->c == other.c;
    }

    std::size_t hashCode() const {
        return std::hash<double>{}(r) ^ (std::hash<int>{}(c) << 1);
    }
};

class SimulationModel {
public:
    virtual std::string lambda() = 0;
    virtual void deltaInt(double timeElapsed) = 0;
    virtual void deltaExt(std::string input, double timeElapsed) = 0;
    virtual void deltaCon(std::string input, double timeElapsed) = 0;

    virtual double getNextInternalEvent() = 0;

    virtual ~SimulationModel() = default;
};

class Event {
private:
    Time time;
    SimulationModel* model;
    std::string input;
    std::string type;

public:
    Event(const std::string& input, const Time& time, SimulationModel* model, const std::string& type)
        : input(input), time(time), model(model), type(type) {}

    const Time& getTime() const {
        return time;
    }

    SimulationModel* getModel() const {
        return model;
    }

    const std::string& getInput() const {
        return input;
    }

    const std::string& getType() const {
        return type;
    }

    int compareTo(const Event& other) {
        // Compare based on the time of the events
        return this->time.compareTo(other.getTime());
    }
};

class InternalEvent : public Event {
public:
    InternalEvent(const std::string& input, const Time& time, SimulationModel* model)
        : Event(input, time, model, "internal") {}
};

class ExternalEvent : public Event {
public:
    ExternalEvent(const std::string& input, const Time& time, SimulationModel* model)
        : Event(input, time, model, "external") {}
};

class ConfluentEvent : public Event {
public:
    ConfluentEvent(const std::string& input, const Time& time, SimulationModel* model)
        : Event(input, time, model, "confluent") {}
};

class EventQueue {
private:
    std::deque<Event*> queue;

public:
    EventQueue() = default;

    void scheduleInternalEvent(double r, SimulationModel* model) {
        for (int i = 0; i < queue.size(); i++) {
            Event* existingEvent = queue[i];

            if (existingEvent->getTime().getR() == r) {
                if (existingEvent->getModel() == model) {
                    if (existingEvent->getType() == "external") {
                        scheduleConfluentEvent(existingEvent, model, r, i);
                    }
                    return;
                }

                int element = i;
                bool eventHasBeenScheduled = false;

                if (element == queue.size() - 1) {
                    Time eventTime(r, 0);
                    Event* newEvent = new InternalEvent("", eventTime, model);
                    queue.push_back(newEvent);
                }

                while (!eventHasBeenScheduled && element < (queue.size() - 1)) {
                    if (existingEvent->getTime().getR() == r) {
                        if (existingEvent->getModel() == model) {
                            if (existingEvent->getType() == "external") {
                                scheduleConfluentEvent(existingEvent, model, r, i);
                            }
                            return;
                        }

                        element++;
                        existingEvent = queue[element];
                    }
                    else {
                        double lastEventR = queue[element - 1]->getTime().getR();
                        int lastEventC = queue[element - 1]->getTime().getC();
                        int c = (lastEventR == r) ? lastEventC + 1 : 0;
                        Time eventTime(r, c);
                        Event* newEvent = new InternalEvent("", eventTime, model);
                        queue.insert(queue.begin() + element, newEvent);
                        eventHasBeenScheduled = true;
                    }
                }

                return;
            }
            else if (existingEvent->getTime().getR() > r) {
                Time eventTime(r, 0);
                Event* newEvent = new InternalEvent("", eventTime, model);
                queue.insert(queue.begin() + i, newEvent);
                return;
            }
        }

        Time eventTime(r, 0);
        Event* newEvent = new InternalEvent("", eventTime, model);
        queue.push_back(newEvent);
    }

    void scheduleConfluentEvent(Event* existingEvent, SimulationModel* model, double r, int i) {
        int c = existingEvent->getTime().getC();
        Time eventTime(r, c);
        Event* newEvent = new ConfluentEvent(existingEvent->getInput(), eventTime, model);
        queue[i] = newEvent;
    }

    void scheduleExternalEvent(const std::string& input, double r, SimulationModel* model) {
        Event* newEvent;
        Time eventTime;

        for (int i = 0; i < queue.size(); i++) {
            Event* existingEvent = queue[i];

            if (existingEvent->getTime().getR() == r) {
                int element = i;
                bool eventHasBeenScheduled = false;

                while (!eventHasBeenScheduled) {
                    if (existingEvent->getType() == "internal") {
                        if (existingEvent->getModel() == model) {
                            int c = existingEvent->getTime().getC();
                            eventTime = Time(r, c);
                            newEvent = new ConfluentEvent(input, eventTime, model);
                            queue[i] = newEvent;
                            eventHasBeenScheduled = true;
                        }
                        else {
                            element++;
                            existingEvent = queue[element];
                        }
                    }
                    else {
                        element++;
                        if (queue[element]->getTime().getR() == r) {
                            existingEvent = queue[element];
                        }
                        else {
                            int c = existingEvent->getTime().getC() + 1;
                            eventTime = Time(r, c);
                            newEvent = new ExternalEvent(input, eventTime, model);
                            queue.insert(queue.begin() + element, newEvent);
                            eventHasBeenScheduled = true;
                        }
                    }
                }

                return;
            }
            else if (existingEvent->getTime().getR() > r) {
                eventTime = Time(r, 0);
                newEvent = new ExternalEvent(input, eventTime, model);
                queue.insert(queue.begin() + i, newEvent);
                return;
            }
        }

        eventTime = Time(r, 0);
        newEvent = new ExternalEvent(input, eventTime, model);
        queue.push_back(newEvent);
    }

    std::deque<Event*> getNextEvents() {
        if (queue.empty()) {
            return std::deque<Event*>();
        }

        std::deque<Event*> events;

        double r = queue[0]->getTime().getR();
        double nextR = r;

        while (nextR == r) {
            events.push_back(queue[0]);
            queue.pop_front();

            nextR = queue.empty() ? -1.0 : queue[0]->getTime().getR();
        }

        return events;
    }

    double timeAdvance() const {
        return queue[0]->getTime().getR();
    }

    bool isEmpty() const {
        return queue.empty();
    }
};

class Simulator {
private:
    EventQueue queue;
    std::map<double, std::string> inputs;
    std::map<SimulationModel*, std::string> models;
    std::map<SimulationModel*, SimulationModel*> couplings;

public:
    Simulator() = default;

    void scheduleEvents() {
        SimulationModel* inputModel = couplings[nullptr];

        for (const auto& input : inputs) {
            queue.scheduleExternalEvent(input.second, input.first, inputModel);
        }
    }

    void addInput(const std::string& input, double r) {
        inputs[r] = input;
    }

    void addModel(SimulationModel* m) {
        models[m] = "";
    }

    void addCoupling(SimulationModel* m1, SimulationModel* m2) {
        couplings[m1] = m2;
    }

    void routeInputTo(SimulationModel* m) {
        couplings[nullptr] = m;
    }

    void takeOutputFrom(SimulationModel* m) {
        couplings[m] = nullptr;
    }

    void clearOutputs() {
        for (auto& model : models) {
            model.second = "";
        }
    }

    std::string simulate() {
        scheduleEvents();

        std::deque<Event*> events;

        std::stringstream sb;

        while (!queue.isEmpty()) {
            double r = queue.timeAdvance();

            events = queue.getNextEvents();

            clearOutputs();

            for (Event* event : events) {
                if (event->getType() != "external") {
                    SimulationModel* model = event->getModel();

                    std::string output = model->lambda();

                    models[model] = output;
                }
            }

            for (const auto& model : models) {
                std::string output = model.second;

                if (!output.empty()) {
                    if (couplings[model.first] == nullptr) {
                        sb << r << " - " << output << "\n";
                    }
                    else {
                        SimulationModel* destinationModel = couplings[model.first];

                        queue.scheduleExternalEvent(output, r, destinationModel);
                    }
                }
            }

            for (Event* event : events) {
                if (event->getType() == "internal") {
                    event->getModel()->deltaInt(event->getTime().getR());
                }
                else if (event->getType() == "external") {
                    event->getModel()->deltaExt(event->getInput(), event->getTime().getR());
                }
                else if (event->getType() == "confluent") {
                    event->getModel()->deltaCon(event->getInput(), event->getTime().getR());
                }

                double nextInternalEvent = event->getModel()->getNextInternalEvent();

                if (nextInternalEvent < std::numeric_limits<double>::infinity()) {
                    queue.scheduleInternalEvent(nextInternalEvent, event->getModel());
                }
            }
        }

        return sb.str();
    }
};

//---------------------------------------------------
// EXAMPLE MODEL
//---------------------------------------------------

class Machine : public SimulationModel {
private:
    int parts;
    double nextInternalEvent;
    const int timeToProcess;

public:
    Machine(int timeToProcess) : timeToProcess(timeToProcess), parts(0), nextInternalEvent(std::numeric_limits<double>::infinity()) {}

    std::string lambda() override {
        return "1";
    }

    void deltaInt(double timeElapsed) override {
        parts--;

        if (parts > 0) {
            nextInternalEvent = timeElapsed + timeToProcess;
        }
        else {
            nextInternalEvent = std::numeric_limits<double>::infinity();
        }
    }

    void deltaExt(std::string input, double timeElapsed) override {
        int originalParts = parts;

        parts += std::stoi(input);

        if (parts == 0) {
            nextInternalEvent = std::numeric_limits<double>::infinity();
        }
        else if (originalParts == 0 && parts > 0) {
            nextInternalEvent = timeElapsed + timeToProcess;
        }
    }

    void deltaCon(std::string input, double timeElapsed) override {
        int originalParts = parts;

        parts += std::stoi(input) - 1;

        if (parts == 0) {
            nextInternalEvent = std::numeric_limits<double>::infinity();
        }
        else if (originalParts == 0 && parts > 0) {
            nextInternalEvent = timeElapsed + timeToProcess;
        }
    }

    double getNextInternalEvent() override {
        return nextInternalEvent;
    }
};

class Drill : public Machine {
public:
    Drill() : Machine(2) {}

    std::string lambda() override {
        return "1 part completed";
    }
};

class Press : public Machine {
public:
    Press() : Machine(1) {}
};

int main() {
    Simulator sim;

    Press p;
    Drill d;

    sim.addModel(&p);
    sim.addModel(&d);

    sim.addCoupling(&p, &d);

    sim.routeInputTo(&p);
    sim.takeOutputFrom(&d);

    sim.addInput("12", 1.5);
    sim.addInput("2", 2.7);

    std::string output = sim.simulate();

    std::cout << output << std::endl;

    return 0;
}
