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
/*
 * Modifications KM3L0TT :
 *  - Tableau de correspondances broches Arduino Mega 2560 (PORT/bit)
 *  - LED_MAPPING_TABLE : nouvelle signature _map_(PORT, bit, inv, night, pwm_off)
 *    suppression Timer4, ajout mode nuit par sortie, coupure securite par sortie
 *  - 32 sorties LedWiz declarees sur D22-D53 avec parametres par defaut
 *  - NIGHT_SWITCH_PORT / NIGHT_SWITCH_BIT : interrupteur bascule Mode Nuit
 *  - PANEL_MAPPING_TABLE / ADC_MAPPING_TABLE : commentaires de reference ajoutés
 */


/*

	_map_( B, 7, 0 )
	_map_( A, 5, 0, 0, 0 ) 
	_map_( A, 6, 0, 0, 0 ) 
	_map_( A, 7, 0, 0, 0 ) 
*/

/* ================================================================
 * CORRESPONDANCES BROCHES - Arduino Mega 2560
 * ================================================================
 * Légende :
 *   [SW]     Interrupteur Mode Nuit  -> définir NIGHT_SWITCH_PORT / BIT
 *   [LW]     Sortie LedWiz           -> déclarer dans LED_MAPPING_TABLE
 *   [BTN]    Bouton panneau          -> déclarer dans PANEL_MAPPING_TABLE
 *   [ADC]    Entrée analogique       -> déclarer dans ADC_MAPPING_TABLE
 *   [SYS]    Réservé système (UART, I2C, SPI, LED interne)
 *
 * Câblage [SW] : broche choisie → GND  (pull-up interne automatique)
 *                interrupteur OUVERT = jouets actifs
 *                interrupteur FERMÉ  = jouets muets
 * ----------------------------------------------------------------
 * Arduino      | PORT | PIN | Remarque
 * -------------|------|-----|------------------------------------
 * D0           |  E   |  0  | [SYS] RX0
 * D1           |  E   |  1  | [SYS] TX0
 * D2           |  E   |  4  |
 * D3           |  E   |  5  | PWM
 * D4           |  G   |  5  |
 * D5           |  E   |  3  | PWM
 * D6           |  H   |  3  | PWM OC4A
 * D7           |  H   |  4  | PWM OC4B
 * D8           |  H   |  5  | PWM OC4C
 * D9           |  H   |  6  | PWM
 * D10          |  B   |  4  | PWM
 * D11          |  B   |  5  | PWM
 * D12          |  B   |  6  | PWM
 * D13          |  B   |  7  | [SYS] LED interne (health blink)
 * D14          |  J   |  1  | [SYS] TX3
 * D15          |  J   |  0  | [SYS] RX3
 * D16          |  H   |  1  | [SYS] TX2
 * D17          |  H   |  0  | [SYS] RX2
 * D18          |  D   |  3  | [SYS] TX1
 * D19          |  D   |  2  | [SYS] RX1
 * D20          |  D   |  1  | [SYS] SDA I2C (MPU6050)
 * D21          |  D   |  0  | [SYS] SCL I2C (MPU6050)
 * -------------|------|-----|-- 32 sorties LedWiz ---------------
 * D22          |  A   |  0  | [LW] LedWiz 1
 * D23          |  A   |  1  | [LW] LedWiz 2
 * D24          |  A   |  2  | [LW] LedWiz 3
 * D25          |  A   |  3  | [LW] LedWiz 4
 * D26          |  A   |  4  | [LW] LedWiz 5
 * D27          |  A   |  5  | [LW] LedWiz 6
 * D28          |  A   |  6  | [LW] LedWiz 7
 * D29          |  A   |  7  | [LW] LedWiz 8
 * D30          |  C   |  7  | [LW] LedWiz 9
 * D31          |  C   |  6  | [LW] LedWiz 10
 * D32          |  C   |  5  | [LW] LedWiz 11
 * D33          |  C   |  4  | [LW] LedWiz 12
 * D34          |  C   |  3  | [LW] LedWiz 13
 * D35          |  C   |  2  | [LW] LedWiz 14
 * D36          |  C   |  1  | [LW] LedWiz 15
 * D37          |  C   |  0  | [LW] LedWiz 16
 * D38          |  D   |  7  | [LW] LedWiz 17
 * D39          |  G   |  2  | [LW] LedWiz 18
 * D40          |  G   |  1  | [LW] LedWiz 19
 * D41          |  G   |  0  | [LW] LedWiz 20
 * D42          |  L   |  7  | [LW] LedWiz 21
 * D43          |  L   |  6  | [LW] LedWiz 22
 * D44          |  L   |  5  | [LW] LedWiz 23  PWM
 * D45          |  L   |  4  | [LW] LedWiz 24  PWM
 * D46          |  L   |  3  | [LW] LedWiz 25  PWM
 * D47          |  L   |  2  | [LW] LedWiz 26
 * D48          |  L   |  1  | [LW] LedWiz 27
 * D49          |  L   |  0  | [LW] LedWiz 28
 * D50          |  B   |  3  | [LW] LedWiz 29  (MISO SPI - éviter si shield SPI)
 * D51          |  B   |  2  | [LW] LedWiz 30  (MOSI SPI - éviter si shield SPI)
 * D52          |  B   |  1  | [LW] LedWiz 31  (SCK  SPI - éviter si shield SPI)
 * D53          |  B   |  0  | [LW] LedWiz 32  (SS   SPI - éviter si shield SPI)
 * -------------|------|-----|-- Analogiques / Digital ------------
 * A0  / D54    |  F   |  0  | [BTN]
 * A1  / D55    |  F   |  1  | [BTN]
 * A2  / D56    |  F   |  2  | [BTN]
 * A3  / D57    |  F   |  3  | [BTN]
 * A4  / D58    |  F   |  4  | [BTN]
 * A5  / D59    |  F   |  5  | [BTN]
 * A6  / D60    |  F   |  6  | [BTN]
 * A7  / D61    |  F   |  7  | [BTN]
 * A8  / D62    |  K   |  0  | [SW]  <- défaut NIGHT_SWITCH
 * A9  / D63    |  K   |  1  |
 * A10 / D64    |  K   |  2  |
 * A11 / D65    |  K   |  3  |
 * A12 / D66    |  K   |  4  |
 * A13 / D67    |  K   |  5  |
 * A14 / D68    |  K   |  6  |
 * A15 / D69    |  K   |  7  | [ADC] Plunger (potentiomètre)
 * ================================================================
 *
 * Interrupteur Mode Nuit - choisir une broche libre ci-dessus :
 */
#define NIGHT_SWITCH_PORT   K
#define NIGHT_SWITCH_BIT    0

/*
 * LED_MAPPING_TABLE - Declaration des 32 sorties LedWiz
 * =======================================================
 * Syntaxe : _map_( PORT, bit, inv, night, pwm_off )
 *
 *  PORT    : lettre du port AVR                    ex: A, B, C...
 *  bit     : numero de bit du port (0 a 7)         ex: 0, 3, 7
 *  inv     : inversion logique de la sortie        0 = actif HIGH (defaut)
 *                                                  1 = actif LOW
 *  night   : coupure en Mode Nuit                  0 = toujours actif (defaut)
 *                                                  1 = coupe si interrupteur ferme
 *  pwm_off : coupure securite (unite = ~10ms)      0 = pas de coupure (defaut)
 *                                                  200 = coupure apres ~2 secondes
 *
 * Exemples :
 *  _map_( A, 0, 0, 0,   0 )  // LED simple        : toujours active, pas de coupure
 *  _map_( A, 1, 0, 1,   0 )  // Lampe             : coupee en Mode Nuit
 *  _map_( A, 2, 0, 1, 200 )  // Solenoide         : coupe en Mode Nuit + securite 2s
 *  _map_( A, 3, 0, 0, 200 )  // Solenoide critique: toujours actif + securite 2s
 *
 * Note : si le logiciel reactive une sortie apres coupure securite, le timer repart.
 */
#define LED_MAPPING_TABLE(_map_) \
	\
	_map_( A, 0, 0, 0, 0 ) /* D22 - LedWiz  1 */ \
	_map_( A, 1, 0, 0, 0 ) /* D23 - LedWiz  2 */ \
	_map_( A, 2, 0, 0, 0 ) /* D24 - LedWiz  3 */ \
	_map_( A, 3, 0, 0, 0 ) /* D25 - LedWiz  4 */ \
	_map_( A, 4, 0, 0, 0 ) /* D26 - LedWiz  5 */ \
	_map_( A, 5, 0, 0, 0 ) /* D27 - LedWiz  6 */ \
	_map_( A, 6, 0, 0, 0 ) /* D28 - LedWiz  7 */ \
	_map_( A, 7, 0, 0, 0 ) /* D29 - LedWiz  8 */ \
	_map_( C, 7, 0, 0, 0 ) /* D30 - LedWiz  9 */ \
	_map_( C, 6, 0, 0, 0 ) /* D31 - LedWiz 10 */ \
	_map_( C, 5, 0, 0, 0 ) /* D32 - LedWiz 11 */ \
	_map_( C, 4, 0, 0, 0 ) /* D33 - LedWiz 12 */ \
	_map_( C, 3, 0, 0, 0 ) /* D34 - LedWiz 13 */ \
	_map_( C, 2, 0, 0, 0 ) /* D35 - LedWiz 14 */ \
	_map_( C, 1, 0, 0, 0 ) /* D36 - LedWiz 15 */ \
	_map_( C, 0, 0, 0, 0 ) /* D37 - LedWiz 16 */ \
	_map_( D, 7, 0, 0, 0 ) /* D38 - LedWiz 17 */ \
	_map_( G, 2, 0, 0, 0 ) /* D39 - LedWiz 18 */ \
	_map_( G, 1, 0, 0, 0 ) /* D40 - LedWiz 19 */ \
	_map_( G, 0, 0, 0, 0 ) /* D41 - LedWiz 20 */ \
	_map_( L, 7, 0, 0, 0 ) /* D42 - LedWiz 21 */ \
	_map_( L, 6, 0, 0, 0 ) /* D43 - LedWiz 22 */ \
	_map_( L, 5, 0, 0, 0 ) /* D44 - LedWiz 23 */ \
	_map_( L, 4, 0, 0, 0 ) /* D45 - LedWiz 24 */ \
	_map_( L, 3, 0, 0, 0 ) /* D46 - LedWiz 25 */ \
	_map_( L, 2, 0, 0, 0 ) /* D47 - LedWiz 26 */ \
	_map_( L, 1, 0, 0, 0 ) /* D48 - LedWiz 27 */ \
	_map_( L, 0, 0, 0, 0 ) /* D49 - LedWiz 28 */ \
	_map_( B, 3, 0, 0, 0 ) /* D50 - LedWiz 29 - MISO SPI (eviter si shield SPI) */ \
	_map_( B, 2, 0, 0, 0 ) /* D51 - LedWiz 30 - MOSI SPI (eviter si shield SPI) */ \
	_map_( B, 1, 0, 0, 0 ) /* D52 - LedWiz 31 - SCK  SPI (eviter si shield SPI) */ \
	_map_( B, 0, 0, 0, 0 ) /* D53 - LedWiz 32 - SS   SPI (eviter si shield SPI) */ \
	\
	/* end */




#if (USE_MOUSE)
#define MOUSE_X_CLK_INDEX    9
#define MOUSE_X_DIR_INDEX   10
#define MOUSE_Y_CLK_INDEX   11
#define MOUSE_Y_DIR_INDEX   12
#endif

#define SHIFT_SWITCH_INDEX   10

/*
 * PANEL_MAPPING_TABLE - Boutons du panneau de controle
 * ======================================================
 * Syntaxe : _map_( PORT, bit, code_normal, code_shift )
 *
 *  PORT        : lettre du port AVR
 *  bit         : numero de bit du port (0 a 7)
 *  code_normal : code HID envoye (appui simple)
 *  code_shift  : code HID envoye (appui avec bouton SHIFT)
 *                0 = pas de fonction en mode shift
 *
 * Broches disponibles (Port F = A0-A7, Port L = L0/L2/L4) :
 *  A0  / D54  | F | 0 |   A4  / D58  | F | 4 |
 *  A1  / D55  | F | 1 |   A5  / D59  | F | 5 |
 *  A2  / D56  | F | 2 |   A6  / D60  | F | 6 |
 *  A3  / D57  | F | 3 |   A7  / D61  | F | 7 |
 *  D45        | L | 4 |   D47         | L | 2 |   D49 | L | 0 |
 *
 * -> Modifier les codes ci-dessous selon vos besoins
 */
#define PANEL_MAPPING_TABLE(_map_) \
	\
	_map_( F, 0,    MOD_LeftShift,      AC_VolumeDown      )  /* A0  / D54 */ \
	_map_( F, 1,    MOD_RightShift,     AC_VolumeUp        )  /* A1  / D55 */ \
	_map_( F, 2,    MOD_LeftControl,    AC_Mute            )  /* A2  / D56 */ \
	_map_( F, 3,    MOD_RightControl,   KEY_M              )  /* A3  / D57 */ \
	_map_( F, 4,    AG_Button1,         AG_Button5         )  /* A4  / D58 */ \
	_map_( F, 5,    AG_Button2,         AG_Button6         )  /* A5  / D59 */ \
	_map_( F, 6,    KEY_Esc,            0                  )  /* A6  / D60 */ \
	_map_( F, 7,    KEY_Enter,          0                  )  /* A7  / D61 */ \
	_map_( L, 0,    AG_Button3,         AG_Button7         )  /* D49        */ \
	_map_( L, 2,    AG_Button4,         AG_Button8         )  /* D47        */ \
	_map_( L, 4,    KEY_E,              0                  )  /* D45        */ \
	\
	/* end */

/*
 * ADC_MAPPING_TABLE - Entrees analogiques
 * =========================================
 * Syntaxe : _map_( PORT, bit, mux, val_min, val_max, joyid, axis )
 *
 *  PORT    : lettre du port AVR
 *  bit     : numero de bit du port
 *  mux     : selecteur ADC - 0x00..0x07 = A0..A7,  0x20..0x27 = A8..A15
 *  val_min : valeur min de calibration (depend de l'installation)
 *  val_max : valeur max de calibration
 *  joyid   : identifiant joystick (ID_AccelGyro, ...)
 *  axis    : axe joystick (0=X, 1=Y, 2=Z)
 *
 * Broches analogiques disponibles :
 *  A8  / D62  | K | 0 | mux 0x20  <- NIGHT_SWITCH par defaut (ne pas utiliser en ADC)
 *  A9  / D63  | K | 1 | mux 0x21
 *  A10 / D64  | K | 2 | mux 0x22
 *  A11 / D65  | K | 3 | mux 0x23
 *  A12 / D66  | K | 4 | mux 0x24
 *  A13 / D67  | K | 5 | mux 0x25
 *  A14 / D68  | K | 6 | mux 0x26
 *  A15 / D69  | K | 7 | mux 0x27  <- Plunger (potentiometre) par defaut
 *
 * Calibration : verifier avec les proprietes manette de jeu dans Windows.
 * Objectif : couvrir toute la plage de l'axe Z entre les deux positions du plunger.
 */

#define PLUNGER_MIN -1.5
#define PLUNGER_MAX 1.88

#define ADC_MAPPING_TABLE(_map_) \
	\
	_map_( K, 7, 0x27, PLUNGER_MIN, PLUNGER_MAX, ID_AccelGyro, 2 ) /* Analog Pin 15 Z   */ \
	\
	/* end */
