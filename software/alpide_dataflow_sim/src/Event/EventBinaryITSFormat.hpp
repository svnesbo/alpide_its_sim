/**
 * @file   EventBinaryFormat.hpp
 * @author Simon Voigt Nesbo
 * @date   March 5, 2018
 * @brief  Definitions for file format for MC event binary files.
 */

#ifndef EVENT_BINARY_FORMAT_HPP
#define EVENT_BINARY_FORMAT_HPP

#include <cstdint>


static const std::uint8_t DETECTOR_START = 0x20;
static const std::uint8_t DETECTOR_END   = 0x40;

static const std::uint8_t LAYER_START    = 0x01;
static const std::uint8_t LAYER_END      = 0x11;

static const std::uint8_t STAVE_START    = 0x02;
static const std::uint8_t STAVE_END      = 0x12;

static const std::uint8_t MODULE_START   = 0x03;
static const std::uint8_t MODULE_END     = 0x13;

//static const std::uint8_t HALF_MODULE_START 0x04;
//static const std::uint8_t HALF_MODULE_END 0x14;

static const std::uint8_t CHIP_START     = 0x05;
static const std::uint8_t CHIP_END       = 0x15;

static const std::uint8_t DIGIT          = 0x06;


#endif // EVENT_BINARY_FORMAT_HPP
