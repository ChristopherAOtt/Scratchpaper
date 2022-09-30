#pragma once

#include "Primitives.hpp"

constexpr float LOW = 0.05f;
constexpr float MEDIUM = 0.5f;
constexpr float HIGH = 0.90f;

constexpr FVec3 COLOR_RED_MILD =    {HIGH,   MEDIUM, MEDIUM};
constexpr FVec3 COLOR_GREEN_MILD =  {MEDIUM, HIGH,   MEDIUM};
constexpr FVec3 COLOR_BLUE_MILD =   {MEDIUM, MEDIUM, HIGH};
constexpr FVec3 COLOR_RED_VIVID =   {1.0,    0.0,    0.0};
constexpr FVec3 COLOR_GREEN_VIVID = {0.0,    1.0,    0.0};
constexpr FVec3 COLOR_BLUE_VIVID =  {0.0,    0.0,    1.0};
constexpr FVec3 COLOR_WHITE =       {1.0,    1.0,    1.0};
constexpr FVec3 COLOR_BLACK =       {0.0,    0.0,    0.0};

