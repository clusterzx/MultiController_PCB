
# MultiController_PCB
Design Files für Review Auftrag.


**Ziel des Controllers:**

 1. Steuerung von Geräten über PWM 0-10V und/oder DAC 0-10V . Die Geräte bekommen ihre Spannungsversorgugn von einer externen Quelle (somit reines Steuersignal)
 2. Steuerung von handelsüblichen PC Fans (PWM) über den Molexanschluss (müssten 12V sein).
 3. Steuerung der PWM Signale unabhängig von einander. Bedeutet z.B. das auf RJ Jack 1 PWM 50% liegen und auf RJ 2 nichts dafür auf RJ 3-4 100%.
 4. Steuerung des Terminalblocks 3,5,12V (an/aus) (dort liegt immer nur 1 Verbraucher an)
 5. Anschluss eines optionalen OLED Displays und steuerung über ESP32

**ChatGPT generierte Zusammenfassung des Auftragschats:**
## Zusammenfassung

Dieses Dokument fasst die finalen Funktionen und Änderungen für das Multicontroller PCB-Projekt zusammen.

## Finalisierte Funktionen

1. **Spannungsversorgung**:
   - USB-C als primäre Stromquelle mit Power Delivery (PD) für 12V.
   - Step-Down-Konverter auf 5V und ein LDO (Low Dropout Regulator) auf 3.3V.
   - Optionaler Boost-Konverter für zusätzliche 12V, falls mehr Leistung benötigt wird.
   - **Micro-USB-Anschluss** nur zum Programmieren (nicht bestückt in der finalen Kundenversion).

2. **RJ14-Anschlüsse (6P4C)**:
   - Steuerung von Geräten über PWM und GND.
   - Modularer RJ14-Anschluss (6P4C) für Kompatibilität mit Standardkabeln (1:1 oder crossed).

3. **Schraubklemmen für stärkere Stromverbindungen**:
   - Verwendung von Phoenix-Klemmen, um dickere Kabel (z.B. AWG 14) anzuschließen.
   - Schraubklemmen mit 3.5mm Pitch für Zuverlässigkeit und eine robuste Verbindung.

4. **ESP32-S3 Mikrocontroller**:
   - Externer DAC hinzugefügt, da der ESP32-S3 keinen integrierten DAC hat.
   - Reset- und Boot-Knöpfe am Board integriert.
   - Alle benötigten GPIOs auf PWM-Kompatibilität überprüft.

5. **Steuerung von PC-Lüftern und anderen Geräten**:
   - 12V-Anschluss für PC-Lüftersteuerung (optional per Software schaltbar).
   - Mehrere PWM-Ausgänge sowie ein analoger Modus über eine Operationsverstärker- und Kondensator-Schaltung.

6. **Montagemöglichkeiten und Board-Größe**:
   - Boardgröße: 60 x 80 mm.
   - Befestigungslöcher an allen vier Ecken für einfache Gehäusemontage.
   - Layout so gestaltet, dass ein Gehäuse einfach konstruiert werden kann.

## Wichtige Änderungen im Verlauf

- **Änderung des PD-Controllers**: Der ursprünglich geplante PD-Controller (AP33771C) war nicht verfügbar; daher Wechsel zum besser verfügbaren HUSB238.
- **Änderungen an den RJ14-Buchsen**: Diskussion über verschiedene RJ-Typen (RJ11, RJ12, RJ14); endgültige Entscheidung für RJ14 mit 6P4C für bessere Kompatibilität.
- **Überarbeitung der Schraubklemmen**: Zunächst wurden filigrane Klemmen verwendet; später durch robustere Phoenix-Schraubklemmen ersetzt.
- **Dual-Assembly-Option**: Ursprünglich wurden zwei mögliche Bestückungsoptionen (JLCPCB und Aisler) in Betracht gezogen; aufgrund des Preisvorteils letztendlich für JLCPCB entschieden.
- **Optimierung des KiCad-Projekts**: Letzte Anpassungen und Testpunkte (TP) wurden geklärt; Bauteilzuordnungen für die Bestellung bei JLCPCB überprüft.

---

Diese Zusammenfassung bietet eine vollständige Übersicht über die finalen Funktionen und Änderungen, die im Verlauf des Projekts vorgenommen wurden.
