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

#ifndef DEVCONFIG_H__INCLUDED
#define DEVCONFIG_H__INCLUDED

// #define DEBUGLEVEL 2

/****************************************
 USB device config
****************************************/

#define ENABLE_LED_DEVICE
/* LEDWiz only on Pro Micro: no panel, no keyboard, no joystick, no gyro */

// Note: if enabled in pinmap.h, the night switch uses NIGHT_SWITCH_PORT / NIGHT_SWITCH_BIT.

#define NUM_JOYSTICKS 0
#define USE_MOUSE 0
#define USE_CONSUMER 0
#define USE_KEYBOARD 0


/*
 * [KM3L0TT] Communication Serie ESP32
 * ======================================
 * Decommenter ESP32_SERIAL_BAUD pour activer la liaison serie avec l'ESP32 Fronton.
 * UART1 utilise : D1=TX / D0=RX (pins reservees dans pinmap.h)
 *
 * LEDWIZ_SERIAL_ID : changer selon le ProMicro
 *   0x2F = LedWiz 2 Fronton (ProMicro 1)
 *   0x3F = LedWiz 3 Fronton (ProMicro 2)
 *
 * Sur ProMicro, nightMode est pilote UNIQUEMENT par la trame serie [0C AA 00/01]
 * (pas de switch physique connecte directement sur le ProMicro)
 * Pour ajouter un switch physique local, decommenter NIGHT_SWITCH dans pinmap.h
 */
#define ESP32_SERIAL_BAUD  115200
#define LEDWIZ_SERIAL_ID          0x2F   /* Changer en 0x3F pour ProMicro 2 */
#endif
