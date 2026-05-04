lwcloneu2 - KM3L0TT PinCab Edition
====================================

Firmware AVR pour controleur LED compatible LedWiz, avec support
joystick/clavier HID et communication serie vers hub ESP32.
Concu pour les cabinets de pinball virtuels (PinCab) mais utilisable
dans tout projet necessitant un controleur LED USB compatible LedWiz.


Fonctionnalites
===============

Sorties LED
-----------
- Jusqu'a 32 sorties LedWiz (Mega 2560)
- Jusqu'a 16 sorties LedWiz (Pro Micro / Leonardo)
- Soft-PWM uniforme sur toutes les sorties (pas de Timer4 hardware)
- Mode Nuit par sortie : coupe selectivement certaines sorties
  en mode nuit (parametre 'night' dans LED_MAPPING_TABLE)
- Coupure securite par sortie : protection des solenoides contre
  un maintien trop long (parametre 'pwm_off' en unites de ~10ms)

Entrees
-------
- Boutons joystick / clavier / souris HID (PANEL_MAPPING_TABLE)
- Plunger analogique (ADC_MAPPING_TABLE)
- Accelerometre MPU6050 (axes X/Y reportes comme joystick)
- Interrupteur Mode Nuit physique (active LOW, pull-up interne)

Mode Nuit
---------
- Interrupteur physique (NIGHT_SWITCH_PORT / NIGHT_SWITCH_BIT)
- Commande serie depuis ESP32 hub (nightModeSerial)
- Priorite : switch physique > commande serie > inactif
- Sur Pro Micro sans switch : pilotage serie uniquement

Communication Serie ESP32
--------------------------
- Protocole serie non-bloquant sur UART1 (115200 baud)
- Heartbeat toutes les 500ms
- Trames debug (sorties PWM, entrees, HID, plunger, gyro)
  activees a la demande via commande serie [0C DD 01]
- Sorties forcees (ADDR_FORCED_OUTPUTS) : certaines sorties
  toujours envoyees au changement, meme sans mode debug actif
  (ex : R/G/B/Beacon/Fan/Knocker pour LEDs adressables WS2812B)


Materiel Supporte
=================

- Arduino Mega 2560       (ATmega2560 + 16U2) -> LedWiz 1-32 + HID complet
- Arduino Pro Micro clone (ATmega32U4)         -> LedWiz 1-16, UART1 reserve
- Arduino Leonardo        (ATmega32U4)         -> LedWiz 1-16, HID complet
- Custom Breakout Board   (ATmega32U2)
- Arduino Uno Rev. 2/3    (non teste)


Configuration - pinmap.h
=========================

Syntaxe LED_MAPPING_TABLE
--------------------------
  _map_( PORT, bit, inv, night, pwm_off )

  PORT    : lettre du port AVR (A, B, C...)
  bit     : numero de bit du port (0 a 7)
  inv     : inversion logique  0=actif HIGH / 1=actif LOW
  night   : mode nuit          0=toujours actif / 1=coupe en mode nuit
  pwm_off : securite (x~10ms)  0=pas de coupure / 200=coupe apres ~2s

  Exemples :
    _map_( A, 0, 0, 0,   0 )  // LED simple        : toujours active
    _map_( A, 1, 0, 1,   0 )  // Lampe             : coupee en mode nuit
    _map_( A, 2, 0, 1, 200 )  // Solenoide         : mode nuit + securite 2s
    _map_( A, 3, 0, 0, 200 )  // Solenoide critique: toujours actif + securite 2s

Interrupteur Mode Nuit
-----------------------
  Cablez un interrupteur a bascule entre la broche choisie et GND.
  Pull-up interne active automatiquement.
    Interrupteur OUVERT = jouets actifs
    Interrupteur FERME  = mode nuit

  Dans pinmap.h :
    #define NIGHT_SWITCH_PORT   K    // lettre du port AVR
    #define NIGHT_SWITCH_BIT    0    // numero de bit

Sorties Forcees (LEDs adressables)
------------------------------------
  Permet d'envoyer certaines sorties en permanence vers l'ESP32
  meme si le mode debug est inactif. Utile pour piloter des LEDs
  adressables WS2812B (Beacon, Fan, Knocker) via l'ESP32 Fronton.

  Dans pinmap.h du Pro Micro 2 (LW3) :
    #define ADDR_FORCED_OUTPUTS { 1, 2, 3, 4, 5, 6, -1 }
    //                            R  G  B  Bea Fan Kno

  Dans pinmap.h du Mega (ex: Knocker a relayer vers ESP32 Fronton) :
    #define ADDR_FORCED_OUTPUTS { 10, -1 }


Communication Serie - Protocole ESP32
======================================

Materiel :
  Mega      : UART1 = D18 (TX1) / D19 (RX1)
  Pro Micro : UART1 = D1  (TX)  / D0  (RX)

Activation dans devconfig.h :
  #define ESP32_SERIAL_BAUD   115200
  #define LEDWIZ_SERIAL_ID    0x1C    // ID de cette carte

Identifiants (ID) :
  0x0C  ESP32 Caisse    (hub central, emet les commandes)
  0x1C  LedWiz 1        (Mega - caisse)
  0x0F  ESP32 Fronton   (relais ESP-NOW)
  0x2F  LedWiz 2        (Pro Micro 1 - fronton)
  0x3F  LedWiz 3        (Pro Micro 2 - fronton)

Format des trames :
  [ID] [TYPE] [OCTET3] [OCTET4]   4 octets (sauf exceptions)
  Envoi au changement d'etat, sauf trames systeme (500ms)

TYPE 0x10 - Sortie LedWiz  [debug ou forcee, au changement]
  [ID] [10] [num 01..20] [pwm 00..FF]
  Ex : sortie 1 a fond   -> 1C 10 01 FF
  Ex : sortie 5 a 50%    -> 1C 10 05 80
  Ex : sortie 1 eteinte  -> 1C 10 01 00

TYPE 0x20 - Entree physique  [debug, au changement]
  [ID] [20] [index 01..NN] [etat 00=relache / 01=appuye]
  Ex : entree 1 appuyee  -> 1C 20 01 01
  Ex : entree 1 relachee -> 1C 20 01 00

TYPE 0x21 - Code HID envoye  [debug, a l'appui uniquement]
  [ID] [21] [index 01..NN] [code HID resolu]
  Envoye seulement si code HID != 0
  Ex : entree 1 -> KEY_Esc    -> 1C 21 01 29
  Ex : entree 2 shifted        -> 1C 21 02 05

TYPE 0x30 - Plunger ADC  [debug, si valeur change]
TYPE 0x31 - Gyro/Accel X  [debug, si valeur change]
TYPE 0x32 - Gyro/Accel Y  [debug, si valeur change]
  [ID] [TYPE] [hi] [lo]   uint16 / int16 big-endian
  Ex : plunger 512  -> 1C 30 02 00
  Ex : gyro X +300  -> 1C 31 01 2C
  Ex : gyro Y -128  -> 1C 32 FF 80

TYPE 0xFF - Heartbeat  [toujours actif, toutes les 500ms]
  [ID] [FF]   (2 octets)
  Ex : Mega        -> 1C FF
  Ex : Pro Micro 1 -> 2F FF
  Ex : Pro Micro 2 -> 3F FF

Commandes ESP32 -> Esclaves  [toujours actif, toutes les 500ms]
  [0C] [AA] [00/01]   Night Mode OFF/ON  (3 octets)
  [0C] [DD] [00/01]   Debug Mode OFF/ON  (3 octets)
  Ex : Night ON  -> 0C AA 01
  Ex : Night OFF -> 0C AA 00
  Ex : Debug ON  -> 0C DD 01
  Ex : Debug OFF -> 0C DD 00

ESP-NOW Relay (ESP32 Caisse -> ESP32 Fronton) :
  L'ESP32 Caisse relaie en broadcast les trames 0x10 recues du Mega
  vers l'ESP32 Fronton, avec l'ID source d'origine conserve.
  Permet a l'ESP32 Fronton de recevoir des donnees de plusieurs
  LedWiz sans connexion serie directe.
  Ex : Knocker Mega (0x1C) -> Serial -> ESP32 Caisse -> ESP-NOW -> ESP32 Fronton


Compilation
===========

Prerequis :
  AVR Toolchain (avr-gcc, avr-libc) + LUFA framework

Recuperer les sources :
  git clone <repo>
  git submodule update --init

Compiler tous les firmwares :
  make

Flasher le Mega (ATmega2560) :
  avrdude -c wiring -p atmega2560 -P comXX -b 115200 -U flash:w:m2560/arduino_mega2560__m2560.hex:i

Flasher le 16U2 (USB du Mega) :
  Utiliser l'outil Flip d'Atmel avec arduino_mega2560__m16u2.hex
  Pour revenir a l'Arduino original :
  Arduino-usbserial-atmega16u2-Mega2560-Rev3.hex


Credits
=======

- Andreas Dittrich  : firmware LWCloneU2 original
                      https://github.com/evilwombat/lwcloneu2

- Xavier Hosxe      : integration accelerometre MPU6050

- KM3L0TT           : modifications et extensions du firmware
                      (refactoring LED mapping, mode nuit par sortie,
                      coupure securite, communication serie ESP32,
                      support Pro Micro, protocole hub PinCab)
