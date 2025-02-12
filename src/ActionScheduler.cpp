//
// Author: windy.yam0@gmail.com
//
// The main purpose of this module is to provide a convenient way of scheduling a delayed function call or periodic function call
// This module use a timeline based linked list to manage the schedule function calls. The linked list is statically allocated in an array without heap involved
// The earlier node will always be closer to the (logical) head, later node to the (logical) tail
// New events will be inserted into timeline based on the relative time
// Thus, the efficiency is guaranteed as we don't need to travse the whole linked list every time we proceed the time
// Periodic events is inserted the same as one shot event except it will reload a new event after previous one expires
// Event callback chain is also supported, means you can schedule another one or multiple new events at the end of one event callback
// This module can be used on ISR as well to push a function call to run out of ISR context
// This module is also useful to cooperate with low power usage as everything is managed in timeline, thus also the sleep time is determined
// A schedule is simply an invocation Schedule, no any configuration needed, no static data needed, just the function, the arg and the delay
// The arg is a void pointer which normally is 4 bytes, that you can cast into anything like integer, float, bool, or generically a pointer to your data
// For unschedule, the safety is enforced by local counter and magic number, so unscheduling an expired event won't cause too much trouble at the moment
//
#include "ActionScheduler.h"

ActionScheduler::ActionScheduler() 
    : mNodeStartIdx(0)
    , mNodeEndIdx(0)
    , mActiveNodes(0)
    , mProceedingTime(0)
    , mActiveNodesWaterMark(0)
{
    clear();
}

bool ActionScheduler::getFreeSlot(uint8_t* slotIdx) {
    bool ret = false;
    //find next available slot starting from the end
    for (uint8_t i = (mNodeEndIdx + 1U) % ACTION_SCHEDULER_MAX_NODES; 
         i != mNodeEndIdx; 
         i = (i + 1U) % ACTION_SCHEDULER_MAX_NODES)
    {
        if (mNodes[i].callback == NULL)
        {
            *slotIdx = i;
            ret = true;
            break;
        }
    }
    return ret;
}

uint16_t ActionScheduler::generateActionIdAt(uint8_t idx) {
    return idx | ((uint16_t)mNodes[idx].usedCounter << 8);
}

void ActionScheduler::removeNodeAt(uint8_t idx) {
    if(idx < ACTION_SCHEDULER_MAX_NODES)
    {
        mNodes[idx].callback = NULL;
        if (mActiveNodes > 1U)
        {
            if (idx == mNodeStartIdx)
            {
                uint8_t nextCursor = mNodes[idx].nextNodeIdx;
                mNodes[nextCursor].previousNodeIdx = nextCursor;
                mActiveNodes -= 1U;
                uint32_t timeleft = mNodes[mNodeStartIdx].delayToPrevious;
                mNodeStartIdx = nextCursor;
                mNodes[mNodeStartIdx].delayToPrevious += timeleft;
            }
            else if (idx == mNodeEndIdx)
            {
                uint8_t previousCursor = mNodes[idx].previousNodeIdx;
                mNodes[previousCursor].nextNodeIdx = previousCursor;
                mNodeEndIdx = previousCursor;
                mActiveNodes -= 1U;
            }
            else
            {
                if((mNodes[idx].previousNodeIdx == idx) && (mNodes[idx].nextNodeIdx == idx))
                {
                    // this is not the start node nor the end node, but its next and previous are itself
                    // means this is an isolated node not in the timeline
                    return;
                }
                uint8_t previousCursor = mNodes[idx].previousNodeIdx;
                uint8_t nextCursor = mNodes[idx].nextNodeIdx;
                mNodes[previousCursor].nextNodeIdx = nextCursor;
                mNodes[nextCursor].previousNodeIdx = previousCursor;
                mNodes[nextCursor].delayToPrevious += mNodes[idx].delayToPrevious;
                mActiveNodes -= 1U;
            }
        }
        else if (mActiveNodes == 1U)
        {
            if(idx != mNodeStartIdx)
            {
                // This is an isolated node
                return;
            }
            //only the head node
            mActiveNodes = 0;
            mNodeStartIdx = idx;
            mNodeEndIdx = idx;
        }
    }
}

void ActionScheduler::insertNode(uint8_t idx, uint32_t delay) {
    int16_t idxA = -1, idxB = (int16_t)mNodeStartIdx;
    //find the correct location for the new node in the linked list, starting from first node
    while (mNodes[idxB].delayToPrevious <= delay)
    {
        delay = delay - mNodes[idxB].delayToPrevious;
        idxA = idxB;
        if (idxB == (int16_t)mNodeEndIdx) //end
        {
            idxB = -1;
            break;
        }
        else
        {
            idxB = (int16_t)mNodes[idxB].nextNodeIdx;
        }
    }
    mNodes[idx].delayToPrevious = delay;
    // Insert node
    if (idxA < 0)
    {
        //this means node should be inserted only before idxB, and in this situation idxB is the old start
        mNodes[idx].previousNodeIdx = idx;
        mNodes[idx].nextNodeIdx = (uint8_t)idxB;
        mNodes[idxB].previousNodeIdx = idx;
        mNodes[idxB].delayToPrevious = mNodes[idxB].delayToPrevious - mNodes[idx].delayToPrevious;
        mNodeStartIdx = idx;
    }
    else if (idxB < 0)
    {
        //this means node should be inserted only after idxA, and in this situation idxA is the old end
        mNodes[idx].previousNodeIdx = (uint8_t)idxA;
        mNodes[idx].nextNodeIdx = idx; //set it to self as the end
        mNodes[idxA].nextNodeIdx = idx;
        mNodeEndIdx = idx;
    }
    else
    {
        //normal insertion between 2 nodes
        mNodes[idx].previousNodeIdx = (uint8_t)idxA;
        mNodes[idx].nextNodeIdx = (uint8_t)idxB;
        mNodes[idxA].nextNodeIdx = idx;
        mNodes[idxB].previousNodeIdx = idx;
        mNodes[idxB].delayToPrevious -= mNodes[idx].delayToPrevious;
    }
}

bool ActionScheduler::proceed(uint32_t timeElapsedMs) {
    bool ret = false;
    noInterrupts(); // Critical section begin

    while ((timeElapsedMs >= mNodes[mNodeStartIdx].delayToPrevious) && (mActiveNodes > 0U))
    {
        timeElapsedMs -= mNodes[mNodeStartIdx].delayToPrevious;
        mProceedingTime += mNodes[mNodeStartIdx].delayToPrevious;
        uint8_t currentCursor = mNodeStartIdx;
        mActiveNodes -= 1U;
        ActionCallback_t cb = mNodes[currentCursor].callback;
        void* arg = mNodes[currentCursor].arg;
        if (mActiveNodes > 0U)
        {
            // isolate the node out from the timeline
            uint8_t nextCursor = mNodes[currentCursor].nextNodeIdx;
            mNodes[nextCursor].previousNodeIdx = nextCursor;
            mNodeStartIdx = nextCursor;
            mNodes[currentCursor].nextNodeIdx = currentCursor;
        }
        else
        {
            mNodes[currentCursor].nextNodeIdx = currentCursor;
        }
        // This whole function should be inside the lock, but here we need to unlock for the callback chain
        interrupts(); // Allow interrupts during callback
        ActionReturn_t actionRet = cb(arg);
        noInterrupts(); // Re-enter critical section
        switch(actionRet)
        {
            case ACTION_ONESHOT:
                mNodes[currentCursor].callback = NULL;
                break;
            case ACTION_RELOAD:
                // The callback can unschedule this, result in callback changed to null, we need to check this
                if(mNodes[currentCursor].callback != NULL)
                {
                    if (mActiveNodes == 0U) //the linked list is empty, this is the first node
                    {
                        mNodes[currentCursor].delayToPrevious = mNodes[currentCursor].reload;
                    }
                    else
                    {
                        insertNode(currentCursor, mNodes[currentCursor].reload);
                    }
                    mActiveNodes++;
                }
                break;
            default:
                // Nothing
                break;
        }
        ret = true;
    }

    if (mActiveNodes > 0U)
    {
        mNodes[mNodeStartIdx].delayToPrevious -= timeElapsedMs;
        mProceedingTime += timeElapsedMs;
    }
    
    interrupts(); // Critical section end
    return ret;
}

ActionSchedulerId_t ActionScheduler::scheduleReload(uint32_t delayedTime, uint32_t reload, ActionCallback_t cb, void* arg) {
    uint16_t ActionSchedulerId = ACTION_SCHEDULER_ID_INVALID;
    if ((cb == NULL) || (mActiveNodes >= ACTION_SCHEDULER_MAX_NODES))
    {
        return ActionSchedulerId;
    }
    
    noInterrupts(); // Critical section begin
    
    if (mActiveNodes == 0U) //the linked list is empty, this is the first node
    {
        uint8_t freeCursor = mNodeStartIdx;
        if(!getFreeSlot(&freeCursor))
        {
            interrupts();
            return ACTION_SCHEDULER_ID_INVALID;
        }
        mNodes[freeCursor].usedCounter++;
        mNodes[freeCursor].previousNodeIdx = freeCursor;
        mNodes[freeCursor].nextNodeIdx = freeCursor; //set it to self as the end
        mNodes[freeCursor].delayToPrevious = delayedTime;
        mNodes[freeCursor].callback = cb;
        mNodes[freeCursor].arg = arg;
        mNodes[freeCursor].reload = reload;
        mNodeStartIdx = freeCursor;
        mNodeEndIdx = freeCursor;
        mActiveNodes += 1U;
        ActionSchedulerId = generateActionIdAt(freeCursor);
    }
    else
    {
        uint8_t freeCursor = mNodeEndIdx;
        if(!getFreeSlot(&freeCursor))
        {
            interrupts();
            return ACTION_SCHEDULER_ID_INVALID;
        }

        mNodes[freeCursor].usedCounter++;
        mNodes[freeCursor].callback = cb;
        mNodes[freeCursor].arg = arg;
        mNodes[freeCursor].delayToPrevious = delayedTime;
        mNodes[freeCursor].reload = reload;
        mActiveNodes += 1U;

        insertNode(freeCursor, delayedTime);
        ActionSchedulerId = generateActionIdAt(freeCursor);
    }
    
    interrupts(); // Critical section end
    
    if(mActiveNodes > mActiveNodesWaterMark)
    {
        mActiveNodesWaterMark = mActiveNodes;
    }

    return ActionSchedulerId;
}

ActionSchedulerId_t ActionScheduler::schedule(uint32_t delayedTime, ActionCallback_t cb, void* arg) {
    return scheduleReload(delayedTime, delayedTime, cb, arg);
}

bool ActionScheduler::unschedule(ActionSchedulerId_t* actionId) {
    bool ret = false;
    if (*actionId != ACTION_SCHEDULER_ID_INVALID)
    {
        noInterrupts(); // Critical section begin
        uint8_t id = (uint8_t)(*actionId & 0xffU);
        uint8_t counter = (uint8_t)(*actionId >> 8U);
        if ((id < ACTION_SCHEDULER_MAX_NODES) && (mNodes[id].callback != NULL) && (mNodes[id].usedCounter == counter))
        {
            ret = true;
            removeNodeAt(id);
            *actionId = ACTION_SCHEDULER_ID_INVALID;
        }
        interrupts(); // Critical section end
    }
    return ret;
}

bool ActionScheduler::unscheduleAll(ActionCallback_t cb) {
    bool ret = false;
    noInterrupts(); // Critical section begin
    uint8_t currentCursor = mNodeStartIdx;
    uint8_t nextCursor = currentCursor;
    bool isEnd;
    do {
        currentCursor = nextCursor;
        nextCursor = mNodes[currentCursor].nextNodeIdx;
        isEnd = currentCursor == mNodeEndIdx;
        if (mNodes[currentCursor].callback == cb)
        {
            ret = true;
            removeNodeAt(currentCursor);
        }
    } while (!isEnd);
    interrupts(); // Critical section end
    return ret;
}

void ActionScheduler::clear() {
    noInterrupts(); // Critical section begin
    for (uint16_t i = 0; i < ACTION_SCHEDULER_MAX_NODES; i++)
    {
        mNodes[i].usedCounter = 0U;
        mNodes[i].arg = NULL;
        mNodes[i].callback = NULL;
        mNodes[i].delayToPrevious = 0U;
        mNodes[i].reload = 0U;
        mNodes[i].nextNodeIdx = 0U;
        mNodes[i].previousNodeIdx = 0U;
    }
    mNodeStartIdx = 0;
    mNodeEndIdx = 0;
    mActiveNodes = 0;
    mProceedingTime = 0;
    interrupts(); // Critical section end
}

uint32_t ActionScheduler::getNextEventDelay() {
    if(mActiveNodes > 0U)
    {
        return mNodes[mNodeStartIdx].delayToPrevious;
    }
    return UINT32_MAX;
}

uint32_t ActionScheduler::getProceedingTime() {
    return mProceedingTime;
}

void ActionScheduler::clearProceedingTime() {
    mProceedingTime = 0;
}

bool ActionScheduler::isCallbackArmed(ActionCallback_t cb) {
    noInterrupts(); // Critical section begin
    bool ret = false;
    
    if (mActiveNodes > 0U && mNodes[mNodeStartIdx].callback == cb)
    {
        ret = true;
    }
    else
    {
        for (uint8_t i = (mNodeStartIdx + 1U) % ACTION_SCHEDULER_MAX_NODES; 
             i != mNodeStartIdx; 
             i = (i + 1U) % ACTION_SCHEDULER_MAX_NODES)
        {
            if (mNodes[i].callback == cb)
            {
                ret = true;
                break;
            }
        }
    }
    
    interrupts(); // Critical section end
    return ret;
}

uint16_t ActionScheduler::getActiveNodesWaterMark() {
    return mActiveNodesWaterMark;
}