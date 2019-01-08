/*
 Name:		ebsProjekt.ino
 Created:	12/12/2018 4:33:07 PM
 Author:	Delia De-Sassi
 Descripton: Designed for a 2WD MiniQ robot. (Arduino Leonardo)
			There are 3 possible cases to choose.
			1: The robot follows a dark line (best results with black line on white underground. both non reflecting)
			2: The movement of the robot can be controlled over the remote. the current movment is displayed on the lcd.
			   Each direction has their own color of the rgb-led
			3: The robot follows the line. but the direction can be changed over the controll. The robot will turn in the desired direction till
			 it finds the line again.

			There is a Menu. it is only possible to access the diffrent cases from there.
			In the Menu there is printed out which button to press on the controller to achieve which case
*/


#include "States.h"
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

//Motors
#define EN1 6//enable the right motor driver port 
#define IN1 7//direction of the right motor
#define EN2 5//enable the left motor driver port
#define IN2 12//direction of the left motor
#define FORW 0//forward
#define BACK 1//back
//Infraret
#define IR_IN 17//IR receiver pin

//Line_Follow
#define THRESHOLD 550 //For line detection. Below 550 is black line above is wood table (or white paper).
						//Needs to be adjusted depending on the enviroment
#define MOTOR_SPEED_FOLLOW 33 //min speed that does not cause problems with being stuck. Can change depending on batteries and
						// if the robot is plugged in

IRrecv irrecv(IR_IN);
decode_results results;
Adafruit_NeoPixel led(1, 10, NEO_GRB + NEO_KHZ800);
LiquidCrystal_I2C lcd(0x20, 16, 2);

int data[5] = {}; //Array for detecting the black line
State currentState = MENU;


void setup() {

	currentState = MENU;

	////uncomment to start a serial connection (good for debugging)
	//Serial.begin(9600);
	//Serial.println("Hello World");

	// Initialize RGB-LED
	led.begin();
	led.show();
	led.setBrightness(60);

	//Initialize LCD-Screen
	lcd.init();
	lcd.backlight();
	display_Menu();

	//Initalize Infraret communication
	pinMode(IR_IN, INPUT);//init the ir receiver pin <- also for interrupt pin important
	irrecv.enableIRIn();

	////Interrupt
	//digitalWrite(IR_IN, HIGH); //Enable pullup resistor

	pinMode(5, OUTPUT);//init the motor driver pins
	pinMode(6, OUTPUT);
	pinMode(7, OUTPUT);
	pinMode(12, OUTPUT);
}


void loop() {


	//checks for new input from controller
	if (irrecv.decode(&results))
	{
		//uncomment for output of the number transmittet by the controller (Debugging)
		//Serial.println(results.value);
		//lcd.clear();
		//lcd.println(results.value);
		irrecv.resume();

	}
	//unsigned long userInput = results.value;
	State nextState = getNextState(results.value);

	switch (nextState)
	{
	case MENU:

		if (currentState != MENU) //only actualizes when it comes from a diffrent state
		{
			motor_control(FORW, 0, FORW, 0);
			display_Menu();
		}
		currentState = MENU;
		break;
	case FOLLOW:
		if (currentState != FOLLOW)
		{
			lcd.clear();
			lcd.setCursor(0, 0);
			lcd.print("Follow Line");
		}
		currentState = FOLLOW;
		read_linearray_values();
		follow_line();
		break;
	case HUMAN_CONTROL:
		if (currentState != HUMAN_CONTROL)
		{
			lcd.clear();
			lcd.setCursor(0, 0);
			lcd.print("Input direction");
		}
		currentState = HUMAN_CONTROL;
		directionControl(results.value);
		break;
	case FOLLOW_AND_CHANGE:
		if (currentState != FOLLOW_AND_CHANGE)
		{
			lcd.clear();
			lcd.setCursor(0, 0);
			lcd.print("Turn if you like");
		}
		currentState = FOLLOW_AND_CHANGE;
		read_linearray_values();
		follow_line();
		unsigned long input = results.value;
		if (turnInput(input))
		{
			turn(input);
		}
		break;
	}



}

///converts the controll number into the possilbe states. 
///if the number is not an actual state the current state is returned
State convertControllNumber(unsigned long number)
{
	if (number == ONE)
	{
		return FOLLOW;
	}
	else if (number == TWO)
	{
		return HUMAN_CONTROL;
	}
	else if (number == THREE)
	{
		return FOLLOW_AND_CHANGE;
	}
	else if (number == POWERBUTTON)
	{
		return MENU;
	}
	return currentState;
}

///gets the next possible state
State getNextState(unsigned long controllNumber)
{
	//From menu every state is possible
	//from all other states it is only possible to go back to menu
	if (currentState != MENU)
	{
		if (controllNumber == POWERBUTTON)//used to return to MENU
		{
			return MENU;
		}
		else
		{
			return currentState;
		}
	}
	else
	{
		return convertControllNumber(controllNumber);
	}
}

///Turns the robot in the controler defined direction till it finds the line again.
void turn(unsigned long number)
{
	turnOnLine(number);
	delay(500);
	//continue with line following as soon as  line found again
	bool line_not_found = true;


	int loopCount = 0;
	while (line_not_found)
	{
		read_linearray_values();
		if (data[2] < THRESHOLD)//line in middle
		{
			led.setPixelColor(0, 0, 0, 255);//blau
			led.show();
			motor_control(FORW, 0, FORW, 0);
			line_not_found = false;
			lcd.clear();
			lcd.setCursor(0, 0);
			lcd.print("line found");
		}
		if (loopCount > 2000)
		{
			//Exit strategie if the line is not found. 
			//so the robot can be controlled again by the user
			break;
		}
		loopCount++;
	}
}

///Checks if the user input is valid to turn the robot while following the line
bool turnInput(unsigned long number)
{

	if (number == LEFT || number == RIGHT || number == VOLMINUS || number == PLAY)
	{
		return true;
	}
	else
	{
		return false;
	}
}

///turns the robot on line
void turnOnLine(unsigned long number)
{
	const int speed = 30;
	if (number == LEFT)
	{
		//left
		lcd.clear();
		lcd.setCursor(0, 0);
		lcd.print("left");
		motor_control(FORW, 0, FORW, 0);
		delay(500);
		motor_control(FORW, speed, BACK, speed);
	}
	else if (number == RIGHT)
	{
		//right
		lcd.clear();
		lcd.setCursor(0, 0);
		lcd.print("right");
		motor_control(FORW, 0, FORW, 0);
		delay(500);
		motor_control(BACK, speed, FORW, speed);
	}
	else if (number == VOLMINUS)
	{
		//backwards
		lcd.clear();
		lcd.setCursor(0, 0);
		lcd.print("RANDOOOOM");
		int myRandomNumber = random(100); //Generats pseudo random number to decide if turn back on left side or right side
		motor_control(FORW, 0, FORW, 0);
		delay(500);
		if (myRandomNumber % 2)
		{
			motor_control(FORW, speed, BACK, speed);
		}
		else
		{
			motor_control(BACK, speed, FORW, speed);
		}	
		
	}
	else if (number == PLAY)
	{
		// stop
		lcd.clear();
		lcd.setCursor(0, 0);
		lcd.print("stop");
		motor_control(FORW, 0, FORW, 0);
	}
}
///Changes motor speeds depending on the controller input
void directionControl(unsigned long number)
{
	const int speed = 40;
	if (number == VOLPLUS)
	{
		//FORWARD
		lcd.clear();
		lcd.setCursor(0, 0);
		lcd.print("forward");
		motor_control(FORW, speed, FORW, speed);

	}
	else if (number == LEFT)
	{
		//left
		lcd.clear();
		lcd.setCursor(0, 0);
		lcd.print("left");
		motor_control(FORW, speed, FORW, 0);
	}
	else if (number == RIGHT)
	{
		//right
		lcd.clear();
		lcd.setCursor(0, 0);
		lcd.print("right");
		motor_control(FORW, 0, FORW, speed);
	}
	else if (number == VOLMINUS)
	{
		//backwards
		lcd.clear();
		lcd.setCursor(0, 0);
		lcd.print("backwards");
		motor_control(BACK, speed, BACK, speed);
	}
	else if (number == PLAY)
	{
		// stop
		lcd.clear();
		lcd.setCursor(0, 0);
		lcd.print("stop");
		motor_control(FORW, 0, FORW, 0);
	}
}

void display_Menu()
{
	lcd.clear();
	lcd.setCursor(0, 0);
	lcd.print("Menu: Line:1 ");
	lcd.setCursor(0, 1);
	lcd.print("Contrl:2 Both:3");
	led.setPixelColor(0, 0, 0, 0);//no light
	led.show();
	delay(2000);
}

///fills in the data array the values of the line array sensor
void read_linearray_values(void)//read the five sensors
{
	char i;
	for (i = 0; i < 5; i++)
	{
		data[i] = analogRead(i);//store the value read from the sensors
	}
}

///sets motor speed depending on where the line is detected
void follow_line()
{
	int r_speed = 0;
	int l_speed = 0;
	if ((data[0] < THRESHOLD || data[1] < THRESHOLD) && (data[3] > THRESHOLD || data[4] > THRESHOLD))//left detect black line
	{
		led.setPixelColor(0, 0, 255, 0);//grün
		led.show();
		r_speed = MOTOR_SPEED_FOLLOW;
		l_speed = 0;
	}
	else  if ((data[3] < THRESHOLD || data[4] < THRESHOLD) && (data[0] > THRESHOLD || data[1] > THRESHOLD))//black line in the right
	{
		led.setPixelColor(0, 255, 0, 0);//rot
		led.show();
		r_speed = 0;
		l_speed = MOTOR_SPEED_FOLLOW;
	}
	else if (data[2] < THRESHOLD)//line in middle
	{
		led.setPixelColor(0, 0, 0, 255);//blau
		led.show();
		r_speed = MOTOR_SPEED_FOLLOW;
		l_speed = MOTOR_SPEED_FOLLOW;
	}
	else //no defined case. just drive forward
	{
		led.setPixelColor(0, 255, 0, 255);//violet
		led.show();
		r_speed = MOTOR_SPEED_FOLLOW;
		l_speed = MOTOR_SPEED_FOLLOW;
	}
	motor_control(FORW, r_speed, FORW, l_speed);
	delay(10);

}

void motor_control(int M1_DIR, int M1_EN, int M2_DIR, int M2_EN)
{
	//////////M1= Right Motor////////////////////////
	digitalWrite(IN1, M1_DIR);
	analogWrite(EN1, M1_EN);
	///////////M2==Left Motor//////////////////////
	digitalWrite(IN2, M2_DIR);
	analogWrite(EN2, M2_EN); 
}

///Only for debugging. Can be used to controll the motors over a serial connection
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
		motor_control(FORW, r_speed, FORW, l_speed);
		delay(2000);
	}
}

///only for debugging. Prints on the lcd the Values the linearray reads
void test_Line()
{
	read_linearray_values();
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