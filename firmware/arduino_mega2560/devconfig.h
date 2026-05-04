/*
 * LWCloneU2
 * Copyright (C) 2013 Andreas Dittrich <lwcloneu2@cithraidt.de>
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
 * if not, write to the Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef DEVCONFIG_H__INCLUDED
#define DEVCONFIG_H__INCLUDED

// #define DEBUGLEVEL 2


/****************************************
 USB device config
****************************************/

#define ENABLE_LED_DEVICE
#define ENABLE_ANALOG_INPUT

#define ENABLE_PANEL_DEVICE

// Note : le Mode Nuit est désormais géré par un interrupteur à bascule
// physique dont la broche est définie dans pinmap.h (NIGHT_SWITCH_PORT / NIGHT_SWITCH_BIT).

#define NUM_JOYSTICKS 0
#define USE_MOUSE 0
#define USE_CONSUMER 1
#define USE_KEYBOARD 1

#define USE_ACCELGYRO 1
#define ACCELGYRO_MPU6050 1
// Does not work
// #define ACCELGYRO_MPU6050_USE_FIFO 1


/*
 * [KM3L0TT] Communication Serie ESP32
 * ======================================
 * Decommenter ESP32_SERIAL_BAUD pour activer la liaison serie avec l'ESP32.
 * UART1 utilise : D18=TX1 / D19=RX1 (partage avec DEBUGLEVEL si active)
 *
 * LEDWIZ_SERIAL_ID : identifiant de cette carte dans les trames serie
 *   0x1C = LedWiz 1 Caisse (Mega)
 *   0x2F = LedWiz 2 Fronton (ProMicro 1)
 *   0x3F = LedWiz 3 Fronton (ProMicro 2)
 *
 * nightModeSerial : mode nuit recu depuis l'ESP32 [0C AA 00/01]
 *   Priorite inferieure au switch physique NIGHT_SWITCH_PORT/BIT
 *   Si switch ferme : nightMode=1 quelle que soit la valeur serie
 *
 * debugModeSerial : active l'envoi des trames debug [0C DD 00/01]
 *   Par defaut (debug inactif) : seul heartbeat toutes les 500ms
 *   Si actif : sorties PWM, entrees, HID, plunger, gyro (au changement)
 */
#define ESP32_SERIAL_BAUD  115200
#define LEDWIZ_SERIAL_ID          0x1C
#endif

