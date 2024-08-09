#include "TM1637Display.h"

#define CLK1 12   // Pin connected to CLK on the module
#define DIO1 11   // Pin connected to DIO on the module

#define DIO2 10   // DIO on second module. In this case, the clock can be shared

// For reference...
// void showNumberDecEx(int num, uint8_t dots = 0, bool leading_zero = false, uint8_t length = 4, uint8_t pos = 0);
// void setSegments(const uint8_t segments[], uint8_t length = 4, uint8_t pos = 0);

const int brightness = 0; // Brightness value, although still a bit wonky when tested

const int rButton = 5;
const int mButton = 6;
const int lButton = 7;

const int RESET;

const uint8_t SEG_P = 0b01110011; // uint8_t => unsigned 8-bit integer and is synonomous with Arduino keyword BYTE (could use hex, but more readeable this way in this case)
const uint8_t SEG_1 = 0b00110000;
const uint8_t SEG_2 = 0b01011011;
const uint8_t SEG_BLANK = 0b00000000;
const uint8_t SEG_COLON = 0b01000000;

uint8_t SEG_P1[4] = {SEG_P, SEG_1, SEG_BLANK, SEG_BLANK};
uint8_t SEG_P2[4] = {SEG_P, SEG_2, SEG_BLANK, SEG_BLANK};

struct Player{
  bool myTurn;
  int timeRemaining;
  unsigned long previousTimeStop;
  int mins, secs;

  Player();

  int updatedTime();
  void initiateTime(int totalTime);
  
};

Player PlayerL;
Player PlayerR;

// TODO: Check Displays/Players/Buttons are all correctly matching
TM1637Display DisplayL(CLK1, DIO1); // Classes initiated for each individual display
TM1637Display DisplayR(CLK1, DIO2);

int timeSetting(void);
void chooseWhiteBlack(void);

void setup() {
  DisplayL.setBrightness(brightness);
  DisplayR.setBrightness(brightness);

  pinMode(rButton, INPUT_PULLUP);
  pinMode(lButton, INPUT_PULLUP);
  pinMode(mButton, INPUT_PULLUP);
  pinMode(RESET, OUTPUT);

  int totalTime = timeSetting(); // function call to retrieve timesetting for each chess clock (in seconds)
  PlayerL.initiateTime(totalTime);
  PlayerR.initiateTime(totalTime);

  chooseWhiteBlack();
}

void loop() {
  DisplayL.showNumberDecEx(PlayerL.updatedTime(), SEG_COLON, true);
  DisplayR.showNumberDecEx(PlayerR.updatedTime(), SEG_COLON, true);
 
  if(digitalRead(lButton)){
    PlayerL.myTurn = false;
    PlayerR.myTurn = true;
  }
  if(digitalRead(rButton)){
    PlayerR.myTurn = false;
    PlayerL.myTurn = true;
  }
  
  if(PlayerL.timeRemaining <= 0 || PlayerR.timeRemaining <= 0){  // Conditional statement in the event that the a player loses by time
    if(PlayerL.timeRemaining <= 0){
      PlayerL.secs = 0;
    }
    else{
      PlayerR.secs = 0;
    }
    while(true){
      DisplayL.showNumberDecEx(PlayerL.mins + PlayerL.secs, SEG_COLON, true);
      DisplayR.showNumberDecEx(PlayerR.mins + PlayerR.secs, SEG_COLON, true);
    }
  }
  // TODO: Write condition and behaviour for end of the game (either within class or in loop() function (leaning with class))
  // TODO: Lookup and implement restart function with button or something
}

/* 
  Time setup initializes the amount of minutes the game wants in total(1 - 10 but can be changed easily).
  It does this by pressing the buttons and increasing or decreasing the number shown on the display.
  When the middle button is pressed, the number is returned and the game starts shortly.
*/
int timeSetting(void){
  int numDisplayed = 5; // starts the clock in the middle, but can be changed
  while(!digitalRead(mButton)){
    DisplayL.showNumberDecEx(numDisplayed*100, 0X40, true, 4);  // 0X40 is hex to keep the colons on. Trailing 0's is 'true'. 4 is the total amount of digits in the code
    DisplayR.showNumberDecEx(numDisplayed*100, 0X40, true, 4);

    if(digitalRead(lButton) == HIGH && numDisplayed > 1){   // Limits to min of 1
      delay(150);
      numDisplayed--;
    }
    if(digitalRead(rButton) == HIGH && numDisplayed < 10){  // Limits to max of 10
      delay(150);
      numDisplayed++;
    }
  }
  return numDisplayed * 60; // returns back minutes in seconds
}

/*
  This function allows the user to choose what side of the clock will be white and the other black and soon after starts the game through countdown.
  In this case, white and black is subjective to the orientation of the clock relative to the board, so left and right are used instead

  This works using a while loop and waiting for mid button to be pressed. Until then, left and right button can be used to indicate who "P1" and "P2" are.
  Afterwards, clock countdowns final seconds before clock runs into the loop() function outside of the setup() function where it is called
*/
void chooseWhiteBlack(void){
  PlayerL.myTurn = true;  // defaults left as 'P1'
  PlayerR.myTurn = false;
  delay(250);
  while(!digitalRead(mButton)){

    if(PlayerL.myTurn){
      DisplayL.setSegments(SEG_P1);
      DisplayR.setSegments(SEG_P2);
    }
    else{
      DisplayL.setSegments(SEG_P2);
      DisplayR.setSegments(SEG_P1);
    }

    if(digitalRead(lButton)){ // sets left as White
      PlayerL.myTurn = true;
      PlayerR.myTurn = false;
    }
    if(digitalRead(rButton)){ // sets right as White
      PlayerL.myTurn = false;
      PlayerR.myTurn = true;
    }
  }

  for(int i = 3; i > 0; i--){ // all this just to make it blink(bruh smh)
    int tempNumShown = i*1000 + i*100 + i*10 + i;
    int timeStamp = millis(); // used millis() instead of seconds() to get better timing flexibility
    while((millis() - timeStamp) < 1000){ // works as a delay but without actually calling delay to avoid blanks on displays
      DisplayL.showNumberDec(tempNumShown);
      DisplayR.showNumberDec(tempNumShown);
    }
  }
  PlayerL.previousTimeStop = PlayerR.previousTimeStop = millis();
  // TODO: check if PreviousTimeStop is working effectively
}

// ***************************************** Player Struct Implementation ****************************************
Player::Player(){
  myTurn = true;
  timeRemaining = previousTimeStop = 0;
  mins = secs = 0;
}

int Player::updatedTime(){  // This updates the time to be displayed
  if(myTurn == true){
    timeRemaining = timeRemaining - ((millis() - previousTimeStop)/1000);
    mins = (timeRemaining / 60) * 100; // This will cover both minutes places on its respectibe spot on the display
    secs = timeRemaining % 60;
    if((millis()-previousTimeStop) >= 1000){ // This resets previous time spot every second
      previousTimeStop = millis();
    }
  }
  else{
    previousTimeStop = millis();
  }
  return(mins + secs);  // Full number to print on the display
}

void Player::initiateTime(int totalTime){ // This method allows time settings to be fully instatiated
  timeRemaining = totalTime;
  mins = (timeRemaining / 60) * 100;
  secs = timeRemaining % 60;
}

