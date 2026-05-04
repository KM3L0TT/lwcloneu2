/*
 * LWCloneU2 - Communication Serie ESP32
 * [KM3L0TT] Protocole serie non-bloquant Mega/ProMicro <-> ESP32 Caisse
 *
 * Liaison physique : UART1 (D18=TX1 / D19=RX1 sur Mega, D1=TX / D0=RX sur ProMicro)
 * Vitesse          : definie par ESP32_SERIAL_BAUD dans devconfig.h (recommande 115200)
 *
 * ============================================================
 * TABLE DES TRAMES - Format : [ID] [TYPE] [OCTET3] [OCTET4]
 * ============================================================
 *
 * --- Identifiants (ID) ---
 * 0x0C  ESP32 Caisse    (hub central, commandes broadcast)
 * 0x1C  LedWiz 1        (Mega - caisse)
 * 0x0F  ESP32 Fronton   (relais ESP-NOW, non connecte au Mega)
 * 0x2F  LedWiz 2        (ProMicro 1 - fronton)
 * 0x3F  LedWiz 3        (ProMicro 2 - fronton)
 *
 * --- Sorties LedWiz [mode debug, au changement] ---
 * [ID] [0x10] [num 0x01..0x20] [pwm 0x00..0xFF]
 *   num  = numero sortie LedWiz (1-32)
 *   pwm  = valeur PWM courante (0=eteint, 255=max)
 *   Ex : sortie 1 a fond  -> 1C 10 01 FF
 *   Ex : sortie 5 a 50%   -> 1C 10 05 80
 *   Ex : sortie 1 eteinte -> 1C 10 01 00
 *
 * --- Entrees physiques [mode debug, au changement] ---
 * [ID] [0x20] [index 0x01..0xNN] [etat 0x00=relache / 0x01=appuye]
 *   index = numero d'entree selon PANEL_MAPPING_TABLE (1-base)
 *   Ex : entree 1 appuyee  -> 1C 20 01 01
 *   Ex : entree 1 relachee -> 1C 20 01 00
 *
 * --- Code HID envoye [mode debug, a l'appui uniquement] ---
 * [ID] [0x21] [index 0x01..0xNN] [code HID resolu]
 *   Envoye uniquement si code HID != 0 (certaines entrees n'ont pas de code HID)
 *   code = code HID resolu apres logique normal/shift
 *   Ex : entree 1 -> KEY_Esc  -> 1C 21 01 29
 *   Ex : entree 2 shifted     -> 1C 21 02 05
 *
 * --- Analogique [mode debug, si valeur change] ---
 * [ID] [0x30] [hi] [lo]   Plunger ADC (uint16 big-endian, 0-1023)
 * [ID] [0x31] [hi] [lo]   Gyro/Accel X (int16 big-endian)
 * [ID] [0x32] [hi] [lo]   Gyro/Accel Y (int16 big-endian)
 *   Ex : plunger 512  -> 1C 30 02 00
 *   Ex : gyro X +300  -> 1C 31 01 2C
 *   Ex : gyro Y -128  -> 1C 32 FF 80
 *
 * --- Systeme [toujours actif, toutes les 500ms] ---
 * [ID] [0xFF]             Heartbeat (2 octets)
 *   Ex : Mega        -> 1C FF
 *   Ex : ProMicro 1  -> 2F FF
 *   Ex : ProMicro 2  -> 3F FF
 *
 * --- Commandes ESP32 -> Esclaves [toujours actif, toutes les 500ms] ---
 * [0x0C] [0xAA] [0x00/0x01]   Night Mode OFF/ON  (3 octets)
 * [0x0C] [0xDD] [0x00/0x01]   Debug Mode OFF/ON  (3 octets)
 *   Ex : Night ON  -> 0C AA 01
 *   Ex : Night OFF -> 0C AA 00
 *   Ex : Debug ON  -> 0C DD 01
 *   Ex : Debug OFF -> 0C DD 00
 *
 * ============================================================
 * ACTIVATION dans devconfig.h :
 *   #define ESP32_SERIAL_BAUD  115200   <- active tout le module
 *   #define LEDWIZ_SERIAL_ID          0x1C     <- ID de cette carte
 * ============================================================
 */

#ifndef SERIAL_ESP32_H__INCLUDED
#define SERIAL_ESP32_H__INCLUDED

#include <stdint.h>

/* ── Types de trames ─────────────────────────────────────── */
#define ESP_TYPE_OUTPUTS    0x10  /* Sortie LedWiz  : num + pwm        */
#define ESP_TYPE_INPUT      0x20  /* Entree physique: index + etat      */
#define ESP_TYPE_HID        0x21  /* Code HID envoye: index + code      */
#define ESP_TYPE_ANALOG     0x30  /* Plunger ADC    : hi + lo           */
#define ESP_TYPE_GYRO_X     0x31  /* Gyro/Accel X   : hi + lo           */
#define ESP_TYPE_GYRO_Y     0x32  /* Gyro/Accel Y   : hi + lo           */
#define ESP_TYPE_HEARTBEAT  0xFF  /* Heartbeat      : ID + FF (2 oct)   */

/* ── Commandes ESP32 -> Esclaves ─────────────────────────── */
#define ESP_CMD_NIGHT       0xAA  /* Night Mode : valeur 0x00 ou 0x01   */
#define ESP_CMD_DEBUG       0xDD  /* Debug Mode : valeur 0x00 ou 0x01   */
#define ESP_ID_ESP32        0x0C  /* ID de l'ESP32 Caisse               */

/* ── Variables recues depuis l'ESP32 ─────────────────────── */
#if defined(ESP32_SERIAL_BAUD)

/* nightModeSerial : valeur du Mode Nuit recue via trame [0C AA 00/01]
 * Mise a jour automatiquement par serial_esp32_task()
 * Utilisee dans panel.c pour la logique de priorite nightMode */
extern volatile uint8_t nightModeSerial;

/* debugModeSerial : active/desactive l'envoi des trames debug (0x10..0x32)
 * Recue via trame [0C DD 00/01]
 * Si 0 : seul le heartbeat est envoye (defaut) */
extern volatile uint8_t debugModeSerial;

#endif /* ESP32_SERIAL_BAUD */

/* ── API publique ────────────────────────────────────────── */

/* Initialise UART1 a la vitesse ESP32_SERIAL_BAUD
 * A appeler dans hardware_init(), apres sei() */
void serial_esp32_init(void);

/* Tache principale non-bloquante - appeler dans la boucle principale
 *
 * Parametres (utilises uniquement si debugModeSerial == 1) :
 *   pwm_state   : tableau des valeurs PWM courantes (1 par sortie LedWiz)
 *   n_outputs   : nombre de sorties (= NUMBER_OF_LEDS)
 *   input_state : tableau des etats d'entrees (1 par entree, 0=relache/1=appuye)
 *   hid_state   : tableau des codes HID resolus (1 par entree, 0=pas de code)
 *   n_inputs    : nombre d'entrees (= NUMBER_OF_INPUTS)
 *   plunger     : valeur ADC plunger (0-1023), 0 si non utilise
 *   gyro_x      : acceleration X depuis mpu6050_ReadData(), 0 si non utilise
 *   gyro_y      : acceleration Y depuis mpu6050_ReadData(), 0 si non utilise
 */
void serial_esp32_task(
    const uint8_t *pwm_state,
    uint8_t        n_outputs,
    const uint8_t *input_state,
    const uint8_t *hid_state,
    uint8_t        n_inputs,
    uint16_t       plunger,
    int            gyro_x,
    int            gyro_y
);

#endif /* SERIAL_ESP32_H__INCLUDED */
