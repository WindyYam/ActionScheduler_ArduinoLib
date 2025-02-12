# ActionScheduler
ActionScheduler is a lightweight scheduling library for delayed function execution with an argument(can be a pointer to real data), designed for embedded systems and real-time applications. It allows you to schedule one-shot or recurring function calls with precise timing, making it useful for a variety of tasks such as timeouts, state machine transitions, and periodic sensor readings.  

The key design is to eliminate the need for declaring any struct/any registration, only the callback function itself. You are to schedule callback function directly.  
All the function events are managed in a linked list as timeline, allowing easy insertion/removal.  
Specially, you can also treat it as a queue. In this case, the argument is the queue data, the function is the function to process queue data, the delay will be 0 as we want immediate processing.  
In multithreads environment, you can also treat it as a way to synchronize function call(like a queue).  

## Key Features

- **Efficient Scheduling**: ActionScheduler uses a lightweight, low-overhead design to manage scheduled actions, ensuring minimal impact on system performance. The timeline is managed as a linked list.  
- **Flexible Scheduling**: You can schedule both one-shot and recurring actions, with the ability to specify the delay and, for recurring actions, the reload interval.  
- **Callback-based Design**: Actions are defined using function pointers, allowing you to schedule any arbitrary function as an action.  
- **Cross-Platform Compatibility**: The library is written in standard C99 and can be used on a variety of embedded platforms, including ARM Cortex-M microcontrollers.  
- **Interrupt-Safe**: The library is designed to be interrupt-safe, allowing you to schedule actions from within interrupt contexts.  
- **Flexible Time Keeping**: The library does not depend on any specific time-keeping mechanism, allowing you to integrate it with your application's existing time management system.  
- **Low Power Friendly**: The library is easy to be incorporated with low power strategy as events are scheduled as timeline nodes. You can get the time remaining to the next event that's going to happen.  

## Usage
To use the ActionScheduler library, follow these steps:  

1. Include the action_scheduler.h header file in your project. 
2. Put somewhere periodically call `ActionScheduler.proceed(timeElapsed)` to update the scheduler.  
3. At any time anywhere in your code (even in ISR), schedule your callback using the provided functions, such as `ActionScheduler.schedule()` and `ActionScheduler.scheduleReload()`. the callback should have the following signature:  
`ActionReturn_t myActionCallback(void* arg);`  
The callback function should return `ACTION_ONESHOT` if the action runs only one time, or `ACTION_RELOAD` if the action should be scheduled again after the specified reload interval.  

The ActionScheduler library can be configured by defining the following preprocessor macro before including the header file:  

`MAX_ACTION_SCHEDULER_NODES`: Specifies the maximum number of scheduled actions the library can handle. The default value is 64, but you can adjust this based on the requirements of your application.  

Here is an example:
```
#include <Arduino.h>

//Optionally config the max nodes
#define ACTION_SCHEDULER_MAX_NODES 64
#include <ActionScheduler.h>

ActionScheduler actionScheduler;

ActionReturn_t printTask(void* arg){
  Serial.println("Hello ActionScheduler!");
  return ACTION_RELOAD;
}

ActionReturn_t printTaskOneShot(void* arg){
  Serial.println("This is oneshot!");
  return ACTION_ONESHOT;
}

void setup() {
  Serial.begin(115200);
  actionScheduler.schedule(1000, printTask, NULL);
  actionScheduler.schedule(2000, printTaskOneShot, NULL);
}

void loop() {
  static uint32_t lastTime = 0;
  uint32_t nowTime = millis();
  actionScheduler.proceed(nowTime - lastTime);
  lastTime = nowTime;
}
```

## Contribution and Feedback
