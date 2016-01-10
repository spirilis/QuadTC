///
/// @file QuadTC.h
/// @brief  Header
/// @headerfile <>
/// @details    Simple library for Spirilis's Quad Thermocouple BoosterPack
///
/// @author   Eric Brundick
/// @date     2016
/// @version  101
/// @copyright (C) 2016 Eric Brundick spirilis at linux dot com
///  @n GNU Lesser General Public License
///  @n
///  @n This library is free software; you can redistribute it and/or
///  @n modify it under the terms of the GNU Lesser General Public
///  @n License as published by the Free Software Foundation; either
///  @n version 2.1 of the License, or (at your option) any later version.
///  @n
///  @n This library is distributed in the hope that it will be useful,
///  @n but WITHOUT ANY WARRANTY; without even the implied warranty of
///  @n MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
///  @n Lesser General Public License for more details.
///  @n
///  @n You should have received a copy of the GNU Lesser General Public
///  @n License along with this library; if not, write to the Free Software
///  @n Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA


#ifndef QUADTC_H
#define QUADTC_H

// Supports Arduino, Energia et al
#include <Arduino.h>
#include <SPI.h>

// Default TCPOWER, SPICS pins
#define QUADTC_DEFAULT_GPIO_TCPOWER 40
#define QUADTC_DEFAULT_GPIO_SPICS1 39
#define QUADTC_DEFAULT_GPIO_SPICS2 38
#define QUADTC_DEFAULT_GPIO_SPICS3 37
#define QUADTC_DEFAULT_GPIO_SPICS4 36


// Definitions for MAX31855 fault bits
#define MAX31855_FAULT_SCV 0x04
#define MAX31855_FAULT_SCG 0x02
#define MAX31855_FAULT_OC 0x01

// User-friendly versions
#define MAX31855_FAULT_SHORT_VCC MAX31855_FAULT_SCV
#define MAX31855_FAULT_SHORT_GND MAX31855_FAULT_SCG
#define MAX31855_FAULT_DISCONNECT MAX31855_FAULT_OC

// Typedefs and enums
typedef enum {
    NONE=0,
    SHORT_VCC,
    SHORT_GND,
    DISCONNECTED,
    BUS_ERROR
} max31855_Fault;


// Debugging
// #define QUADTC_DEBUG 1
#ifdef QUADTC_DEBUG
/// @brief Assert expression 'x' is valid, if not, print error.  Only for internal QuadTC library use.
/// @details If the expression 'x' is false, an error including the supplied 'str' string and
///          value 'val' is printed out to a debugging interface of some type defined by the
///          _quadtc_do_assert() private function within QuadTC.
/// @warning Because _quadtc_do_assert relies on Serial, DO NOT USE THIS inside a Constructor!!
#define QUADTC_ASSERT(x, str, val) { if (!(x)) { _quadtc_do_assert(str, val); }; }
#else
#define QUADTC_ASSERT(x, str, val)
#endif


template <unsigned int tc_count>
class QuadTC {
    private:
        uint8_t _spics[tc_count];
        uint8_t _gpio_tcpower;
        int16_t _tctemp[tc_count];
        int16_t _ambtemp[tc_count];
        max31855_Fault _faults[tc_count];

        /// @brief Pulls 32-bit word from MAX31855 over SPI
        uint32_t _retrieve_data(unsigned int idx);

        #ifdef QUADTC_DEBUG
        /// @brief Sends library debugging information to Serial
        void _quadtc_do_assert(const char *s, const int val) {
            // Assumes Serial is available and ready to roll.
            Serial.print("QuadTC assert triggered: ");
            Serial.print(s); Serial.print(' '); Serial.println(val);
        }
        #endif

    public:
        QuadTC(); ///< Constructor
        QuadTC(uint8_t tcpower, uint8_t cs1);
        QuadTC(uint8_t tcpower, uint8_t cs1, uint8_t cs2);
        QuadTC(uint8_t tcpower, uint8_t cs1, uint8_t cs2, uint8_t cs3);
        QuadTC(uint8_t tcpower, uint8_t cs1, uint8_t cs2, uint8_t cs3, uint8_t cs4);

        ///
        /// @brief Initialize
        /// @details Initialize the GPIO state of all Power Domains and SPI Chip Select lines.
        ///          This library expects the user to run SPI.begin() before actually using the chips
        ///          but it's not strictly necessary to run SPI.begin() before initializing this library.
        ///          @n If you are using the QUADTC_DEBUG facilities, be sure to run Serial.begin() before this.
        void begin(void);

        ///
        /// @brief Stop
        /// @details Ensure Power Domain is switched off to gracefully close out this library's function.
        ///          It also releases the SPI Chip Select lines by setting the GPIO ports to INPUT mode.
        void end(void);

        ///
        /// @brief Activate Thermocouple Power Domain
        /// @details This applies power to the thermocouple chips so they may begin doing ADC conversion.
        ///          The firmware should wait around 300ms after running .start() before obtaining data.
        void start(void);

        ///
        /// @brief Shut off Thermocouple Power Domain
        /// @details This is almost an alias for end(), but it's not intended to be the last time you
        ///          use this library - it's just a complement to start().  It deselects all the
        ///          SPI Chip Select lines in case they were somehow left selected.
        void stop(void);

        ///
        /// @brief Retrieve thermocouple, ambient temperature values
        /// @details This function will go out to each MAX31855 and read its data, storing it locally for
        ///          consumption.  You may run stop() immediately after this to shut off the MAX31855
        ///          chips to save power.
        /// @returns Number equal to how many MAX31855 chips are present and not faulted.
        unsigned int retrieve(void);

        ///
        /// @brief Retrieve Thermocouple temperature
        /// @details Retrieves the Thermocouple temperature in whole degrees Celsius for the
        ///          indicated MAX31855 chip
        /// @param idx Index of which MAX31855 chip (1-tc_count)
        /// @returns Temperature in degrees Celsius, no decimal
        int16_t getThermocoupleCelsius(unsigned int idx);

        ///
        /// @brief Retrieve Thermocouple temperature
        /// @details Retrieves the Thermocouple temperature in whole degrees Fahrenheit for the
        ///          indicated MAX31855 chip
        /// @param idx Index of which MAX31855 chip (1-tc_count)
        /// @returns Temperature in degrees Fahrenheit, no decimal
        int16_t getThermocoupleFahrenheit(unsigned int idx);

        ///
        /// @brief Retrieve Cold-Junction Compensation temperature
        /// @details Retrieves the ambient temperature at the MAX31855 chip itself, which is used
        ///          for Cold-Junction Compensation.  Reflects PCB temperature.
        /// @param idx Index of which MAX31855 chip (1-tc_count)
        /// @returns Temperature in degrees Celsius, no decimal
        int16_t getAmbientCelsius(unsigned int idx);

        ///
        /// @brief Retrieve Cold-Junction Compensation temperature
        /// @details Retrieves the ambient temperature at the MAX31855 chip itself, which is used
        ///          for Cold-Junction Compensation.  Reflects PCB temperature.
        /// @param idx Index of which MAX31855 chip (1-tc_count)
        /// @returns Temperature in degrees Fahrenheit, no decimal
        int16_t getAmbientFahrenheit(unsigned int idx);

        ///
        /// @brief Return fault codes, if any
        /// @details Reports the fault state of the specified MAX31855 chip
        /// @param idx Index of which MAX31855 chip (1-tc_count)
        /// @returns An enum representing the present fault state (NONE, SHORT_VCC, SHORT_GND, DISCONNECTED)
        max31855_Fault getFault(unsigned int idx);
};


// Actual implementation code
#include "QuadTC.tpp"

#endif /* QUADTC_H */
