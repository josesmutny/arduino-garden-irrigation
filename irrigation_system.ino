#include <EEPROM.h>
#include <LiquidCrystal_I2C.h> // provides similar functionality as LiquidCrystal library, but for I2C
#include <Wire.h>			   // enables communication with I2C display

/***** Wiring ************************************************************************************/
/*
 * I2C display:
 *  SCL -> A5
 *  SDA -> A4
 *  GND -> GND
 *  VCC -> VCC
 *
 * Buttons
 *  1 -> PIN 3
 *  2 -> PIN 4
 *
 * Potenciometer
 *  PIN 1 -> GND (through resistor )
 *  PIN 2 -> PIN 3
 *  PIN 3 -> VCC
 */
/***** Constants and limits ************************************************************************************/

// LCD
const int LCD_ADDRESS = 0x3F;
const int LCD_WIDTH = 20;
const int LCD_HEIGHT = 4;
// Buttons
const int BUTTON_UP = 3;
const int BUTTON_DOWN = 4;
// Potenciometer
const int POTENCIOMETER = A0;
const int THERMOMETER = A1;
const int TMP_POT = A1;
const int POTENC_MAX_VAL = 1023;
const int POTENC_MIN_VAL = 0;
// Program Constants
int TEMP_MAX = 30;
int TEMP_MID = 20;
int TEMP_MIN = 10;
int HUMID_MAX = 80;
int HUMID_MIN = 0;
int IRRIGATION_LENGTH = 15;
// Pump


/***** Characters ************************************************************************************/

byte degreesSign[] = {B00110, B01001, B01001, B00110, B00000, B00000, B00000, B00000};
byte upArrow[] = {B00000, B00100, B01110, B11111, B00100, B00100, B00100, B00000};
byte downArrow[] = {B00000, B00100, B00100, B00100, B11111, B01110, B00100, B00000};
byte backArrow[] = {B00000, B00100, B01100, B11111, B01101, B00101, B00001, B01111};
byte tempWarning[] = {B01101, B01101, B01100, B01101, B01100, B10010, B10010, B01100};
byte temp[] = {B00000, B01011, B01011, B11100, B01000, B01000, B01001, B00110};
byte emptyBox[] = {B00000, B00000, B01110, B10001, B10001, B10001, B01110, B00000};
byte fullBox[] = {B00000, B00000, B01110, B11111, B11111, B11111, B01110, B00000};

const int DEGREE_CHAR = 0;
const int UP_ARR_CHAR = 1;
const int DOWN_ARR_CHAR = 2;
const int BACK_ARR_CHAR = 3;
const int TEMP_W_CHAR = 4;
const int TEMP_CHAR = 5;
const int EMPTY_CHAR = 6;
const int FULL_CHAR = 7;

/***** LCD ************************************************************************************/

LiquidCrystal_I2C lcd(LCD_ADDRESS, LCD_WIDTH, LCD_HEIGHT);

bool outOfRange(int col, int row) {
	return (col < 0 || col >= LCD_WIDTH || row < 0 || row >= LCD_HEIGHT);
}

// prints starting from x, y position
void printAt(const char *str, const int x, const int y) {
	if (outOfRange(x, y))
		return;
	lcd.setCursor(x, y);
	lcd.print(str);
}
// prints a value without fixed width
void printAt(const int val, const int col, const int row, const int width) {
	if (outOfRange(col, row))
		return;
	lcd.setCursor(col, row);
	String tmp(val);
	for (int i = 0; i < (width - tmp.length()); i++) {
		lcd.print(' ');
	}
	lcd.print(val);
}

void clear(int row) {
	if (outOfRange(0, row))
		return;
	for (int i = 0; i < LCD_WIDTH; i++) {
		clear(i, row);
	}
}
void clear(int col, int row) {
	if (outOfRange(col, row))
		return;
	lcd.setCursor(col, row);
	lcd.write(' ');
}

/***** Slider ************************************************************************************/

// Allows for simple reads from program Potenciometer (at pin POTENCIOMETER )
class Slider {
	int m_Pin;
	int m_Min;
	int m_Max;
	static const int PADDING_PERCENT = 5; // allows reads for edges

public:
	Slider(const int pin, const int min, const int max) : m_Pin(pin), m_Min(min), m_Max(max) {
	}
	int read(void) const {
		Serial.println(analogRead(m_Pin));
		if (analogRead(m_Pin) > (POTENC_MAX_VAL * (double(100 - PADDING_PERCENT) / 100))) {
			return m_Max;
		}
		return (int)map((long)analogRead(m_Pin), (long)POTENC_MIN_VAL, (long)POTENC_MAX_VAL, (long)m_Min, (long)m_Max);
	}
};

/***** Button ************************************************************************************/

// basic debouncing
class Button {
	const int m_Pin;
	bool m_Pressed;

public:
	Button(int pin) : m_Pin(pin), m_Pressed(false) {
	}

	bool read() {

		// if not pressed, return false and set m_Pressed to false
		// if pressed:
		// 	  if m_Pressed == true then return false
		//	  if m_Pressed != true then return true, set m_Pressed to true

		if (!digitalRead(m_Pin)) {
			m_Pressed = false;
			return false;
		}
		bool returnVal = !m_Pressed;
		m_Pressed = true;
		return returnVal;
	}
};

/***** State logic ************************************************************************************/

enum States { HOME,
			  SETTINGS,
			  SET_TIME,
			  SCHEDULE,
			  RESET };

/***** State functions ************************************************************************************/

void homeStart(void) {
	lcd.clear();
	lcd.setCursor(0, 0);
	lcd.write(TEMP_CHAR);
	lcd.print(":");
	lcd.setCursor(5, 0);
	lcd.write(DEGREE_CHAR);
	lcd.print("C  Hu:");
	lcd.setCursor(15, 0);
	lcd.print("%");

	lcd.setCursor(0, 2);
	lcd.write(UP_ARR_CHAR);
	lcd.print(": Start irrigating");
	lcd.setCursor(0, 3);
	lcd.write(DOWN_ARR_CHAR);
	lcd.print(": Open settings ");
}

/***** Setup ************************************************************************************/

void setup() {
	// Serial for debugging
	Serial.begin(9600);

	// Initialize LCD
	lcd.init();
	lcd.backlight();
	lcd.setCursor(0, 0);
	lcd.print("Hello world!");
	printAt("This is line two", 0, 1);
	lcd.clear();

	// Input/output declaration
	pinMode(BUTTON_DOWN, INPUT);
	pinMode(BUTTON_UP, INPUT);
	pinMode(POTENCIOMETER, INPUT);
	pinMode(TMP_POT, INPUT);

	// Custom Characters
	lcd.createChar(UP_ARR_CHAR, upArrow);
	lcd.createChar(DOWN_ARR_CHAR, downArrow);
	lcd.createChar(BACK_ARR_CHAR, backArrow);
	lcd.createChar(DEGREE_CHAR, degreesSign);
	lcd.createChar(TEMP_W_CHAR, tempWarning);
	lcd.createChar(TEMP_CHAR, temp);
	lcd.createChar(FULL_CHAR, fullBox);
	lcd.createChar(EMPTY_CHAR, emptyBox);

	// initiate HOME scren
	homeStart();
}

// initialize states
States currState = HOME;
States nextState = HOME;
// Initialize controls
Slider temperature(TMP_POT, -20, 40);
Slider humidity(TMP_POT, 0, 100);
Button bttnDown(BUTTON_DOWN);
Button bttnUp(BUTTON_UP);

// modify function vor int values
bool modify(int &val, const int min, const int max) {
	bool inc = bttnUp.read();
	bool dec = bttnDown.read();

	if (inc || dec) {
		val += inc - dec;
		if (val < min || val > max)
			val = val < min ? max : min;
		return true;
	}
	return false;
}

void confirmReset(void) {
	lcd.clear();
	lcd.setCursor(2, 1);
	lcd.print("Reset settings?");
	lcd.setCursor(3, 3);
	lcd.write(UP_ARR_CHAR);
	lcd.print(":Yes    ");
	lcd.write(DOWN_ARR_CHAR);
	lcd.print(":No");

	// wait for user
	while (true) {
		// reset settings
		if (bttnUp.read()) {
			//reset settings to default, and store to eeprom
			return;
		}
		// go back to settings
		if (bttnDown.read()) {
			// do nothing
			return;
		}
	}
}

void loop() {
	switch (nextState) {
	case HOME:
		if (currState != nextState) {
			homeStart();
		}
		currState = HOME;
		while (true) {
			printAt(temperature.read(), 2, 0, 3);
			printAt(humidity.read(), 12, 0, 3);
			if (bttnDown.read()) {
				nextState = SETTINGS;
				break;
			}
		}
		break;
	case SETTINGS:
		currState = SETTINGS;
		const int OPTION_COUNT = 10;
		lcd.clear();
		Slider sel(POTENCIOMETER, 0, OPTION_COUNT - 1);
		const int shift = 1;
		const char names[OPTION_COUNT][LCD_WIDTH + 1] = {"Modify schedule", "Temp Max", "Temp Mid", "Temp Min", "Length", "Hum Max", "Hum Min", "Set time", "Reset settings", "Go home"};
		int values[OPTION_COUNT] = {0, TEMP_MAX, TEMP_MID, TEMP_MIN, IRRIGATION_LENGTH, HUMID_MAX, HUMID_MIN, 0, 0, 0};
		int range[OPTION_COUNT][2] = {{0, 0},
									  {20, 40}, // degrees C
									  {10, 30}, // degrees C
									  {0, 20},	// degrees C
									  {1, 120}, // minutes
									  {0, 100}, // percent
									  {0, 100}, // percent
									  {0, 0},
									  {0, 0},
									  {0, 0}};
		int currVal = sel.read();					   // value from 0 to OPTION_COUNT - 1
		int lastVal = currVal == 0 ? OPTION_COUNT : 0; // so that they are not equal at first

		bool valueModified = false;

		while (true) {

			int currSel = sel.read();
			bool currUp = bttnUp.read();
			bool currDown = bttnDown.read();
			switch (currSel) {
			case 0:
				break;
			case 7:
				break;
			case 8: // reset settings
				if (currUp || currDown) {
					confirmReset();
					nextState = HOME;
				}
				break;
			case 9: // go Home
				if (currUp)
					nextState = HOME;
				break;
			default: // for normal values
				valueModified = modify(values[currSel], range[currSel][0], range[currSel][1]);
				break;
			}

			// continue only if the state remained as settings
			if (nextState != SETTINGS)
				break;

			currVal = sel.read() + shift;
			if (lastVal == currVal && !valueModified)
				continue;

			// refresh options only if screen has changed
			if (lastVal / 4 != currVal / 4)
				lcd.clear();

			// print rows
			for (int row = 0; row < LCD_HEIGHT; row++) {
				lcd.setCursor(0, row);
				if (currVal / LCD_HEIGHT == 0 && row < shift) {
					lcd.write(UP_ARR_CHAR);
					lcd.write(DOWN_ARR_CHAR);
					lcd.print(":Modify or Select");
					continue;
				}
				// add new line in every line, and round the value of the potentiometer
				// to LCD_HEIGHT;
				int currOption = row + ((currVal / LCD_HEIGHT) * 4) - shift;
				if (currOption >= OPTION_COUNT) {
					for (; row < LCD_HEIGHT; row++) {
						lcd.setCursor(0, row);
						for (int i = 0; i < LCD_WIDTH; i++) {
							lcd.print(" ");
						}
					}
					continue;
				}
				lcd.write(row == currVal % LCD_HEIGHT ? FULL_CHAR : EMPTY_CHAR);
				lcd.print(" ");
				lcd.print(names[currOption]);
				lcd.print(" ");
				if (currOption != 0 && currOption != 7 && currOption != 8 && currOption != 9) {
					printAt(values[currOption], 13, row, 3);
				}
			}
			lastVal = currVal;
		}
		break;
	}
}
