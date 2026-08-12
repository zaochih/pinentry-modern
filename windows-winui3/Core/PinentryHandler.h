#pragma once

extern "C"
{
#include "../../pinentry/pinentry.h"
}

extern pinentry_cmd_handler_t pinentry_cmd_handler;

int pinentry_winui3_cmd_handler(pinentry_t pe);

