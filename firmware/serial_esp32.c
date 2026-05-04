/*
 * LWCloneU2 - Communication Serie ESP32
 * [KM3L0TT] Implementation non-bloquante Mega/ProMicro <-> ESP32
 *
 * Compatible ATmega2560 (Mega)    : UART1 = D18(TX1) / D19(RX1)
 * Compatible ATmega32U4 (ProMicro): UART1 = D1(TX)   / D0(RX)
 *
 * Le module est compile uniquement si ESP32_SERIAL_BAUD est defini
 * dans devconfig.h. Sans ce define, toutes les fonctions sont des stubs vides.
 */

#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>

#include <hwconfig.h>
#include "serial_esp32.h"
#include "devconfig.h"

/* ── Stubs si module desactive ───────────────────────────── */
#if !defined(ESP32_SERIAL_BAUD) || !defined(LEDWIZ_SERIAL_ID)

void serial_esp32_init(void) {}
void serial_esp32_task(const uint8_t *p, uint8_t n, const uint8_t *i,
                       const uint8_t *h, uint8_t ni, uint16_t a, int x, int y) {}

#else

/* ── Selection des registres UART1 selon le MCU ─────────── */
#if defined(__AVR_ATmega2560__)
    /* Mega 2560 : UART1 = D18(TX) / D19(RX) */
    #define UBRR_REG    UBRR1
    #define UCSR_A      UCSR1A
    #define UCSR_B      UCSR1B
    #define UCSR_C      UCSR1C
    #define UDR_REG     UDR1
    #define UDRIE_BIT   UDRIE1
    #define RXEN_BIT    RXEN1
    #define TXEN_BIT    TXEN1
    #define UCSZ0_BIT   UCSZ10
    #define UCSZ1_BIT   UCSZ11
    #define RXCIE_BIT   RXCIE1
    #define U2X_BIT     U2X1
    #define ISR_RX      USART1_RX_vect
    #define ISR_UDRE    USART1_UDRE_vect
#elif defined(__AVR_ATmega32U4__)
    /* Pro Micro : UART1 = D1(TX) / D0(RX) */
    #define UBRR_REG    UBRR1
    #define UCSR_A      UCSR1A
    #define UCSR_B      UCSR1B
    #define UCSR_C      UCSR1C
    #define UDR_REG     UDR1
    #define UDRIE_BIT   UDRIE1
    #define RXEN_BIT    RXEN1
    #define TXEN_BIT    TXEN1
    #define UCSZ0_BIT   UCSZ10
    #define UCSZ1_BIT   UCSZ11
    #define RXCIE_BIT   RXCIE1
    #define U2X_BIT     U2X1
    #define ISR_RX      USART1_RX_vect
    #define ISR_UDRE    USART1_UDRE_vect
#else
    #error "serial_esp32.c : MCU non supporte (ATmega2560 ou ATmega32U4 requis)"
#endif

/* ── Calcul UBRR (mode U2X = double vitesse pour precision) ─ */
#define BAUD_PRESCALE  ((uint16_t)((F_CPU / (8UL * ESP32_SERIAL_BAUD)) - 1))

/* ── Buffer TX circulaire (256 octets, taille = puissance de 2) ─ */
#define TX_BUF_SIZE  256
#define TX_BUF_MASK  (TX_BUF_SIZE - 1)
static volatile uint8_t tx_buf[TX_BUF_SIZE];
static volatile uint8_t tx_head = 0;
static volatile uint8_t tx_tail = 0;

/* ── Buffer RX (16 octets, suffit pour 1 trame de 3 octets) ─ */
#define RX_BUF_SIZE  16
#define RX_BUF_MASK  (RX_BUF_SIZE - 1)
static volatile uint8_t rx_buf[RX_BUF_SIZE];
static volatile uint8_t rx_head = 0;
static volatile uint8_t rx_tail = 0;

/* ── Variables exportees ─────────────────────────────────── */
/* [KM3L0TT] nightModeSerial : valeur Mode Nuit recue depuis ESP32
 * Mise a jour par process_rx() a chaque trame [0C AA 00/01]
 * Priorite : switchPhysique prend le dessus si ferme (voir panel.c) */
volatile uint8_t nightModeSerial = 0;

/* [KM3L0TT] debugModeSerial : gate pour les trames debug (0x10..0x32)
 * Si 0 : seul heartbeat toutes les 500ms
 * Si 1 : toutes les trames actives au changement d'etat */
volatile uint8_t debugModeSerial = 0;

/* ── ISR TX : envoie le prochain octet du buffer circulaire ─ */
ISR(ISR_UDRE)
{
    if (tx_tail != tx_head) {
        UDR_REG = tx_buf[tx_tail];
        tx_tail = (tx_tail + 1) & TX_BUF_MASK;
    } else {
        /* Buffer vide : desactiver l'interruption TX */
        UCSR_B &= ~(1 << UDRIE_BIT);
    }
}

/* ── ISR RX : stocke l'octet recu ───────────────────────── */
ISR(ISR_RX)
{
    uint8_t b = UDR_REG;
    uint8_t next = (rx_head + 1) & RX_BUF_MASK;
    if (next != rx_tail) {
        rx_buf[rx_head] = b;
        rx_head = next;
    }
    /* Si buffer plein : octet perdu, ne jamais bloquer en ISR */
}

/* ── Ecriture non-bloquante d'un octet dans le buffer TX ─── */
static void uart_put(uint8_t b)
{
    uint8_t next = (tx_head + 1) & TX_BUF_MASK;
    if (next == tx_tail) return; /* Buffer plein : abandon (non-bloquant) */
    tx_buf[tx_head] = b;
    tx_head = next;
    UCSR_B |= (1 << UDRIE_BIT); /* Activer l'ISR TX */
}

/* ── Envoi d'une trame 4 octets : [ID][TYPE][B3][B4] ───────
 * [KM3L0TT] Format standard pour sorties, entrees, HID, analogique */
static void send4(uint8_t type, uint8_t b3, uint8_t b4)
{
    uart_put(LEDWIZ_SERIAL_ID);
    uart_put(type);
    uart_put(b3);
    uart_put(b4);
}

/* ── Envoi heartbeat + commandes periodiques ESP32 (2-3 oct) */
/* [KM3L0TT] 2 octets pour heartbeat, 3 pour Night et Debug */
static void send_heartbeat(void)
{
    uart_put(LEDWIZ_SERIAL_ID);
    uart_put(ESP_TYPE_HEARTBEAT);
}

/* ── Traitement des trames recues depuis l'ESP32 ────────────
 * [KM3L0TT] Format attendu : [0C] [TYPE] [VALEUR]  (3 octets)
 * Parseur simple a 3 etats, non-bloquant */
static void process_rx(void)
{
    static uint8_t rx_state = 0; /* 0=attente ID, 1=attente type, 2=attente valeur */
    static uint8_t rx_type  = 0;

    while (rx_tail != rx_head)
    {
        uint8_t b = rx_buf[rx_tail];
        rx_tail = (rx_tail + 1) & RX_BUF_MASK;

        switch (rx_state)
        {
            case 0: /* Attente ID source = ESP32 Caisse */
                if (b == ESP_ID_ESP32) rx_state = 1;
                break;

            case 1: /* Type de commande */
                if (b == ESP_CMD_NIGHT || b == ESP_CMD_DEBUG) {
                    rx_type  = b;
                    rx_state = 2;
                } else {
                    rx_state = 0; /* Type inconnu, resynchronisation */
                }
                break;

            case 2: /* Valeur (0x00 ou 0x01) */
                if (rx_type == ESP_CMD_NIGHT) {
                    /* [KM3L0TT] nightModeSerial : priorite inferieure au switch physique
                     * La logique finale est dans panel_ScanInput() */
                    nightModeSerial = (b != 0) ? 1 : 0;
                } else if (rx_type == ESP_CMD_DEBUG) {
                    debugModeSerial = (b != 0) ? 1 : 0;
                }
                rx_state = 0;
                break;
        }
    }
}

/* ── Etats precedents pour detection de changement ─────────
 * [KM3L0TT] Envoi uniquement au changement (event-driven)
 * Economise la bande passante et evite de surcharger l'ESP32 */
static uint8_t  prev_pwm[32];    /* Valeurs PWM precedentes (max 32 sorties) */
static uint8_t  prev_input[16];  /* Etats entrees precedents (max 16 entrees) */
static uint16_t prev_plunger = 0xFFFF;
static int      prev_gyro_x  = 0x7FFF;
static int      prev_gyro_y  = 0x7FFF;

/* ── Initialisation UART1 ───────────────────────────────── */
void serial_esp32_init(void)
{
    /* Mode double vitesse pour meilleure precision baud */
    UCSR_A  = (1 << U2X_BIT);
    UBRR_REG = BAUD_PRESCALE;
    /* 8N1 : TX + RX actifs, interruption RX activee */
    UCSR_C  = (1 << UCSZ1_BIT) | (1 << UCSZ0_BIT);
    UCSR_B  = (1 << RXEN_BIT) | (1 << TXEN_BIT) | (1 << RXCIE_BIT);
}

/* ── Tache principale non-bloquante ─────────────────────── */
void serial_esp32_task(
    const uint8_t *pwm_state,
    uint8_t        n_outputs,
    const uint8_t *input_state,
    const uint8_t *hid_state,
    uint8_t        n_inputs,
    uint16_t       plunger,
    int            gyro_x,
    int            gyro_y)
{
    static uint32_t last_sys_ms = 0;

    /* Lire les trames entrantes a chaque appel */
    process_rx();

    /* Timestamp courant via led_millis() */
    extern unsigned long led_millis(void);
    uint32_t now = (uint32_t)led_millis();

    /* ── Trames systeme : heartbeat + Night + Debug (500ms) ── */
    if ((now - last_sys_ms) >= 500)
    {
        last_sys_ms = now;

        /* [KM3L0TT] Heartbeat : 2 octets, toujours envoye */
        send_heartbeat();

        /* [KM3L0TT] Night et Debug : 3 octets chacun, toujours envoyes
         * Permet a une carte qui reboot de resynchroniser en moins de 500ms */
        /* Note : ces trames sont generees par l'ESP32, pas par la Mega/ProMicro
         * La Mega/ProMicro ne fait QUE recevoir Night et Debug, pas les emettre.
         * Ce bloc est donc un no-op ici — inclus pour documentation. */
    }

    /* ── Trames debug : uniquement si debugModeSerial == 1 ── */
    /* ── Sorties forcees : toujours envoyees au changement   ── */

    /* --- Sorties LedWiz : envoi au changement de valeur PWM --- */
    /* [KM3L0TT] Deux modes d'envoi :
     *   1. debugModeSerial == 1 : toutes les sorties sont envoyees
     *   2. ADDR_FORCED_OUTPUTS defini dans pinmap.h :
     *      certaines sorties sont toujours envoyees au changement
     *      meme si debugModeSerial == 0 (ex: R/G/B/Beacon/Fan/Knocker
     *      pour l'ESP32 Fronton qui pilote les LEDs adressables)
     *
     * Format ADDR_FORCED_OUTPUTS dans pinmap.h :
     *   #define ADDR_FORCED_OUTPUTS { 5, 6, 7, 8, 9, 10, -1 }
     *   (numeros de sorties 1-base, termine par -1) */
    if (pwm_state && n_outputs > 0)
    {
        uint8_t lim = (n_outputs > 32) ? 32 : n_outputs;

        #if defined(ADDR_FORCED_OUTPUTS)
        static const int8_t forced[] = ADDR_FORCED_OUTPUTS;
        #endif

        for (uint8_t i = 0; i < lim; i++)
        {
            uint8_t v = pwm_state[i];
            if (v == prev_pwm[i]) continue; /* Pas de changement : rien a envoyer */

            /* Determiner si cette sortie doit etre envoyee */
            bool send = false;

            if (debugModeSerial) {
                send = true; /* Debug actif : toutes les sorties */
            }
            #if defined(ADDR_FORCED_OUTPUTS)
            else {
                /* Verifier si cette sortie est dans la liste forcee */
                for (uint8_t f = 0; forced[f] != -1; f++) {
                    if (i == (uint8_t)(forced[f] - 1)) {
                        send = true;
                        break;
                    }
                }
            }
            #endif

            if (send) {
                /* [ID][0x10][num 1-base][pwm 0-255] */
                send4(ESP_TYPE_OUTPUTS, (uint8_t)(i + 1), v);
            }

            /* Toujours mettre a jour prev_pwm pour detecter le prochain changement */
            prev_pwm[i] = v;
        }
    }

    /* ── Reste des trames debug : uniquement si debugModeSerial == 1 ── */
    if (!debugModeSerial) return;

    /* --- Entrees physiques + HID : envoi au changement d'etat --- */
    /* [KM3L0TT] 1 trame 0x20 par entree dont l'etat change
     *           1 trame 0x21 supplementaire si code HID present (appui uniquement) */
    if (input_state && n_inputs > 0)
    {
        uint8_t lim = (n_inputs > 16) ? 16 : n_inputs;
        for (uint8_t i = 0; i < lim; i++)
        {
            uint8_t s = (input_state[i] != 0) ? 1 : 0;
            if (s != prev_input[i]) {
                /* [ID][0x20][index 1-base][etat 0/1] */
                send4(ESP_TYPE_INPUT, (uint8_t)(i + 1), s);

                /* Code HID uniquement a l'appui et si code != 0 */
                if (s == 1 && hid_state && hid_state[i] != 0) {
                    /* [ID][0x21][index 1-base][code HID resolu] */
                    send4(ESP_TYPE_HID, (uint8_t)(i + 1), hid_state[i]);
                }

                prev_input[i] = s;
            }
        }
    }

    /* --- Analogique : envoi si valeur change --- */
    /* [KM3L0TT] Plunger : [ID][0x30][hi][lo] */
    if (plunger != prev_plunger) {
        send4(ESP_TYPE_ANALOG,
              (uint8_t)(plunger >> 8),
              (uint8_t)(plunger));
        prev_plunger = plunger;
    }

    /* [KM3L0TT] Gyro X : [ID][0x31][hi][lo] */
    if (gyro_x != prev_gyro_x) {
        send4(ESP_TYPE_GYRO_X,
              (uint8_t)((uint16_t)gyro_x >> 8),
              (uint8_t)((uint16_t)gyro_x));
        prev_gyro_x = gyro_x;
    }

    /* [KM3L0TT] Gyro Y : [ID][0x32][hi][lo] */
    if (gyro_y != prev_gyro_y) {
        send4(ESP_TYPE_GYRO_Y,
              (uint8_t)((uint16_t)gyro_y >> 8),
              (uint8_t)((uint16_t)gyro_y));
        prev_gyro_y = gyro_y;
    }
}

#endif /* ESP32_SERIAL_BAUD && LEDWIZ_SERIAL_ID */
