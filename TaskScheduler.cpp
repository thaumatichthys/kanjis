#include "TaskScheduler.h"


/*
    Similar to setTimeOut in JavaScript
*/


#define millis() to_ms_since_boot(get_absolute_time())

#define QUEUE_SIZE 32

typedef struct {
    uint16_t timeout;
    uint32_t timeWhenSet;
    void (*function)();
} QueuedFunction;

QueuedFunction queue[QUEUE_SIZE];
uint8_t queueLength = 0;

bool AddTask(void (*function)(), uint16_t timeout) {
    if (queueLength >= QUEUE_SIZE)
        return false;
    queue[queueLength].timeout = timeout;
    queue[queueLength].function = function;
    queue[queueLength].timeWhenSet = millis();
    queueLength++;
    return true;
}

void UpdateTasks() {
    uint32_t time = millis();
    for (int i = 0; i < queueLength; i++) {
        if (time > (queue[i].timeout + queue[i].timeWhenSet)) {
            queue[i].function();
            queueLength--;
            // move the other array elements down to fill the gap
            for (int u = i; u < queueLength; u++)
                queue[u] = queue[u + 1];
        }
    }
}
