/**
 * QuadTC.tpp - Code for the QuadTC class supporting Spirilis's Quad Thermocouple BoosterPack
 *              Not intended to be built directly by the compiler; it is included inside QuadTC.h
 *
 *  @author   Eric Brundick
 *  @date     2016
 *  @version  101
 *  @copyright (C) 2016 Eric Brundick spirilis at linux dot com
 *   @n GNU Lesser General Public License
 *   @n
 *   @n This library is free software; you can redistribute it and/or
 *   @n modify it under the terms of the GNU Lesser General Public
 *   @n License as published by the Free Software Foundation; either
 *   @n version 2.1 of the License, or (at your option) any later version.
 *   @n
 *   @n This library is distributed in the hope that it will be useful,
 *   @n but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   @n MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *   @n Lesser General Public License for more details.
 *   @n
 *   @n You should have received a copy of the GNU Lesser General Public
 *   @n License along with this library; if not, write to the Free Software
 *   @n Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */


template <unsigned int tc_count>
QuadTC<tc_count>::QuadTC()
{
    int i;

    for (i=0; i < tc_count; i++) {
        _spics[i] = 0;
        _tctemp[i] = 0;
        _ambtemp[i] = 0;
        _faults[i] = NONE;
    }

    _gpio_tcpower = QUADTC_DEFAULT_GPIO_TCPOWER;
    if (tc_count > 0) {
        _spics[0] = QUADTC_DEFAULT_GPIO_SPICS1;
    }
    if (tc_count > 1) {
        _spics[1] = QUADTC_DEFAULT_GPIO_SPICS2;
    }
    if (tc_count > 2) {
        _spics[2] = QUADTC_DEFAULT_GPIO_SPICS3;
    }
    if (tc_count > 3) {
        _spics[3] = QUADTC_DEFAULT_GPIO_SPICS4;
    }

    _spi = &SPI;
}

template <unsigned int tc_count>
QuadTC<tc_count>::QuadTC(uint8_t tcpower, uint8_t cs1)
{
    QuadTC();

    _gpio_tcpower = tcpower;
    if (tc_count > 0) {
        _spics[0] = cs1;
    }
}

template <unsigned int tc_count>
QuadTC<tc_count>::QuadTC(uint8_t tcpower, uint8_t cs1, uint8_t cs2)
{
    QuadTC();

    _gpio_tcpower = tcpower;
    if (tc_count > 0) {
        _spics[0] = cs1;
    }
    if (tc_count > 1) {
        _spics[1] = cs2;
    }
}

template <unsigned int tc_count>
QuadTC<tc_count>::QuadTC(uint8_t tcpower, uint8_t cs1, uint8_t cs2, uint8_t cs3)
{
    QuadTC();

    _gpio_tcpower = tcpower;
    if (tc_count > 0) {
        _spics[0] = cs1;
    }
    if (tc_count > 1) {
        _spics[1] = cs2;
    }
    if (tc_count > 2) {
        _spics[2] = cs3;
    }
}

template <unsigned int tc_count>
QuadTC<tc_count>::QuadTC(uint8_t tcpower, uint8_t cs1, uint8_t cs2, uint8_t cs3, uint8_t cs4)
{
    QuadTC();

    _gpio_tcpower = tcpower;
    if (tc_count > 0) {
        _spics[0] = cs1;
    }
    if (tc_count > 1) {
        _spics[1] = cs2;
    }
    if (tc_count > 2) {
        _spics[2] = cs3;
    }
    if (tc_count > 3) {
        _spics[3] = cs4;
    }
}

template <unsigned int tc_count>
void QuadTC<tc_count>::setSPI(SPIClass *spiclass_instance)
{
    _spi = spiclass_instance;
}

template <unsigned int tc_count>
void QuadTC<tc_count>::begin(void)
{
    int i;

    QUADTC_ASSERT(tc_count > 0, "Invalid tc_count of 0! tc_count =", tc_count);
    QUADTC_ASSERT(tc_count < 5, "tc_count is > 4 but this BoosterPack only supports 4 MAX31855 chips! tc_count =", tc_count);

    // Initialize GPIOs
    for (i=0; i < tc_count; i++) {
        pinMode(_spics[i], OUTPUT);
        digitalWrite(_spics[i], HIGH);
    }
    pinMode(_gpio_tcpower, OUTPUT);
    digitalWrite(_gpio_tcpower, LOW);

    // Clear all data
    for (i=0; i < tc_count; i++) {
        _tctemp[i] = 0;
        _ambtemp[i] = 0;
        _faults[i] = NONE;
    }
}

template <unsigned int tc_count>
void QuadTC<tc_count>::end(void)
{
    int i;

    digitalWrite(_gpio_tcpower, LOW);
    for (i=0; i < tc_count; i++) {
        pinMode(_spics[i], INPUT);
    }
}

template <unsigned int tc_count>
void QuadTC<tc_count>::stop(void)
{
    int i;

    digitalWrite(_gpio_tcpower, LOW);   // Deactivate MAX31855 Power Domain
    for (i=0; i < tc_count; i++) {
        digitalWrite(_spics[i], HIGH);  // Deselect SPI Chip Select
    }
}

template <unsigned int tc_count>
void QuadTC<tc_count>::start(void)
{
    int i;

    for (i=0; i < tc_count; i++) {
        digitalWrite(_spics[i], HIGH);  // Deselect SPI Chip Select
    }
    digitalWrite(_gpio_tcpower, HIGH);  // Activate power to MAX31855 Power Domain
}

template <unsigned int tc_count>
unsigned int QuadTC<tc_count>::retrieve(void)
{
    int i;
    unsigned int good = 0;
    uint32_t res;
    int16_t datum;

    // Warning: This does not check to see if the Thermocouple Power Domain is active.
    for (i=0; i < tc_count; i++) {
        res = _retrieve_data(i+1);
        if (res == 0x00000000 || res == 0xFFFFFFFF) {  // Most likely an error - such as TC Power Domain not active!
            _faults[i] = BUS_ERROR;

            // Nice debugging hook...
            QUADTC_ASSERT(0, "retrieve: Invalid SPI data received at idx =", i);
        } else {
            // Interpret results!
            datum = res >> 18;
            if (datum & 0x2000) {  // Extend Sign-Bit if negative
                datum |= 0xC000;
            }
            _tctemp[i] = datum;

            datum = res >> 4;
            datum &= 0x0FFF;
            if (datum & 0x0800) {
                datum |= 0xF000;
            }
            _ambtemp[i] = datum;
            
            _faults[i] = NONE;
            if (res & MAX31855_FAULT_SHORT_VCC) {
                _faults[i] = SHORT_VCC;
            }
            if (res & MAX31855_FAULT_SHORT_GND) {
                _faults[i] = SHORT_GND;
            }
            if (res & MAX31855_FAULT_DISCONNECT) {
                _faults[i] = DISCONNECTED;
            }
            if (_faults[i] == NONE) {
                good++;  // Count this as a "good" thermocouple!
            }
        }
    }

    return good;
}

template <unsigned int tc_count>
uint32_t QuadTC<tc_count>::_retrieve_data(unsigned int idx)
{
    int i;
    uint32_t res = 0;
    uint32_t b;

    // Sanity check
    if (idx == 0 || idx > tc_count) {
        QUADTC_ASSERT(0, "_retrieve_data: Invalid idx!", idx);
        return 0;
    }

    // Select chip
    idx--;  // The chip index starts at 1 but our internal arrays start at 0.
    digitalWrite(_spics[idx], LOW);
    for (i=0; i < 4; i++) {
        b = _spi->transfer(0);
        b <<= 8 * (3-i);
        res |= b;
    }
    digitalWrite(_spics[idx], HIGH);

    return res;
}

template <unsigned int tc_count>
int16_t QuadTC<tc_count>::getThermocoupleCelsius(unsigned int idx)
{
    // Sanity check
    if (idx == 0 || idx > tc_count) {
        QUADTC_ASSERT(0, "getThermocoupleCelsius: Invalid idx!", idx);
        return 0;
    }

    idx--;  // The chip index starts at 1 but our internal arrays start at 0.

    int16_t celsius = _tctemp[idx] / 4;  /* Remove the latter 2 bits of precision using arithmetic
                                          * to preserve the sign-bit.
                                          */
    return celsius;
}

template <unsigned int tc_count>
int16_t QuadTC<tc_count>::getThermocoupleFahrenheit(unsigned int idx)
{
    // Sanity check
    if (idx == 0 || idx > tc_count) {
        QUADTC_ASSERT(0, "getThermocoupleFahrenheit: Invalid idx!", idx);
        return 0;
    }

    idx--;  // The chip index starts at 1 but our internal arrays start at 0.

    // fahrenheit will be manipulated with decimal bits (lowest 2 bits) intact.
    int16_t fahrenheit = _tctemp[idx];

    // Degrees_F = (Degrees_C * 9/5) + 32
    fahrenheit *= 9;
    fahrenheit /= 5;
    fahrenheit /= 4;  // Remove decimal bits
    fahrenheit += 32;

    return fahrenheit;
}

template <unsigned int tc_count>
int16_t QuadTC<tc_count>::getAmbientCelsius(unsigned int idx)
{
    // Sanity check
    if (idx == 0 || idx > tc_count) {
        QUADTC_ASSERT(0, "getAmbientCelsius: Invalid idx!", idx);
        return 0;
    }

    idx--;  // The chip index starts at 1 but our internal arrays start at 0.

    int16_t celsius = _ambtemp[idx] / 16;  /* Remove the latter 4 bits of precision using arithmetic
                                            * to preserve the sign-bit.
                                            */
    return celsius;
}

template <unsigned int tc_count>
int16_t QuadTC<tc_count>::getAmbientFahrenheit(unsigned int idx)
{
    // Sanity check
    if (idx == 0 || idx > tc_count) {
        QUADTC_ASSERT(0, "getAmbientFahrenheit: Invalid idx!", idx);
        return 0;
    }

    idx--;  // The chip index starts at 1 but our internal arrays start at 0.

    // fahrenheit will be manipulated with decimal bits (lowest 2 bits) intact.
    int16_t fahrenheit = _ambtemp[idx];

    // Degrees_F = (Degrees_C * 9/5) + 32
    fahrenheit *= 9;
    fahrenheit /= 5;
    fahrenheit /= 16;  // Remove decimal bits
    fahrenheit += 32;

    return fahrenheit;
}

template <unsigned int tc_count>
max31855_Fault QuadTC<tc_count>::getFault(unsigned int idx)
{
    // Sanity check
    if (idx == 0 || idx > tc_count) {
        QUADTC_ASSERT(0, "getFault: Invalid idx!", idx);
        return BUS_ERROR;
    }

    idx--;  // The chip index starts at 1 but our internal arrays start at 0.

    return _faults[idx];
}
