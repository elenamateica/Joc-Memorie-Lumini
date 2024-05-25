#include <Servo.h>
// Biblioteci pentru i2c
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Initializare LCD cu adresa 0x27 si dimensiunea 16 x 2
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Alegeri de culori
#define CHOICE_OFF      0
#define CHOICE_NONE     0
#define CHOICE_RED      (1 << 0)
#define CHOICE_GREEN    (1 << 3)
#define CHOICE_BLUE     (1 << 1)
#define CHOICE_YELLOW   (1 << 2)

// Pinii pentru LED-uri (GPIO)
#define LED_RED         3
#define LED_GREEN       9
#define LED_BLUE        5
#define LED_YELLOW      7

// Pinii pentru butoane (GPIO)
#define BUTTON_RED      2
#define BUTTON_GREEN    8
#define BUTTON_BLUE     4
#define BUTTON_YELLOW   6

// Pin buzzer (PWM)
#define BUZZER1         10
#define BUZZER2         12

// 3 secunde limita timp / 12 runde pentru castig
#define ROUNDS_TO_WIN       12
#define ENTRY_TIME_LIMIT    3000

// Variabile
byte gameBoard[32];
byte gameRound = 0;
Servo myservo;
int score = 0;

void setup() {
  Serial.begin(9600);
  Serial.println("Starting game...");

  // Atașez motorul la pinul 11
  myservo.attach(11);

  // Inițializare butoane
  pinMode(BUTTON_RED, INPUT_PULLUP);
  pinMode(BUTTON_GREEN, INPUT_PULLUP);
  pinMode(BUTTON_BLUE, INPUT_PULLUP);
  pinMode(BUTTON_YELLOW, INPUT_PULLUP);

  // Inițializare LED-uri
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_BLUE, OUTPUT);
  pinMode(LED_YELLOW, OUTPUT);

  // Inițializare buzzer
  pinMode(BUZZER1, OUTPUT);
  pinMode(BUZZER2, OUTPUT);

  // Inițializare LCD
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Score: ");
  lcd.setCursor(7, 0);
  lcd.print(score);
  Serial.println("LCD initializat");

  play_winner();
  myservo.write(90);
}

void loop() {
  // Joc de lumini înainte de începerea jocului
  attractMode();

  setLEDs(CHOICE_RED | CHOICE_GREEN | CHOICE_BLUE | CHOICE_YELLOW);
  myservo.write(90);
  delay(1000);
  setLEDs(CHOICE_OFF);
  delay(250);

  // Modul joc
  if (play_memory()) play_winner();
  else play_loser();
}

boolean play_memory(void) {
  // Număr random
  randomSeed(millis());

  gameRound = 0;

  while (gameRound < ROUNDS_TO_WIN) {
    add_to_moves();
    playMoves();

    for (byte currentMove = 0; currentMove < gameRound; currentMove++) {
      byte choice = wait_for_button();

      if (choice == 0) return false;
      if (choice != gameBoard[currentMove]) return false;
    }

    updateScore();
    delay(1000);
  }

  // Afișează "Winner" pe display
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Winner!");

  return true;
}

void playMoves(void) {
  for (byte currentMove = 0; currentMove < gameRound; currentMove++) {
    toner(gameBoard[currentMove], 150);
    delay(150);
  }
}

void add_to_moves(void) {
  byte newButton = random(0, 4);

  if (newButton == 0) newButton = CHOICE_RED;
  else if (newButton == 1) newButton = CHOICE_GREEN;
  else if (newButton == 2) newButton = CHOICE_BLUE;
  else if (newButton == 3) newButton = CHOICE_YELLOW;

  gameBoard[gameRound++] = newButton;
}

void setLEDs(byte leds) {
  // Starile LED-urilor
  digitalWrite(LED_RED, (leds & CHOICE_RED) ? HIGH : LOW);
  digitalWrite(LED_GREEN, (leds & CHOICE_GREEN) ? HIGH : LOW);
  digitalWrite(LED_BLUE, (leds & CHOICE_BLUE) ? HIGH : LOW);
  digitalWrite(LED_YELLOW, (leds & CHOICE_YELLOW) ? HIGH : LOW);
}

byte wait_for_button(void) {
  // Timer pentru timeout
  long startTime = millis();

  while ((millis() - startTime) < ENTRY_TIME_LIMIT) {
    byte button = checkButton();

    if (button != CHOICE_NONE) {
      toner(button, 150);
      while (checkButton() != CHOICE_NONE);
      delay(10);
      return button;
    }
  }

  return CHOICE_NONE;
}

byte checkButton(void) {
  // GPIO, verific starea fiecărui buton
  if (digitalRead(BUTTON_RED) == 0) return CHOICE_RED;
  if (digitalRead(BUTTON_GREEN) == 0) return CHOICE_GREEN;
  if (digitalRead(BUTTON_BLUE) == 0) return CHOICE_BLUE;
  if (digitalRead(BUTTON_YELLOW) == 0) return CHOICE_YELLOW;
  return CHOICE_NONE;
}

void toner(byte which, int buzz_length_ms) {
  // Sunet buzzer
  setLEDs(which);
  buzz_sound(buzz_length_ms);
  setLEDs(CHOICE_OFF);
}

void buzz_sound(int buzz_length_ms) {
  // Generare sunet buzzer
  long buzz_length_us = buzz_length_ms * (long)1000;
  while (buzz_length_us > 0) {
    buzz_length_us -= 1000;
    digitalWrite(BUZZER1, HIGH);
    delayMicroseconds(500);
    digitalWrite(BUZZER1, LOW);
    delayMicroseconds(500);
  }
}

void play_winner(void) {
  // Efect sunet câștigător
  setLEDs(CHOICE_GREEN | CHOICE_BLUE);
  buzz_sound(1000);
  setLEDs(CHOICE_RED | CHOICE_YELLOW);
  buzz_sound(1000);
  setLEDs(CHOICE_GREEN | CHOICE_BLUE);
  buzz_sound(1000);
  setLEDs(CHOICE_RED | CHOICE_YELLOW);
  buzz_sound(1000);
}

void play_loser(void) {
  // Efect sunet pierzător și resetare scor
  myservo.write(0);
  score = 0;
  lcd.setCursor(7, 0);
  // Șterg scorul precedent
  lcd.print("    ");
  lcd.setCursor(7, 0);
  lcd.print(score);
  setLEDs(CHOICE_RED | CHOICE_GREEN);
  buzz_sound(1500);
  setLEDs(CHOICE_BLUE | CHOICE_YELLOW);
  buzz_sound(1500);
  setLEDs(CHOICE_RED | CHOICE_GREEN);
  buzz_sound(1500);
  setLEDs(CHOICE_BLUE | CHOICE_YELLOW);
  buzz_sound(1500);
}

void attractMode(void) {
  // Efect lumini început
  while (1) {
    setLEDs(CHOICE_RED);
    delay(100);
    if (checkButton() != CHOICE_NONE) return;
    setLEDs(CHOICE_BLUE);
    delay(100);
    if (checkButton() != CHOICE_NONE) return;
    setLEDs(CHOICE_GREEN);
    delay(100);
    if (checkButton() != CHOICE_NONE) return;
    setLEDs(CHOICE_YELLOW);
    delay(100);
    if (checkButton() != CHOICE_NONE) return;
  }
}

void updateScore() {
  // Update scor pe LCD
  score++;
  lcd.setCursor(7, 0);
  lcd.print("    ");
  lcd.setCursor(7, 0);
  lcd.print(score);
}
