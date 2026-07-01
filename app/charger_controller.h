#pragma once

#include "result.h"

Result charger_controller_init();
Result charger_controller_task();
bool charger_controller_isCharging();
