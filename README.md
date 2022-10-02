# Smart Meter Monitor
M-Bus Adapter für Smart Meter Sagemcom T210-D der Netz-NOE mit galvanischer Trennung und Übertragung der Daten ins WLAN.


ACHTUNG: Alle hier eingestellten Informationen erfolgen ohne Gewähr. Dies ist keine Nachbauanleitung, sonder beschreibt lediglich wie ich an diese Projekt herangegangen bin.

![grafik](https://user-images.githubusercontent.com/59536966/193453709-8a2b42c1-649f-40e6-8e9b-00a1a951b350.png)

Der M-Bus Adapter nutzt die Tatsache, dass das Smart Meter keine Eingaben erfordert. Dadurch genügt die einfache Ansteuerung eines Optokopplers. Die M-Bus Leitungen müssen polrichtig angeschlossen werden. Eine Vertauschung ist unkritisch, ergibt jedoch keine Funktion.

Um ein sauberes Signal sicherzustellen wird ein Schmitt-Trigger benutzt. Die überzähligen Gatter können mittels Jumper für eine Ledanzeige genutzt werden.

Der Optokoppler benötigt eine Stromversorgung zwischen 3V3 und 5V. Der Pegel des Ausgangssignals liegt dann ebenfalls in diesem Bereich.

Der M-Bus Transmitter ist getrennt vom Adapter aufgebaut und dient zur drahtlosen Übertragung der Daten. Es kann hier jeder ESP benutzt werden, die Platine ist für einen ESP-12F ausgelegt, der sich gerade in der Bastelkiste befand. Auf der Platine wurden auch zwei Schalter integriert um sie auch zum Flashen des ESP verwenden zu können.

Die Schalpläne und Platinen wurden mit Eagle gezeichnet.

Die Software für den Transmitter wurde mit der Arduino IDE erstellt. Die Daten werden von der M-Bus Schnittstelle entgegengenommen und mittels UDP an eine beliebige Zieladresse im WLAN weitergeleitet. Wenn kein WLAN gefunden wird startet der der Transmitter als AccessPoint (IP: 192.168.4.1). Die Konfiguration kan dann über ein Webinterface eingestellt werden. Die Software unterstützt OTA und Debugging über UDP.

Die Daten werden bei mir von einem Raspberry 3B zur Decodierung und Aufbereitung entgegengenommen. Ein entsprechendes Python-Programm liegt als Beispiel bei.
