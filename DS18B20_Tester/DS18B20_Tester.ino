/*  Slovensko   */
/*Program napisan dne 10. 3. 2024 za potrebe domače uporabe in branja fizičnih 16 byte-nih naslovov
DS18B20 temperaturnih senzorjev in za preizkušanje njihovega delovanja. Namen je še dodat funkcije 
branja načina delovanja senzorja in testiranje parazitskega napajanja. Po potrebi še bo dodana 
funkcija nastavljanja ločljivosti senzorja. Koda bo kmalu komentirana v angleščini.
Več podrobnosti na mojem Githubu: https://github.com/ZanPekosak 
Ali preko maila: pekosak.zan@gmail.com */

/*  English   */
/*This code was written on the 10th of March, 2024 for the means of home lab testing and reading the
physical 16 byte sensor addresses of the DS18B20 temperature sensor and basic testing of their operation.
I plan to add the function of detecting parasitic sensor power and its testing, as the library supports
it by default. I also plan on adding resolution setting option in the code and LCD menus. Code comments
should follow soon.
More details at my Github page: https://github.com/ZanPekosak 
Or over mail at: pekosak.zan@gmail.com */

#include <OneWire.h>
#include <DallasTemperature.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "LowPower.h"

#define ONE_WIRE_BUS 4

OneWire oneWire(ONE_WIRE_BUS);

DallasTemperature sensors(&oneWire);
LiquidCrystal_I2C lcd(0x27, 16, 2);

DeviceAddress insideThermometer;

bool fc = 0;
bool fc2 = 0;
bool fc3 = 0;
bool doonce = 0;
bool doonce2 = 0;
bool wakeupState = 0;
volatile bool menu = 0;
volatile bool confirmState = 0;
const int confirmPin = 2;
const int nextPin = 3;
const int relay = 9;
const int wakeUpPin = 2;
const unsigned long powerDownTime = 30000;
const unsigned long refreshRate = 500;
unsigned long currentMillis = millis();
unsigned long timerMillis = millis();

void confirm() {
  if (digitalRead(confirmPin) == 0) {
    confirmState = 1;
  }
}

void next() {
  if (digitalRead(nextPin) == 0) {
    menu = !menu;
  }
}

void wakeUp() {
  wakeupState = 1;
}

void startTest(DeviceAddress &deviceAddress) {
  sensors.begin();
  sensors.setResolution(insideThermometer, 9);
  if (!sensors.getAddress(deviceAddress, 0)) {
    Serial.println("Unable to find address for Device 0");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Ni senzorja");
    lcd.setCursor(0, 1);
    lcd.print("PRAVILNA VEZAVA?");
  }
}

void printAddress(DeviceAddress deviceAddress) {
  for (uint8_t i = 0; i < 8; i++) {
    if (deviceAddress[i] < 16) Serial.print("0");
    Serial.print(deviceAddress[i], HEX);
  }
  Serial.println();
  char buffer[20];
  sprintf(buffer, "%X%X%X%X%X%X%X%X", deviceAddress[0],
          deviceAddress[1],
          deviceAddress[2],
          deviceAddress[3],
          deviceAddress[4],
          deviceAddress[5],
          deviceAddress[6],
          deviceAddress[7]);
  delay(80);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Adresa senzorja:");
  lcd.setCursor(0, 1);
  lcd.print(buffer);
}

void printTemperature(DeviceAddress deviceAddress) {
  float temperatura = sensors.getTempC(deviceAddress);
  Serial.print("Requesting temperatures...");
  sensors.requestTemperatures();
  Serial.print("Temperatura: ");
  Serial.print(temperatura);
  Serial.print(" °C\n");

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Temperatura:");
  lcd.setCursor(0, 1);
  lcd.print(temperatura);
  lcd.print(" ");
  lcd.print((char)223);
  lcd.print("C");
}

void setup(void) {
  Serial.begin(9600);
  sensors.begin();

  pinMode(nextPin, INPUT_PULLUP);
  pinMode(confirmPin, INPUT_PULLUP);
  pinMode(relay, OUTPUT);
  digitalWrite(relay, HIGH);

  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Starting");
  for (byte x = 0; x < 16; x++) {
    lcd.setCursor(x, 1);
    lcd.print("#");
    delay(70);
  }

  sensors.setResolution(insideThermometer, 9);
  attachInterrupt(digitalPinToInterrupt(confirmPin), confirm, FALLING);
  attachInterrupt(digitalPinToInterrupt(nextPin), next, FALLING);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("DS18B20 Tester");
  lcd.setCursor(0, 1);
  lcd.print("FWV 1.0 9.3.24");
  delay(2000);

  currentMillis = millis();
  timerMillis = millis();
}

void loop(void) {
  if (!fc) {
    if (doonce == 0) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Start test?");
      doonce = 1;
    }
    if (confirmState) {
      Serial.println("Pritisnjena tipka");
      startTest(insideThermometer);
      fc = true;
      fc2 = false;
      fc3 = false;
      confirmState = 0;
      currentMillis = millis();
    }
  } else if (!menu && !fc2) {
    printAddress(insideThermometer);
    fc2 = true;
    fc3 = false;
    currentMillis = millis();
  } else if (menu && !fc3) {
    printTemperature(insideThermometer);
    fc3 = true;
    fc2 = false;
    currentMillis = millis();
  } else if (!fc2 && !fc3) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("NAPAKA!");
    lcd.setCursor(0, 1);
    lcd.print("Resetiraj tester");
    currentMillis = millis();
  }

  if (confirmState == 1 && fc == true) {
    fc = false;
    doonce = 0;
    confirmState = 0;
    currentMillis = millis();
  }

  if ((millis() - currentMillis) >= powerDownTime) {
    if((millis() - timerMillis) >= refreshRate){
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Izklop cez: ");
      lcd.print((2*powerDownTime - (millis() - currentMillis)) / 1000);
      lcd.print(" s");
      timerMillis = millis();
    }
    if ((millis() - currentMillis) >= (2 * powerDownTime)) {
      lcd.clear();
      lcd.noBacklight();
      lcd.noDisplay();
      digitalWrite(relay, LOW);
      detachInterrupt(digitalPinToInterrupt(confirmPin));
      detachInterrupt(digitalPinToInterrupt(nextPin));
      attachInterrupt(digitalPinToInterrupt(confirmPin), wakeUp, FALLING);
      currentMillis = millis();
      LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);
    }
  }
  if (wakeupState == 1) {
    digitalWrite(relay, HIGH);
    delay(100);
    lcd.init();
    lcd.backlight();
    lcd.clear();
    startTest(insideThermometer);
    wakeupState = 0;
    detachInterrupt(digitalPinToInterrupt(confirmPin));
    attachInterrupt(digitalPinToInterrupt(confirmPin), confirm, FALLING);
    attachInterrupt(digitalPinToInterrupt(nextPin), next, FALLING);
    fc = false;
    doonce = 0;
    doonce2 = 0;
    confirmState = 0;
    currentMillis = millis();
  }
}
