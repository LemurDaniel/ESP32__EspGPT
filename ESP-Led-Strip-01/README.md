# ESP-Led-Strip-01 🌈

WS2812-LED-Strip (144 LEDs) am **Waveshare ESP32-C6-Touch-LCD-1.47** (172×320 JD9853, Touch AXS5106L).
Bedienung über das Touch-Display: an / aus, Farbe, Effekt und ein täglicher Timer.

## Bedienung

Vier Zeilen von oben nach unten:

| Zeile | Bedienung | Wirkung |
| --- | --- | --- |
| **LICHT** | antippen | Strip an / aus |
| **FARBE** | Leiste ziehen | links Warmweiß und Weiß, danach der Farbkreis |
| **EFFEKT** | antippen | Einfarbig → Kerze → Atmen → Regenbogen |
| **TIMER** | antippen | aus → 4 h → 8 h → 12 h → aus |

Die Farbe wirkt bei Einfarbig und Atmen, Kerze und Regenbogen haben ihre eigenen Farben. Die Helligkeit ist fest (`LED_BRIGHTNESS` in `platformio.ini`). Die Einstellungen werden 3 s nach der letzten Änderung im NVS gespeichert.

**Timer (täglich):** Ab dem Einstellen leuchtet der Strip N Stunden, ist dann bis zum nächsten Tag aus und geht zur selben Uhrzeit wieder an. Die Taste zeigt z. B. `12 H` und `aus in 3:12`. LICHT schaltet zwischendurch von Hand um, beim nächsten Timer-Wechsel greift wieder der Plan.

Ohne Uhr ist das Einstellen der Bezugspunkt: Nach einem Stromausfall startet der Zyklus ab dem Einschalten neu, die Timer-Stufe bleibt gespeichert.

## Hardware

| Teil | Details |
| --- | --- |
| Board | Waveshare ESP32-C6-Touch-LCD-1.47, ESP32-C6FH8 (8 MB Flash), 160 MHz Single-Core |
| Strip | BTF-LIGHTING WS2812 ECO, 1 m, 144 LEDs, 5 V, GRB |
| Versorgung | USB-C über VBUS, Strom-Limit 500 mA in der Firmware |

### Verdrahtung

```
VBUS ───────────► Strip 5V
GND ────────────► Strip GND
GPIO 7 ─[330 Ω]─► Strip DIN   (Pfeilrichtung beachten!)
```

**Nicht GPIO 1, 2 oder 3:** Bei der Touch-Variante sind das die SPI-Leitungen von Display und TF-Karte.
GPIO 7 ist der einzige Header-Pin ohne Onboard-Funktion.

Für volle Helligkeit (bis ~8,6 A): externes 5-V-Netzteil ≥ 10 A, gemeinsame Masse, 1000-µF-Elko am Strip,
`LED_MAX_MILLIAMPS` anheben. Speist das Netzteil VBUS, darf **nicht gleichzeitig USB** stecken.

### Pinbelegung des Boards

Quelle: [docs.waveshare.com](https://docs.waveshare.com/ESP32-C6-Touch-LCD-1.47), Abschnitt „Pinout Definition“.

| Funktion | GPIO |
| --- | --- |
| LCD SCLK / MOSI | 1 / 2 (geteilt mit TF-Karte) |
| LCD CS / DC / RST / BL | 14 / 15 / 22 / 23 |
| TF MISO / CS | 3 / 4 |
| Touch + IMU I2C SDA / SCL | 18 / 19 |
| Touch INT / RST | 21 / 20 |
| IMU INT1 / INT2 | 5 / 6 |
| BOOT-Taste | 8 |
| Akku-ADC | 0 |
| USB D- / D+ | 12 / 13 |
| UART0 TX / RX | 16 / 17 |
| **Strip Data** | **7** (`LED_DATA_PIN` in `platformio.ini`) |

## Software

PlatformIO + Arduino-Core 3.x über [pioarduino](https://github.com/pioarduino/platform-espressif32), der offiziellen espressif32-Plattform fehlt die C6-Unterstützung.

```powershell
pio run -e waveshare-c6-touch-lcd147                     # Build
pio run -e waveshare-c6-touch-lcd147 --target upload     # Flashen
pio device monitor --baud 115200
```

Unter Windows: Long Paths aktivieren und aus **PowerShell** bauen. Hängt der Upload: BOOT halten → RST drücken → BOOT loslassen.

## Struktur

| Datei | Inhalt |
| --- | --- |
| `src/main.cpp` | Setup und Loop |
| `src/leds/StripController.*` | Effekte, Farbe, Strom-Limit, Persistenz |
| `src/display/LcdPanel.*` | JD9853-Display und Hintergrundbeleuchtung |
| `src/input/TouchPanel.*` | AXS5106L-Touch über I2C |
| `src/timer/DailyTimer.*` | Täglicher Timer 4 / 8 / 12 h |
| `src/ui/ControlScreen.*` | Bedienoberfläche: Tasten für Licht, Effekt und Timer, Farbleiste |

Display-Init und Touch-Protokoll stammen aus dem Waveshare-Demo (`ESP32-C6-Touch-LCD-1.47-Demo.zip`, Link im Wiki).

## Ideen für später

- Externes 5-V-Netzteil für volle Helligkeit
- Weitere Effekte, Display-Dimmen nach Inaktivität
- Gehäuse aus dem 3D-Drucker
