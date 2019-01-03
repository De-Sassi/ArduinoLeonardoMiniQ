/*
 Name:		ebsProjekt.ino
 Created:	12/12/2018 4:33:07 PM
 Author:	delia
*/

//
//#include <IRremoteInt.h>
#include <IRremote.h>
#include <Adafruit_NeoPixel.h>
#include <LiquidCrystal_I2C.h>




//remote controll numbers
#define POWERBUTTON 16580863
#define VOLPLUS 16613503
#define VOLMINUS 16617583
#define LEFT 16589023
#define RIGHT 16605343
#define PLAY 16621663
#define NULLBUTTON 16593103
#define ONE 16582903
#define TWO 16615543
#define THREE 16599223
#define FOUR 16591063

#define EN1 6//enable the right motor driver port 
#define IN1 7//direction of the right motor
#define EN2 5//enable the left motor driver port
#define IN2 12//direction of the left motor

#define FORW 0//forward
#define BACK 1//back

#define IR_IN  17//IR receiver pin
#define L_IR 13//left ir transmitter pin
#define R_IR 8//right ir transmitter pin

//Programmnr definieren
#define FOLLOW_LINE 1
#define DETECT_OBJECT 2
#define BOTH 3
#define BACK_TO_MENU 4

#define THRESHOLD 550 //For line detection. Below 550 is black line above is wood table (or white paper)
#define MOTOR_SPEED 45 //min speed that does not cause problems with being stuck

IRrecv irrecv(IR_IN);
decode_results results;
Adafruit_NeoPixel led(1, 10, NEO_GRB + NEO_KHZ800);
LiquidCrystal_I2C lcd(0x20, 16, 2);

bool inMenuAuswahl = true;
int data[5] = {}; //


enum States
{
	MENU,
	FOLLOW,
	HUMAN_CONTROL,
	FOLLOW_AND_CHANGE

};

// the setup function runs once when you press reset or power the board
void setup() {


	Serial.begin(9600);
	Serial.println("Hello World");

	// Initialize RGB-LED
	led.begin();
	led.show();
	led.setBrightness(60);

	//Initialize LCD-Screen
	lcd.init();
	lcd.backlight();
	lcd.setCursor(0, 0);
	lcd.print("Hello World");
	lcd.setCursor(0, 1);
	lcd.print("second line");

	pinMode(L_IR, OUTPUT);//init the left transmitter pin
	pinMode(R_IR, OUTPUT);//init the right transmitter pin
	pinMode(IR_IN, INPUT);//init the ir receiver pin <- also for interrupt pin important

	digitalWrite(R_IR, HIGH);
	digitalWrite(L_IR, HIGH);

	irrecv.enableIRIn(); 

	////Interrupt
	digitalWrite(IR_IN, HIGH); //Enable pullup resistor
}


void loop() {

	
	if (irrecv.decode(&results))
	{
		Serial.println(results.value);
		lcd.clear();
		lcd.println(results.value);
		irrecv.resume();

	}

	int input = convertControllNumber(results.value);
	if (input != NULL)
	{
		inMenuAuswahl = false;
	}
	if (inMenuAuswahl)
	{
		display_Menu();
	}
	else
	{
		
		if (input == FOLLOW_LINE)
		{
	//16582903 1
			Read_Value();
			follow_line();
		}
		else if (input == DETECT_OBJECT)
		{
	//16615543 2
			lcd.clear();
			lcd.setCursor(0, 0);
			lcd.print("controll");
			delay(1000);
		}
		else if (input == BOTH)
		{
	//16599223 3
			lcd.clear();
			lcd.setCursor(0, 0);
			lcd.print("BOTH");
			delay(1000);
		}
		else if (input == BACK_TO_MENU)
		{
			//16580863 powerbutton
			inMenuAuswahl = true;
			results.value = 0;
			lcd.clear();
			lcd.setCursor(0, 0);
			lcd.print("Back to Menue");
			delay(1000);
			Motor_Control(FORW, 0, FORW, 0);
		}
		else
		{
			lcd.clear();
			lcd.setCursor(0, 0);
			lcd.print("Unknown Command");
			delay(1000);
		}

		
		
	}





}

int convertControllNumber(unsigned long number)
{
	if (number == ONE)
	{
		return 1;
	}
	else if (number == TWO)
	{
		return 2;
	}
	else if (number == THREE)
	{
		return 3;
	}
	else if (number == POWERBUTTON)
	{
		return 4;
	}
	return NULL;
}

void directionControl(unsigned long number)
{
	if (number == VOLPLUS)
	{
		//FORWARD
		Motor_Control(FORW, 45, FORW, 45);
	}
	else if (number == LEFT)
	{
		//left
		Motor_Control(FORW, 45, FORW, 0);
	}
	else if (number == RIGHT)
	{
		//right
		Motor_Control(FORW, 0, FORW, 45);
	}
	else if (number == VOLMINUS)
	{
		//backwards
		Motor_Control(BACK, 45, BACK, 45);
	}
	else if (number == PLAY)
	{
		// stop
		Motor_Control(FORW, 0, FORW, 0);
	}
}
// USER PIN interrupt (for PIN 17). all PINS are on 3 interrupt vectors. find right vector and lvl
//PCINT17 -> PCI2  PCIE2
//PCICR bit 2 to 1
//PCIF2 becomes set to 1 (PCIFR bit 2)-> MCU will jump to corresponding Interrrupt vector
//MASK PCMSK2 bit 1 für PCINT17 auf 1
void display_Menu()
{
	lcd.clear();
	lcd.setCursor(0, 0);
	lcd.print("Menu: Line:1 ");
	lcd.setCursor(0, 1);
	lcd.print("Contrl:2 Both:3");
	delay(2000);
}

void test_controll_over_serial()
{
	//RUN MOTORS
	if (Serial.available())
	{
		char input = Serial.read();
		Serial.println(input);
		int r_speed = 0;
		int l_speed = 0;
		switch (input) {
		case 'l':
			led.setPixelColor(0, 255, 0, 0);//rot
			led.show();
			r_speed = 50;
			l_speed = 10;
			break;
		case 'r':
			led.setPixelColor(0, 0, 255, 0);//grün
			led.show();
			r_speed = 10;
			l_speed = 50;
			break;
		case 'v':
			led.setPixelColor(0, 0, 0, 255);//blau
			led.show();
			r_speed = 50;
			l_speed = 50;
			break;
		case 's':
			led.setPixelColor(0, 0, 255, 100);//hellblau?
			led.show();
			break;
		default:
			led.setPixelColor(0, 255, 0, 255);//violet
			led.show();
			break;

		}
		Motor_Control(FORW, r_speed, FORW, l_speed);
		delay(2000);
	}
}

void test_Line()
{
	Read_Value();
	lcd.clear();
	lcd.setCursor(0, 0);
	lcd.print(data[0]);
	lcd.setCursor(4, 0);
	lcd.print(data[1]);
	lcd.setCursor(8, 0);
	lcd.print(data[2]);
	lcd.setCursor(12, 0);
	lcd.print(data[3]);
	lcd.setCursor(12, 1);
	lcd.print(data[4]);
	delay(1000);
}

void Read_Value(void)//read the five sensors
{
	char i;
	for (i = 0; i < 5; i++)
	{
		data[i] = analogRead(i);//store the value read from the sensors
	}
}

void follow_line()
{
	// Serial.println(data[2]);
	// delay(1000);
	int r_speed = 0;
	int l_speed = 0;
	if ((data[0] < THRESHOLD || data[1] < THRESHOLD) && (data[3] > THRESHOLD || data[4] > THRESHOLD))//left detect black line
	{
		led.setPixelColor(0, 0, 255, 0);//grün
		led.show();
		r_speed = MOTOR_SPEED;
		l_speed = 0;
	}
	else  if ((data[3] < THRESHOLD || data[4] < THRESHOLD) && (data[0] > THRESHOLD || data[1] > THRESHOLD))//black line in the right
	{
		led.setPixelColor(0, 255, 0, 0);//rot
		led.show();
		r_speed = 0;
		l_speed = MOTOR_SPEED;
	}
	else if (data[2] < THRESHOLD)//line in middle
	{
		led.setPixelColor(0, 0, 0, 255);//blau
		led.show();
		r_speed = MOTOR_SPEED;
		l_speed = MOTOR_SPEED;
	}
	else
	{
		led.setPixelColor(0, 255, 0, 255);//violet
		led.show();
		r_speed = MOTOR_SPEED;
		l_speed = MOTOR_SPEED;
	}
	Motor_Control(FORW, r_speed, FORW, l_speed); 
	delay(10);

}

void Motor_Control(int M1_DIR, int M1_EN, int M2_DIR, int M2_EN)
{
	//////////M1////////////////////////
	if (M1_DIR == FORW)  digitalWrite(IN1, FORW); else digitalWrite(IN1, BACK);
	if (M1_EN == 0)       analogWrite(EN1, LOW);  else analogWrite(EN1, M1_EN);
	///////////M2//////////////////////
	if (M2_DIR == FORW) digitalWrite(IN2, FORW);  else digitalWrite(IN2, BACK);
	if (M2_EN == 0)     analogWrite(EN2, LOW);    else analogWrite(EN2, M2_EN);
}