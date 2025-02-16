/**
 * @file ActionScheduler.h
 * @brief A timeline-based scheduler for managing delayed and periodic function calls.
 * @author windy.yam0@gmail.com
 * (Generated using Claude Sonnet)
 * This module provides a convenient way to schedule delayed or periodic function calls
 * using a statically allocated linked list structure. It's designed to be efficient
 * and suitable for embedded systems, particularly Arduino applications.
 *
 * Key features:
 * - Timeline-based linked list management
 * - Static memory allocation (no heap usage)
 * - Support for one-shot and periodic events
 * - ISR-safe operation
 * - Support for event callback chains
 * - Compatible with low-power operations
 */

#ifndef ACTION_SCHEDULER_H
#define ACTION_SCHEDULER_H

#include <Arduino.h>

/**
 * @brief Maximum number of nodes that can be scheduled simultaneously
 * @note Can be overridden in Arduino IDE, must not exceed 255
 */
#ifndef ACTION_SCHEDULER_MAX_NODES
#define ACTION_SCHEDULER_MAX_NODES 64U
#endif

#if ACTION_SCHEDULER_MAX_NODES > 255
#error ACTION_SCHEDULER_MAX_NODES cannot exceed 255! For now
#endif

/**
 * @brief Invalid scheduler ID value
 */
#define ACTION_SCHEDULER_ID_INVALID 0U

/**
 * @brief Return type for action callbacks indicating if the action should be reloaded
 */
typedef enum {
    ACTION_ONESHOT,  /**< Action executes once and is then removed */
    ACTION_RELOAD    /**< Action is rescheduled after execution */
} ActionReturn_t;

/**
 * @brief Function pointer type for action callbacks
 * @param arg Pointer to user-defined argument data
 * @return ActionReturn_t indicating if the action should be reloaded
 */
typedef ActionReturn_t (*ActionCallback_t)(void* arg);

/**
 * @brief Type definition for scheduler action IDs
 */
typedef uint16_t ActionSchedulerId_t;

/**
 * @class ActionScheduler
 * @brief Manages scheduled actions in a timeline-based linked list
 *
 * This class implements a scheduler that can manage multiple delayed or periodic
 * function calls. Events are stored in a timeline-based linked list, where earlier
 * events are closer to the head and later events are closer to the tail.
 */
class ActionScheduler {
public:
    /**
     * @brief Constructs a new ActionScheduler instance
     *
     * Initializes an empty scheduler with no active nodes.
     */
    ActionScheduler();
    
    /**
     * @brief Processes elapsed time and executes due callbacks
     * @param timeElapsedMs Time elapsed since last proceed call in milliseconds
     * @return true if any callbacks were executed, false otherwise
     *
     * Updates the scheduler's timeline by the specified amount of time and
     * executes any callbacks that are due. Callbacks are executed with
     * interrupts enabled to allow for callback chains.
     */
    bool proceed(uint32_t timeElapsedMs);

    /**
     * @brief Schedules a one-shot action
     * @param delayedTime Delay before execution in milliseconds
     * @param cb Callback function to execute
     * @param arg User data to pass to callback
     * @return ActionSchedulerId_t Unique ID for the scheduled action, or ACTION_SCHEDULER_ID_INVALID if scheduling failed
     *
     * Schedules a callback to be executed once after the specified delay.
     * Can be safely called from interrupt handlers.
     */
    ActionSchedulerId_t schedule(uint32_t delayedTime, ActionCallback_t cb, void* arg);

    /**
     * @brief Schedules a periodic action
     * @param delayedTime Initial delay before first execution in milliseconds
     * @param reload Period for subsequent executions in milliseconds
     * @param cb Callback function to execute
     * @param arg User data to pass to callback
     * @return ActionSchedulerId_t Unique ID for the scheduled action, or ACTION_SCHEDULER_ID_INVALID if scheduling failed
     *
     * Schedules a callback to be executed repeatedly with the specified period.
     * Can be safely called from interrupt handlers.
     */
    ActionSchedulerId_t scheduleReload(uint32_t delayedTime, uint32_t reload, ActionCallback_t cb, void* arg);

    /**
     * @brief Cancels a scheduled action
     * @param actionId Pointer to the action ID to unschedule
     * @return true if action was successfully unscheduled, false otherwise
     *
     * Removes the specified action from the scheduler. The action ID is
     * invalidated if the unschedule is successful.
     */
    bool unschedule(ActionSchedulerId_t* actionId);

    /**
     * @brief Cancels all actions with the specified callback
     * @param cb Callback function to unschedule
     * @return true if any actions were unscheduled, false otherwise
     *
     * Removes all actions that use the specified callback function.
     */
    bool unscheduleAll(ActionCallback_t cb);

    /**
     * @brief Removes all scheduled actions
     *
     * Clears the scheduler, removing all pending actions and resetting
     * all internal state.
     */
    void clear(void);

    /**
     * @brief Gets the delay until the next scheduled event
     * @return Delay in milliseconds until next event, or UINT32_MAX if no events are scheduled
     *
     * Returns the time remaining until the next scheduled action will execute.
     */
    uint32_t getNextEventDelay(void);

    /**
     * @brief Gets the total time processed by the scheduler
     * @return Total processed time in milliseconds
     *
     * Returns the cumulative time that has been processed through proceed() calls.
     */
    uint32_t getProceedingTime(void);

    /**
     * @brief Resets the proceeding time counter to zero
     */
    void clearProceedingTime(void);

    /**
     * @brief Checks if a callback is currently scheduled
     * @param cb Callback function to check
     * @return true if the callback is scheduled, false otherwise
     *
     * Determines if the specified callback function is currently scheduled
     * for execution.
     */
    bool isCallbackArmed(ActionCallback_t cb);

    /**
     * @brief Gets the maximum number of simultaneously active nodes
     * @return Maximum number of active nodes since last clear
     *
     * Returns the high water mark for number of simultaneously scheduled
     * actions since the last clear() call.
     */
    uint16_t getActiveNodesWaterMark(void);

private:
    typedef struct {
        ActionCallback_t callback;
        uint32_t delayToPrevious;
        uint32_t reload;
        void* arg;
        uint8_t usedCounter;
        uint8_t previousNodeIdx;
        uint8_t nextNodeIdx;
    } ActionNode_t;

    ActionNode_t mNodes[ACTION_SCHEDULER_MAX_NODES];
    uint8_t mNodeStartIdx;
    uint8_t mNodeEndIdx;
    uint16_t mActiveNodes;
    uint32_t mProceedingTime;
    uint16_t mActiveNodesWaterMark;

    bool getFreeSlot(uint8_t* slotIdx);
    uint16_t generateActionIdAt(uint8_t idx);
    void removeNodeAt(uint8_t idx);
    void insertNode(uint8_t idx, uint32_t delay);
};

#endif /* ACTION_SCHEDULER_H */