# ESP-Led-deko 🕯️

Kerzenflammen-Deko mit WS2812B-LEDs auf dem **Waveshare ESP32-C6-LCD-1.47** (172×320 ST7789, onboard RGB-LED).
Bedienung nur über die BOOT-Taste, das Display (kein Touch) zeigt den Zustand und eine Live-Vorschau der Flamme. Kein WLAN, kein Webserver.

## Bedienung

| Aktion | Wirkung |
| --- | --- |
| BOOT kurz drücken | Kerze an / aus |
| BOOT lang drücken (0,8 s) | Timer: aus → 4 h → 8 h → 12 h → aus |

**Timer (täglich):** Ab dem Einstellen brennt die Kerze N Stunden, ist dann bis zum nächsten Tag aus und geht zur selben Uhrzeit wieder an. Kurz drücken schaltet zwischendurch von Hand um, beim nächsten Timer-Wechsel greift wieder der Plan. Das Display zeigt z. B. `Timer 8h  aus in 3:12`.

Ohne Uhr und WLAN ist das Einstecken der Bezugspunkt: Nach einem Stromausfall startet der Zyklus ab dem Einschalten neu, die Timer-Stufe bleibt gespeichert. Die Gangabweichung des Quarzes liegt bei wenigen Sekunden pro Tag.

An/aus und Timer-Stufe werden im NVS gespeichert.

## Hardware

| Teil | Details |
| --- | --- |
| Board | Waveshare ESP32-C6 1.47" Display Dev Board, ESP32-C6FH8 (8 MB Flash), 160 MHz Single-Core |
| LEDs | Freenove 8 RGB LED Module (8× WS2812B) |
| Versorgung | USB-C, Strom-Limit 400 mA in der Firmware |

### Verdrahtung

| Freenove-Modul (Seite **IN**) | ESP-Pinleiste |
| --- | --- |
| S | 2 |
| V | 5V |
| G | GND |

Seite **OUT** bleibt frei (zum Verketten weiterer Module, dann `LED_COUNT` anpassen).

### Pinbelegung

| Funktion | GPIO | Hinweis |
| --- | --- | --- |
| LCD MOSI / SCLK | 6 / 7 | geteilt mit SD-Karte |
| LCD CS / DC / RST / BL | 14 / 15 / 21 / 22 | |
| Onboard RGB-LED | 8 | spiegelt LED 1 |
| BOOT-Taste | 9 | kurz an / aus, lang Timer |
| **LED-Modul Data** | **2** | `LED_DATA_PIN` in `platformio.ini` |

Header-Seite mit BOOT: `5V GND 3V3 0 1 2 3 4 5`. Meiden: 4, 5, 8, 9, 15 (Strapping), 12, 13 (USB), 6, 7, 14, 15, 21, 22 (LCD).

## Software

PlatformIO + Arduino-Core 3.x über [pioarduino](https://github.com/pioarduino/platform-espressif32), der offiziellen espressif32-Plattform fehlt die C6-Unterstützung.

```powershell
pio run -e waveshare-c6-lcd147                     # Build
pio run -e waveshare-c6-lcd147 --target upload     # Flashen
pio device monitor --baud 115200
```

Unter Windows: Long Paths aktivieren und aus **PowerShell** bauen. Der ESP-IDF-Tool-Installer verweigert Git Bash (MSys).

## Struktur

| Datei | Inhalt |
| --- | --- |
| `src/main.cpp` | Setup und Loop |
| `src/leds/LedController.*` | Kerzenflamme, Strom-Limit, Persistenz |
| `src/display/StatusDisplay.*` | Statusseite und Live-Vorschau als Ring |
| `src/input/BootButton.*` | Entprellte Taste mit kurzem und langem Druck |
| `src/timer/CandleTimer.*` | Täglicher Timer 4 / 8 / 12 h |

## Ideen für später

- Längerer WS2812B-Strip am 5V/3A-Netzteil (`LED_COUNT`, `LED_MAX_MILLIAMPS` anpassen)
- Alter analoger `+ G R B`-Strip über 3 MOSFETs (PWM)
