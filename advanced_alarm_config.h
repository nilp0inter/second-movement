#pragma once

#include "advanced_alarm_face.h"

#define ADVANCED_ALARM_PRESETS_START 6

#define ADVANCED_ALARM_PRESETS { \
    {ALARM_DAY_EACH_DAY, 8, 30, 5, 1, true},  /* Pastilla Mos      */ \
    {ALARM_DAY_WEEKEND, 10, 30, 5, 1, true},   /* Media mañana      */ \
    {ALARM_DAY_WEEKEND, 12,  0, 5, 1, true},   /* Comer             */ \
    {ALARM_DAY_WEEKEND, 12, 45, 5, 1, true},   /* Siesta            */ \
    {ALARM_DAY_WEEKEND, 16,  0, 5, 1, true},   /* Merienda          */ \
    {ALARM_DAY_WORKDAY, 16, 30, 5, 1, true},   /* School            */ \
    {ALARM_DAY_WORKDAY, 16, 52, 5, 1, true},   /* School!           */ \
    {ALARM_DAY_EACH_DAY, 19,  0, 5, 1, true},  /* Baño              */ \
    {ALARM_DAY_EACH_DAY, 19, 40, 5, 1, true},  /* Cena              */ \
    {ALARM_DAY_EACH_DAY, 20, 30, 5, 1, true},  /* Dormir y Pastilla Mos */ \
}
