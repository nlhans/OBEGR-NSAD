
Dit document beschrijft hoe je de IKEA OBEGRÄNSAD kan modden om met een Raspberry Pi Pico te laten werken. De IKEA OBEGRÄNSAD is een 16x16 LED matrix met witte LEDs die je aan en uit kan zetten. Als we deze modden kunnen we hier zelf leuke animaties op laten zien.
[foto wenselijk]
## Wat zit er in?
Als je de IKEA OBEGRÄNSAD openmaakt, dan zal je zien dat er 4 losse borden zitten die met elkaar zijn doorverbonden:

![Internals](https://github.com/nlhans/OBEGR-NSAD/blob/main/img/Pasted%20image%2020251201153451.png?raw=true)

Zoals je ziet zitten er op elke borden 4 chips met de benaming "SCT2024". Dit zijn de LED driver chips die we straks moeten aansturen. Op het onderste bord (#1) zit ook nog een losse extra chip: dit is een kleine (onbekende) microcontroller die IKEA heeft geprogrammeerd om wat standaard patronen te laten zien. Tenslotte zit er op bord 1 ook een aansluiting voor de USB voedingskabel. Men heeft daar nog een klikschakelaar gezet zodat je de voedingslijn aan of af kan schakelen.
## Borden aanpassen
De borden bevatten standaard al een microcontroller die de LED matrixen aansturen. Als we zelf patronen op deze borden willen zetten, dan zou het eerste idee zijn om deze chip te herprogrammeren. Echter helaas levert het Googlen naar dit onderdeelnr geen extra informatie op wat voor chip dit is. Zonder te weten wat daarin zit kunnen we er ook geen firmware voor schrijven.

De andere optie is om de chip te verwijderen en vervolgens met externe bedrading de matrix borden aansturen. Hieronder zie je hoe we dat kunnen doen. We moeten 6 draden aansluiten. op de pads op bord 1 (let op dat deze gemarkeerd zijn als "IN"). Vervolgens moeten we de microcontroller op het bord desolderen.
Het desolderen gaat het makkelijkst met een zogenaamd *hot air rework station*. Dit is (de)solderen met behulp van een hete luchtstroom. Het voordeel van deze techniek is dat je gelijkmatig alle solderingen warm kan maken, waardoor je de chip zonder schade aan de print kan verwijderen.

![Chips met verbinding van signalen](https://github.com/nlhans/OBEGR-NSAD/blob/main/img/Pasted%20image%20251201153841.png?raw=true)

## Signalen uitzoeken van LED matrix & aansluiten op de Pico
Zoals gezegd gebruiken deze borden de STC2024 LED driver chip. De solderingen op de vorige foto zijn ook directe verbindingen naar de eerste driver chip. Maar wat doet deze chip eigenlijk, en hoe werkt die?

De datasheet van deze chip kan je vinden op: http://www.starchips.com.tw/pdf/datasheet/SCT2024V01_03.pdf

De volledige benaming luid: "16-bit Serial-In/Parallel-Out Constant-Current LED Driver".. een hele mond vol. Maar laten we het even opbreken in stukjes:
- 16-bit: Dit wil zeggen dat de chip 16 kanalen individueel kan aansturen, ofwel 16 losse LEDs.
  
- Serial-In/Parallel-Out: De chip werkt als een soort schuifregister. Hierin moet je data inklokken samen met een kloksignaal. Vervolgens stuurt de chip deze signalen dan parallel uit. Later leg ik uit hoe dit exact werkt.
  
- Constant-Current LED Driver: Dit is de meest stabiele manier om een LED aan te sturen. LEDs zijn namelijk stroomgestuurde onderdelen, dus om een LED feller te laten branden moet je de stroom verhogen. De spanning over de LED blijft echter nagenoeg gelijk, dus dat is niet een hele betrouwbare manier om ze aan te sturen. Deze driver chip is zo ontworpen dat die de stroom per kanaal regelt en minder afhankelijk is van de voedingspanning.

En deze onderdelen kunnen we ook zien intern:

![STC2024 Chip Werking](https://github.com/nlhans/OBEGR-NSAD/blob/main/img/STC2024Device.drawio.png?raw=true)

Wat een leuke truuk van deze drivers is dat je ze achter elkaar kan schakelen. Zoals je ziet hebben we 4 signalen links als ingangen, maar is er ook 1 uitgang signaal. De chip werkt als een schuifregister: elke keer als er een puls op CLK wordt gegeven, schuift alle data door de chip heen. Die data komt vanaf pin SDI en komt er weer uit op SDO. Dit signaal kunnen we dus doorlussen om meerdere drivers (en borden!) achter elkaar te zetten. In dit geval hebben we 4 drivers per bord en 4 borden.. 16x4x4 = 256 LEDs. Dat is toevallig evenveel als de 16x16 matrix die we hebben :-)

De signalen zien er dan zo uit:
![LED chain](https://github.com/nlhans/OBEGR-NSAD/blob/main/img/STC2024Chain.drawio.png?raw=true)

## Signalen aansluiten
De signalen die we op de Pico dus moeten aansluiten zijn:
- VCC
- GND
- CLA
- CLK
- DI
- EN

**LET OP! De VCC op deze matrix borden zijn 5V. De Raspberry Pico werkt enkel op 3.3V.**  We kunnen wel digitale signalen naar de borden sturen, en meestal zal een 5V apparaat wel 3.3V signalen begrijpen. Echter let goed op hoe je de borden voedingspanning geeft. Je kan dit doen door of de borden met de losse USB kabel te voeden (verbind wel altijd de GND door), of door VCC op de 5V van de Pico aan te sluiten.

Deze namen staan voor:
- CLK: Het klok signaal van de schuifregisters
- DI: Data ingang voor de eerste schuifregister van de chain. Deze sturen we tegelijk met de klok aan. In de datasheet heet dit signaal "SDI"
- CLA: Deze is wat cryptisch, maar staat voor "LATCH". Dit is een soort luikje waardoor we rustig alle bits door de schuifregisters kunnen klokken, en pas als we de latch bedienen worden ze allemaal door gezet. In de datasheet heet dit signaal "LA/"
- EN: Hiermee kunnen we alle LEDs tegelijk aan en uit zetten. In de datasheet heet dit signaal "OE/"

In mijn geval heb ik ze aangesloten op de Pico bij:
- CLA: GPIO 15
- CLK: GPIO 14
- DI: GPIO 16
- EN: GPIO 17
Op de Pico kunnen we deze GPIOs zo opstarten:
```
#define PIN_CLA   15
#define PIN_CLK   14
#define PIN_DI    16
#define PIN_EN    17

void setup() {
  pinMode(PIN_CLA, OUTPUT);
  pinMode(PIN_CLK, OUTPUT);
  pinMode(PIN_DI, OUTPUT);
  pinMode(PIN_EN, OUTPUT);
}
```

De datasheet laat zien hoe we de signalen moeten aansturen:
![[Screenshot 2025-12-01 at 16.12.01.png]]

Je ziet hier de klokpulsen voorbij komen. Voor 1 chip zijn dit 15 stuks. Telkens als de klok omhoog gaat wordt er een nieuwe bit ingeklokt. Het is dus belangrijk deze bit al op SDI is weggeschreven voordat we de klok hoog maken. Tenslotte als we klaar zijn zetten we 1 puls op  de latch om alle signalen vast te leggen.

In de code ziet dit er zo uit:
```
void shiftPixels(uint8_t* buffer, size_t count) {
  digitalWrite(PIN_CLA, LOW);
  delayMicroseconds(1);
  for (uint_fast8_t i = 0; i < 16; i++) {
    for (uint_fast8_t j = 0; j < 16; j++) {
      digitalWrite(PIN_DI, buffer[i*16+j]>0?HIGH:LOW);
      delayMicroseconds(1);
      digitalWrite(PIN_CLK, HIGH);
      delayMicroseconds(1);
      digitalWrite(PIN_CLK, LOW);
      delayMicroseconds(1);
    }
    delayMicroseconds(1);
  }
  digitalWrite(PIN_CLA, HIGH);
  delayMicroseconds(1);
  digitalWrite(PIN_CLA, LOW);
  delayMicroseconds(1);
  digitalWrite(PIN_EN, HIGH);
  delayMicroseconds(1);
  digitalWrite(PIN_EN, LOW);
}
```

Deze functie neemt een array van pixels aan (`buffer`) en stuurt deze uit. De variabel `count`is hier niet gebruikt, maar dat zou natuurlijk wel kunnen. Er worden hier 16x16=256 bits uitgestuurd.

In de code staan verspreid delayMicroseconds, omdat anders de Pico te snel is voor de drivers chips om alles in te klokken.

## Patroontjes sturen
We kunnen dus patroontjes naar het scherm sturen met bijvoorbeeld:
```
#define PX (16*16)
void loop() {
  uint8_t buffer[PX];
  for (int i = 0; i < PX; i++) {
    buffer[i] = i%2;
  }
  shiftPixels(buffer, PX);
  delay(1000);
}
```

Deze code maakt een pixelbuffer aan van 256 pixels. Vervolgens zetten we in de for loop elke pixel waarde op `i%2`. Dit staat voor modulo 2, wat neerkomt of we kijken of `i` een even of oneven getal is. Elke oneven pixel wordt dan dus actief.

[Foto]

## Pixel mapping
Als we dit naar het scherm krijgen zal je zien dat we een raster krijgen van diagonale lijnen op het scherm. Waarom is dat? Elke rij bevat namelijk een even aantal pixels, dus zouden we dan niet verticale lijnen moeten krijgen?

Dat komt op de manier hoe de pixels op het bord zijn geplaatst. We hebben namelijk nog nergens gesproken over hoe 256 LEDs achter elkaar vertalen naar een 16x16 grid. Hier gaf ik eigenlijk al een beetje een voorproefje met deze tekening:

![LED chain](https://github.com/nlhans/OBEGR-NSAD/blob/main/img/STC2024Chain.drawio.png?raw=true)

Alle drivers zijn namelijk tegen de klok in aan elkaar gelust. Vervolgens zijn de 16 LEDs ook in 2 groepjes van 8 op deze manier aan elkaar gelust. Als we dit willen vertalen naar een nette scherm van 16x16, dan moeten we dus rekening houden waar elke pixel op ons scherm fysiek zit, en op welke plek die zit in de schuifregister chain!

Dat is wat puzzelwerk om uit te zoeken. Als je dat niet wil dan kan je deze code gebruiken om van een X-Y coordinaat de positie in de pixel buffer uit te rekenen:

```
uint8_t mapXY(int8_t x, int8_t y) {
  uint8_t px = (y*16+x);

  uint8_t board = (px / 64);
  uint8_t boardPx = px % 64;
  uint8_t boardPx8 = px % 8;
  
  
  if (boardPx >= 0 && boardPx < 8)   return board*64 + 0  + boardPx8;
  if (boardPx >= 8 && boardPx < 16)  return board*64 + 16 + boardPx8;
  if (boardPx >= 16 && boardPx < 24)  return board*64 + 15 - boardPx8;
  if (boardPx >= 24 && boardPx < 32)  return board*64 + 31 - boardPx8;
  
  if (boardPx >= 32 && boardPx < 40)  return board*64 + 48  + boardPx8;
  if (boardPx >= 40 && boardPx < 48)  return board*64 + 32 + boardPx8;
  if (boardPx >= 48 && boardPx < 56)  return board*64 + 63 - boardPx8;
  if (boardPx >= 56 && boardPx < 64)  return board*64 + 47 - boardPx8;
  
  return 0;
}
```

Als we dan dus mapXY(5,7) tikken, dan rekent deze functie direct uit welke positie dat is in onze schuifregister.

## Aangepast patroon
Gaan we dus weer terug naar het maken van patronen,  dan krijgen we een nieuwe code:

```
#define PX (16*16)
void loop() {
  uint8_t buffer[PX];
  for (int x = 0; x < 16; x++) {
    for (int y = 0; y < 16; y++) {
      buffer[mapXY(x,y)] = x%2;
    }
  }
  shiftPixels(buffer, PX);
  delay(1000);
}
```

We doen nu net zoals voorheen dat elke oneven pixel wordt aangezet. Deze positie slaan we dus echter op middels de mapXY() functie op de juiste plek in de buffer.
Natuurlijk kan je deze loop aanpassen om nog andere patroontjes op het scherm te zetten. Misschien ook wel vaste bitmaps heen sturen (rekening houdend met vertaling van mapXY), enzovoort.

## En verder..
Met deze code zou je pixels en patroontjes op de IKEA OBEGRÄNSAD zetten.

Natuurlijk is een volgende leuk stap om meerdere borden aan elkaar te schakelen. Dan moeten we niet alleen de pixels binnen 1 scherm goed mappen, maar ook over meerdere borden. En dat hangt maar net af hoe we die borden plaatsen.

Het is natuurlijk leuk als er dan nog meer animaties op komen:
- Tekst tonen met behulp van fonts
- Klok
- Scrollende tekst
- MQTT status weergave
- Patronen tekenen vanuit de browser en versturen over WiFi (upgraden naar Pico W!)
- Vuurwerk animaties
- 2-player Pong
- [Game of Life](https://en.wikipedia.org/wiki/Conway%27s_Game_of_Life)
- 3, 4 of 5-bit Grayscale dimming ??
