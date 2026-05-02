/*
 * LWCloneU2
 * Copyright (C) 2013 Andreas Dittrich
 *
 * This program is free software; you can redistribute it and/or modify it under the terms of the
 * GNU General Public License as published by the Free Software Foundation;
 * either version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with this program;
 * if not, write to the Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

/*
 * Arduino Pro Micro (ATmega32U4) - LEDWiz outputs only.
 * LED mapping syntax: _map_(PORT, bit, inv, night, pwm_off)
 * D0/D1 (UART RX/TX) are intentionally left free for serial usage.
 */

/* Optional night switch (active LOW with pull-up), uncomment if used:
 * #define NIGHT_SWITCH_PORT  F
 * #define NIGHT_SWITCH_BIT   0
 */

#define LED_MAPPING_TABLE(_map_) \
	/* Digital pins (except D0/D1 kept for UART) */ \
	_map_( D, 1, 0, 0, 0 ) /* D2  */ \
	_map_( D, 0, 0, 0, 0 ) /* D3  */ \
	_map_( D, 4, 0, 0, 0 ) /* D4  */ \
	_map_( C, 6, 0, 0, 0 ) /* D5  */ \
	_map_( D, 7, 0, 0, 0 ) /* D6  */ \
	_map_( E, 6, 0, 0, 0 ) /* D7  */ \
	_map_( B, 4, 0, 0, 0 ) /* D8  */ \
	_map_( B, 5, 0, 0, 0 ) /* D9  */ \
	_map_( B, 6, 0, 0, 0 ) /* D10 */ \
	_map_( B, 3, 0, 0, 0 ) /* D14 (MISO) */ \
	_map_( B, 1, 0, 0, 0 ) /* D15 (SCK)  */ \
	_map_( B, 2, 0, 0, 0 ) /* D16 (MOSI) */ \
	/* Analog pins used as digital outputs (only A0..A3 for broad clone compatibility) */ \
	_map_( F, 7, 0, 0, 0 ) /* A0 */ \
	_map_( F, 6, 0, 0, 0 ) /* A1 */ \
	_map_( F, 5, 0, 0, 0 ) /* A2 */ \
	_map_( F, 4, 0, 0, 0 ) /* A3 */ \
	\
	/* end */
