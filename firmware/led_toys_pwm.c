/*
 * LWCloneU2
 * Copyright (C) 2013 Andreas Dittrich <lwcloneu2@cithraidt.de>
 * Modified by Xavier Hosxe : real PWM Timer4 + silence mode
 * [KM3L0TT] Refactoring complet :
 *   - Nouvelle signature macro : _map_(PORT, bit, inv, night, pwm_off)
 *   - Suppression Timer4 hardware (non utilise sur ce PCB)
 *   - Suppression PWM_POWER_MAX / PWM_POWER_SAVING (reduction courant supprimee)
 *   - Mode Nuit par sortie (parametre 'night') : coupe si nightMode=1
 *   - Coupure securite par sortie (parametre 'pwm_off') : coupe apres N*~10ms
 *   - Exposition led_get_pwm_state() pour communication serie ESP32
 *
 * This program is free software; you can redistribute it and/or modify it under the terms of the
 * GNU General Public License as published by the Free Software Foundation;
 * either version 2 of the License, or (at your option) any later version.
 */

#include <stdint.h>
#include <avr/interrupt.h>
#include <avr/io.h>

#include <hwconfig.h>
#include "led.h"
#include "comm.h"

#if !defined(LED_TIMER_vect)
    void led_init(void) {}
    void led_update(uint8_t *p8bytes) {}
#else

/* [KM3L0TT] nightMode : defini dans panel.c
 * Mis a jour en temps reel par panel_ScanInput()
 * Sources : switch physique OU nightModeSerial (ESP32) */
#ifdef NIGHT_SWITCH_PORT
extern uint8_t nightMode;
#elif defined(ESP32_SERIAL_BAUD)
extern uint8_t nightMode;
#endif

/* [KM3L0TT] Index de chaque sortie dans les tableaux internes
 * Nouvelle signature : _map_(PORT, bit, inv, night, pwm_off) */
#define MAP(X, bit, inv, night, pwm_off) X##bit##_index,
enum { LED_MAPPING_TABLE(MAP) NUMBER_OF_LEDS };
#undef MAP

#define NUMBER_OF_BANKS  ((NUMBER_OF_LEDS + 7) / 8)
#define MAX_PWM          49

#if (NUMBER_OF_LEDS > 32)
    #error "Nombre de sorties LED superieur a 32 !"
#endif

/* [KM3L0TT] Structure etat de chaque sortie LedWiz
 * - enable  : 1=active, 0=inactive (commande USB)
 * - mode    : profil PWM envoye par le logiciel PC
 * - night   : 1=coupe en Mode Nuit, 0=toujours actif
 * - pwm_off : 0=pas de coupure, >0=coupure apres N*~10ms (securite) */
static struct {
    volatile uint8_t enable;
    volatile uint8_t mode;
    uint8_t          night;
    uint16_t         pwm_off;
} g_LED[NUMBER_OF_BANKS * 8];

/* [KM3L0TT] Compteur de securite par sortie
 * Incremente dans l'ISR toutes les ~10ms quand la sortie est active
 * Declenche la coupure quand ToyTimer[i] >= g_LED[i].pwm_off */
static uint16_t ToyTimer[NUMBER_OF_BANKS * 8];

/* [KM3L0TT] Buffer PWM courant - expose via led_get_pwm_state() */
static uint8_t g_pwm_current[NUMBER_OF_LEDS];

volatile uint16_t g_dt = 256;

uint16_t millis_ledTimersize  = 1600;
uint16_t millis_ledTimerOffAt = 800;
volatile unsigned long millis_timer;

unsigned long led_millis(void) {
    return millis_timer;
}

void led_setBlinkTimer(uint16_t size, uint16_t offAt) {
    millis_ledTimersize  = size;
    millis_ledTimerOffAt = offAt;
}

/* [KM3L0TT] Exposition des valeurs PWM courantes pour serial_esp32_task()
 * Copie les valeurs PWM dans le tableau fourni
 * Retourne le nombre de sorties copiees */
uint8_t led_get_pwm_state(uint8_t *out, uint8_t max_leds)
{
    uint8_t n = (NUMBER_OF_LEDS < max_leds) ? NUMBER_OF_LEDS : max_leds;
    for (uint8_t i = 0; i < n; i++) {
        out[i] = g_pwm_current[i];
    }
    return n;
}

static void update_state(uint8_t *p5bytes);
static void update_profile(int8_t k, uint8_t *p8bytes);
static void update_pwm(uint8_t *pwm, int8_t n, uint16_t t);
static void led_ports_init(void);


void led_init(void)
{
    led_ports_init();
    led_timer_init();

    /* [KM3L0TT] Initialisation de night et pwm_off depuis le pinmap */
    #define MAP(X, bit, inv, night, pwm_off) \
        g_LED[X##bit##_index].night   = night; \
        g_LED[X##bit##_index].pwm_off = pwm_off;
    LED_MAPPING_TABLE(MAP)
    #undef MAP
}


void led_update(uint8_t *p8bytes)
{
    static uint8_t nbank = 0;

    if (p8bytes[0] == 64) {
        update_state(p8bytes + 1);
        nbank = 0;
    } else {
        update_profile(nbank, p8bytes);
        nbank = (nbank + 1) & 0x03;
    }
}


static void update_state(uint8_t *p5bytes)
{
    for (int8_t k = 0; k < NUMBER_OF_BANKS; k++)
    {
        uint8_t b = p5bytes[k];
        for (int8_t i = 0; i < 8; i++)
        {
            uint8_t ledNumber = k * 8 + i;
            uint8_t enable = b & 0x01;

            if (enable && g_LED[ledNumber].enable == 0) {
                /* [KM3L0TT] RAZ du compteur de securite a chaque activation */
                ToyTimer[ledNumber] = 0;
                DbgOut(DBGINFO, "%i ON", ledNumber);
            } else if (!enable && g_LED[ledNumber].enable > 0) {
                DbgOut(DBGINFO, "%i off", ledNumber);
            }

            g_LED[ledNumber].enable = enable;
            b >>= 1;
        }
    }

    uint8_t pulse_speed = p5bytes[4];
    if (pulse_speed > 7) pulse_speed = 7;
    if (pulse_speed == 0) pulse_speed = 1;
    g_dt = pulse_speed * 128;
}


static void update_profile(int8_t k, uint8_t *p8bytes)
{
    if (k >= NUMBER_OF_BANKS) return;
    for (int8_t i = 0; i < 8; i++) {
        g_LED[k * 8 + i].mode = p8bytes[i];
    }
}


static void update_pwm(uint8_t *pwm, int8_t n, uint16_t t)
{
    for (int8_t i = 0; i < n; i++)
    {
        if (g_LED[i].enable == 0) {
            pwm[i] = 0;
            continue;
        }

        /* [KM3L0TT] Mode Nuit : verification par sortie en temps reel
         * Independant des commandes USB - reagit instantanement au switch */
        #if defined(NIGHT_SWITCH_PORT) || defined(ESP32_SERIAL_BAUD)
        if (nightMode && g_LED[i].night) {
            pwm[i] = 0;
            continue;
        }
        #endif

        uint8_t b = g_LED[i].mode;

        if (b <= MAX_PWM) {
            pwm[i] = b;
        } else if (b == 129) {
            /* Triangle */
            uint16_t x = t >> 8;
            if (x & 0x80) x = 255 - x;
            pwm[i] = (MAX_PWM * x) >> 7;
        } else if (b == 130) {
            /* Rectangle */
            pwm[i] = (t & 0x8000) ? MAX_PWM : 0;
        } else if (b == 131) {
            /* Descente */
            uint16_t x = 255 - (t >> 8);
            pwm[i] = (MAX_PWM * x) >> 8;
        } else if (b == 132) {
            /* Montee */
            uint16_t x = t >> 8;
            pwm[i] = (MAX_PWM * x) >> 8;
        } else {
            pwm[i] = 0;
        }
    }
}


ISR(LED_TIMER_vect)
{
    static int8_t  counter      = 0;
    static int8_t  counterMillis = 0;
    static uint16_t t            = 0;
    static uint8_t  pwm[NUMBER_OF_LEDS];

    /* LED de sante interne (D13) */
    counterMillis++;
    if (counterMillis == 5) {
        counterMillis = 0;
        millis_timer++;
        unsigned long ledCheck = millis_timer % millis_ledTimersize;
        if (ledCheck == 0)                   PORTB |= 0b10000000;
        else if (ledCheck == millis_ledTimerOffAt) PORTB &= 0b01111111;
    }

    counter--;

    if (counter < 0)
    {
        counter = MAX_PWM - 1;
        t += g_dt;

        /* [KM3L0TT] Coupure de securite automatique - execute toutes les ~10ms
         * Protege les solenoides si le logiciel plante et laisse une sortie active
         * Si pwm_off > 0 et sortie active depuis trop longtemps -> coupure */
        for (int8_t i = 0; i < NUMBER_OF_LEDS; i++) {
            if (g_LED[i].pwm_off > 0 && g_LED[i].enable) {
                ToyTimer[i]++;
                if (ToyTimer[i] >= g_LED[i].pwm_off) {
                    g_LED[i].enable = 0;
                    ToyTimer[i] = 0;
                    DbgOut(DBGINFO, "%i auto-off securite", i);
                }
            }
        }

        update_pwm(pwm, NUMBER_OF_LEDS, t);

        /* [KM3L0TT] Mise a jour du buffer expose pour serial_esp32_task() */
        for (int8_t i = 0; i < NUMBER_OF_LEDS; i++) {
            g_pwm_current[i] = pwm[i];
        }
    }

    /* Application soft-PWM sur toutes les sorties */
    #define MAP(X, bit, inv, night, pwm_off) \
    if ((pwm[X##bit##_index] > counter) == (!inv)) \
        { PORT##X |= (1 << bit); } \
    else \
        { PORT##X &= ~(1 << bit); }
    LED_MAPPING_TABLE(MAP)
    #undef MAP
}


static void led_ports_init(void)
{
    /* LED de sante interne D13 */
    DDRB |= 0b10000000;

    /* [KM3L0TT] Toutes les sorties LedWiz en sortie, etat initial bas */
    #define MAP(X, bit, inv, night, pwm_off) \
        DDR##X  |= (1 << bit); \
        PORT##X &= ~(1 << bit);
    LED_MAPPING_TABLE(MAP)
    #undef MAP
}

#endif /* LED_TIMER_vect */
