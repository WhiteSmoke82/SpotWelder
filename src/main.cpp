#include <Arduino.h>
#include <TM1637Display.h>
#include <EEPROM.h>

#define CLK 5
#define DIO 4
#define A 6
#define B 7
#define SW 8
#define RELAY 2
#define TRIG 3

TM1637Display display = TM1637Display(CLK, DIO);

int currentStateCLK;
int lastStateCLK;
unsigned long lastButtonPress = 0;
unsigned long lastTriggerPress = 0;
unsigned long saveDelay = 3000;
unsigned long showDelay = 500;
boolean menuMode = true;
int tempValue = 0;
unsigned long lastChangeTime, lastIndexChangeTime;

bool saved = true;

int valueArray[6];

const uint8_t atta[] = {0b01110111, 0b01111000, 0b01111000, 0b01110111};
const uint8_t sust[] = {0b01101101, 0b00011100, 0b01101101, 0b01111000};
const uint8_t cool[] = {0b00111001, 0b01011100, 0b01011100, 0b00110000};
const uint8_t temp[] = {0b01111000, 0b01000000, 0b01100011, 0b00111001};
const uint8_t brit[] = {0b01111100, 0b01010000, 0b00010000, 0b01111000};

const uint8_t save[] = {0b01101101, 0b01110111, 0b00111110, 0b01111001};
const uint8_t elec[] = {0b01111001, 0b00111000, 0b01111001, 0b00111001};

const uint8_t* letterlist[] = {atta, sust, cool, temp, brit};

//attack, sustain, cooldown, treshtemp, brightness, menuIndex;

void getMem(){
	EEPROM.get(0, valueArray[0]);
	EEPROM.get(8, valueArray[1]);
	EEPROM.get(16, valueArray[2]);
	EEPROM.get(24, valueArray[3]);
	EEPROM.get(32, valueArray[4]);
	EEPROM.get(40, valueArray[5]);
}

void saveToMem(){
	EEPROM.put(0, valueArray[0]);
	EEPROM.put(8, valueArray[1]);
	EEPROM.put(16, valueArray[2]);
	EEPROM.put(24, valueArray[3]);
	EEPROM.put(32, valueArray[4]);
	EEPROM.put(40, valueArray[5]);
}

void drawLetter(boolean full){
	display.setSegments(letterlist[valueArray[5]], full ? 4 : 1, 0);
}

void drawValue(){
	display.showNumberDec(valueArray[valueArray[5]], false, 3, 1);
}

void setup() {
	pinMode(A, INPUT);
	pinMode(B, INPUT);
	pinMode(SW, INPUT_PULLUP);
	pinMode(TRIG, INPUT);
	pinMode(RELAY, OUTPUT);

	digitalWrite(RELAY, HIGH);

	Serial.begin(9600);

	lastStateCLK = digitalRead(A);

	getMem();

	display.clear();
  	display.setBrightness(valueArray[4]);

	drawLetter(false);
	drawValue();
}


void increaseValue(){
	valueArray[valueArray[5]]++;
	lastChangeTime = millis();
	saved = false;
}

void decreaseValue(){
	valueArray[valueArray[5]]--;
	lastChangeTime = millis();
	saved = false;
}

void increaseIndex(){
	valueArray[5] ++;
	lastIndexChangeTime = millis();
	if(valueArray[5] > 4) valueArray[5] = 0;
}

void decreaseIndex(){
	valueArray[5] --;
	lastIndexChangeTime = millis();
	if(valueArray[5] < 0) valueArray[5] = 4;
}

boolean needToTurnOff = false;

void reDraw(){
	if(millis() - lastIndexChangeTime < showDelay){
		display.setSegments(letterlist[valueArray[5]]);
		needToTurnOff = true;
	}else{
		drawLetter(false);
		drawValue();
	}
}

void checkRotation(){
	currentStateCLK = digitalRead(A);
	if (currentStateCLK != lastStateCLK  && currentStateCLK == 1){
		if (digitalRead(B) != currentStateCLK) {
			if(menuMode){
				decreaseIndex();
			}else{
				decreaseValue();
			}
		} else {
			if(menuMode){
				increaseIndex();
			}else{
				increaseValue();
			}
		}

		reDraw();
	}
	lastStateCLK = currentStateCLK;
	delay(1);
}

void checkButton(){
	int btnState = digitalRead(SW);
	if (btnState == HIGH) {
		if (millis() - lastButtonPress > 50) {
			menuMode = !menuMode;
		}
		lastButtonPress = millis();
		reDraw();
	}
	delay(1);
}

void checkTrigger(){
	int btnState = digitalRead(TRIG);
	if (btnState == HIGH) {
		if (millis() - lastTriggerPress > 50) {
			long start = millis();
			while(millis() - start <= valueArray[0]){
				display.showNumberDec(valueArray[0] - (millis() - start));
			}
			digitalWrite(RELAY, LOW);

			display.setSegments(elec);
			delay(valueArray[1]);
			
			digitalWrite(RELAY, HIGH);
			start = millis();
			while(millis() - start <= valueArray[2] * 10){
				display.showNumberDec(valueArray[2] * 10 - (millis() - start));
			}
			reDraw();
		}
		lastTriggerPress = millis();
	}
	delay(1);
}

void loop() {
	checkRotation();
	checkButton();
	checkTrigger();
	
	if(needToTurnOff && millis() - lastIndexChangeTime > showDelay){
		reDraw();
		needToTurnOff = false;
	}

	if((millis() - lastChangeTime > saveDelay) && !saved){
		display.setSegments(save);
		saveToMem();
		delay(500);
		saved = true;
		drawLetter(false);
		drawValue();
	}
}