#pragma once

#include <Arduino.h>

namespace Hardware {
constexpr uint8_t LED_PIN = 3;
constexpr uint16_t LED_COUNT = 100;

constexpr uint8_t BH1750_SDA_PIN = 0;
constexpr uint8_t BH1750_SCL_PIN = 1;
constexpr uint8_t BH1750_DVI_PIN = 4;

constexpr uint8_t BTN_SETTINGS = 5;
constexpr uint8_t BTN_RIGHT = 6;
constexpr uint8_t BTN_LEFT = 7;
}  // namespace Hardware

// Compatibility aliases used by the existing firmware implementation.
constexpr uint8_t LED_PIN = Hardware::LED_PIN;
constexpr uint16_t LED_COUNT = Hardware::LED_COUNT;
constexpr uint8_t BH1750_SDA_PIN = Hardware::BH1750_SDA_PIN;
constexpr uint8_t BH1750_SCL_PIN = Hardware::BH1750_SCL_PIN;
constexpr uint8_t BH1750_DVI_PIN = Hardware::BH1750_DVI_PIN;
constexpr uint8_t BTN_SETTINGS = Hardware::BTN_SETTINGS;
constexpr uint8_t BTN_RIGHT = Hardware::BTN_RIGHT;
constexpr uint8_t BTN_LEFT = Hardware::BTN_LEFT;
