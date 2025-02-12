#ifndef ACTION_SCHEDULER_H
#define ACTION_SCHEDULER_H

#include <Arduino.h>

// Configuration option - can be overridden in Arduino IDE
#ifndef ACTION_SCHEDULER_MAX_NODES
#define ACTION_SCHEDULER_MAX_NODES 64U
#endif

#if ACTION_SCHEDULER_MAX_NODES > 255
#error ACTION_SCHEDULER_MAX_NODES cannot exceed 255! For now
#endif

#define ACTION_SCHEDULER_ID_INVALID 0U

typedef enum {
    ACTION_ONESHOT,
    ACTION_RELOAD
} ActionReturn_t;

// The return value indicates if you want to schedule again after finish, in same interval
typedef ActionReturn_t (*ActionCallback_t)(void* arg);
typedef uint16_t ActionSchedulerId_t;

class ActionScheduler {
public:
    ActionScheduler();
    
    bool proceed(uint32_t timeElapsedMs);
	// All Schedule function can be used from interrupt handler as well
    ActionSchedulerId_t schedule(uint32_t delayedTime, ActionCallback_t cb, void* arg);
    ActionSchedulerId_t scheduleReload(uint32_t delayedTime, uint32_t reload, ActionCallback_t cb, void* arg);
    bool unschedule(ActionSchedulerId_t* actionId);
    bool unscheduleAll(ActionCallback_t cb);
    void clear(void);
    uint32_t getNextEventDelay(void);
    uint32_t getProceedingTime(void);
    void clearProceedingTime(void);
    bool isCallbackArmed(ActionCallback_t cb);
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