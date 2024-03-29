/*
  Copyright (c) 2015 Arduino LLC.  All right reserved.
  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.
  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  See the GNU Lesser General Public License for more details.
  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

// This is a modified version of the original SAMD tone implementation
// that uses two separate functions, one for each of timer 4 and timer 5
// on the SAMD

#pragma once

#ifdef __cplusplus

#include "Arduino.h"

void tone1(uint32_t _pin, uint32_t frequency, uint32_t duration = 0);
void noTone1(uint32_t _pin);
void tone2(uint32_t _pin, uint32_t frequency, uint32_t duration = 0);
void noTone2(uint32_t _pin);

#endif
