#include <iostream>
#include <random>
#include "TrafficLight.h"

/* Implementation of class "MessageQueue" */

template <typename T>
T MessageQueue<T>::receive()
{
    // FP.5a : The method receive should use std::unique_lock<std::mutex> and _condition.wait()
    // to wait for and receive new messages and pull them from the queue using move semantics.
    // The received object should then be returned by the receive function.

    // perform _queue modification under the lock
    std::unique_lock<std::mutex> uLock(_mutex);
    _condition.wait(uLock, [this] { return !_queue.empty(); }); // pass unique lock to condition variable

    // remove queue element
    T v = std::move(_queue.back());
    _queue.pop_back();

    return v; // will not be copied due to return value optimization (RVO) in C++
}

template <typename T>
void MessageQueue<T>::send(T &&msg)
{
    // FP.4a : The method send should use the mechanisms std::lock_guard<std::mutex>
    // as well as _condition.notify_one() to add a new message to the queue and afterwards send a notification.

    // perform _queue modification under the lock
    std::lock_guard<std::mutex> uLock(_mutex);

    // clear _queue
    _queue.clear();

    // add msg to queue
    _queue.emplace_back(std::move(msg));
    _condition.notify_one();
}

/* Implementation of class "TrafficLight" */

TrafficLight::TrafficLight()
{
    //_type = ObjectType::trafficLight;
    _currentPhase = TrafficLightPhase::red;
}

void TrafficLight::waitForGreen()
{
    // FP.5b : add the implementation of the method waitForGreen, in which an infinite while-loop
    // runs and repeatedly calls the receive function on the message queue.
    // Once it receives TrafficLightPhase::green, the method returns.
    while (true)
    {
        TrafficLightPhase phase = _messageQueue.receive();
        if (phase == TrafficLightPhase::green)
        {
            break;
        }
    }    
}

TrafficLightPhase TrafficLight::getCurrentPhase()
{
    std::unique_lock<std::mutex> uLock(_mtx);
    return _currentPhase;
}

void TrafficLight::simulate()
{
    // FP.2b : Finally, the private method „cycleThroughPhases“ should be started in a thread when the public method „simulate“ is called. To do this, use the thread queue in the base class.

    // launch cycleThroughPhases function in a thread
    threads.emplace_back(std::thread(&TrafficLight::cycleThroughPhases, this));
}

// virtual function which is executed in a thread
void TrafficLight::cycleThroughPhases()
{
    // FP.2a : Implement the function with an infinite loop that measures the time between two loop cycles
    // and toggles the current phase of the traffic light between red and green and sends an update method
    // to the message queue using move semantics. The cycle duration should be a random value between 4 and 6 seconds.
    // Also, the while-loop should use std::this_thread::sleep_for to wait 1ms between two cycles.

    int targetSwitchTime = 0;
    int stopWatch = 0;

    std::random_device rd;
    std::mt19937 mt(rd());
    std::uniform_int_distribution<> distr(4000, 6000);

    while (true)
    {
        if (stopWatch >= targetSwitchTime)
        {
            // targetSwitchTime has passend and its time to compute a new targetSwitchTime
            // between 4 and 6 seconds (in milliseconds) and reset stopWatch
            targetSwitchTime = distr(mt);
            stopWatch = 0;
            
            std::unique_lock<std::mutex> uLock(_mtx);
            // toggle phase
            if (_currentPhase == TrafficLightPhase::red)
            {
                _currentPhase = TrafficLightPhase::green;
            }
            else
            {
                _currentPhase = TrafficLightPhase::red;
            }
            
            // send update message to the message queue using move semantics.
            _messageQueue.send(std::move(_currentPhase));

            uLock.unlock();
        }
        else
        {
            stopWatch++;
        }

        // sleep at every iteration to reduce CPU usage
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}
