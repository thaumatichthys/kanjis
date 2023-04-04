#pragma once

#include "pico/stdlib.h"
#include "hardware/clocks.h"


bool AddTask(void (*function)(), uint16_t delay);
void UpdateTasks();
