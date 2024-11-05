#include <Arduino.h>

#define BUTTON_1_PIN 2
#define BUTTON_2_PIN 3
#define RGB_BLUE_PIN 4
#define RGB_GREEN_PIN 5
#define RGB_RED_PIN 6

// Game state variables
int wordNumber = 29, gameRunning = 0, correctInput = 0;
char *generatedString[] = {
    "apple", "car", "pearl", "melon", "laptop",
    "phone", "brave", "charm", "bulb", "eagle",
    "fire", "globe", "chilly", "flower", "hard",
    "easy", "lemon", "mango", "tall", "ocean",
    "peace", "beach", "start", "shiny", "stop",
    "city", "color", "shark", "light", "young" };
char chosenString[10];
char inputString[10];
int inputIndex = 0;

// Button debounce variables
unsigned long lastDebounceTime1 = 0;
const unsigned long debounceDelay = 50;
int lastButton1Reading = HIGH;
int button1State = HIGH;

unsigned long lastDebounceTime2 = 0;
int lastButton2Reading = HIGH;
int button2State = HIGH;

// Game timing variables
unsigned long gameStartTime = 0;
const unsigned long gameDuration = 30000;
unsigned long wordStartTime = 0;

// Difficulty levels and their word time limits
int currentDifficultyIndex = 0;
const char *difficultyLevels[] = {"Easy", "Medium", "Hard"};
const unsigned long timeLimits[] = {5000, 3000, 2000};
unsigned long wordTimeLimit = timeLimits[currentDifficultyIndex];

int correctWordCount = 0;

// Function Declarations
void chooseString(int index);
void updateDifficulty();
void displayRoundAnimation();
void startGame();
void stopGame();
void gameOver();
void wordTimeout();
void resetInputState();
void resetLEDs();
void handleButton1();
void handleButton2();
void checkGameTimeout();
void checkWordTimeout();
void handleSerialInput();
void handleBackspace();
void handleCharacterInput(char receivedChar);
void updateInputLEDs();
void handleCorrectWord();

void chooseString(int index) {
  // Copy a randomly chosen word to "chosenString"
  strcpy(chosenString, generatedString[index]);
}

void updateDifficulty() {
  // Cycle through difficulty levels and update word time limit
  currentDifficultyIndex = (currentDifficultyIndex + 1) % 3;
  wordTimeLimit = timeLimits[currentDifficultyIndex];
  Serial.print("Difficulty set to ");
  Serial.println(difficultyLevels[currentDifficultyIndex]);
}

void displayRoundAnimation() {
  // Flash RGB LEDs as an animation for round start/stop
  for (int i = 0; i < 3; i++) {
    digitalWrite(RGB_GREEN_PIN, HIGH);
    digitalWrite(RGB_BLUE_PIN, HIGH);
    digitalWrite(RGB_RED_PIN, HIGH);
    delay(500);
    digitalWrite(RGB_GREEN_PIN, LOW);
    digitalWrite(RGB_BLUE_PIN, LOW);
    digitalWrite(RGB_RED_PIN, LOW);
    delay(500);
  }
}

void startGame() {
  // Initialize game, display start animation, select first word, and prompt user
  displayRoundAnimation();
  gameRunning = 1;
  gameStartTime = millis();
  wordStartTime = millis();
  correctWordCount = 0;

  // Choose initial word and prompt user
  chooseString(random(0, wordNumber));
  Serial.println("\nGame started! Type the word:");
  Serial.println(chosenString);
  digitalWrite(RGB_GREEN_PIN, HIGH); // Green LED indicates game is running
  digitalWrite(RGB_BLUE_PIN, LOW);
  digitalWrite(RGB_RED_PIN, LOW);
  resetInputState();
}

void stopGame() {
  // End game, show end animation, and display score
  displayRoundAnimation();
  gameRunning = 0;
  Serial.println("\nGame stopped.");
  Serial.print("Correct words typed: ");
  Serial.println(correctWordCount);
  resetLEDs();
}

void gameOver() {
  // Handle end of game when time is up
  gameRunning = 0;
  Serial.println("\nTime's up! Game over.");
  Serial.print("Correct words typed: ");
  Serial.println(correctWordCount);
  resetLEDs();
}

void wordTimeout() {
  // Handle word timeout and display next word
  Serial.println("\nTime's up for this word! Next word:");
  chooseString(random(0, wordNumber));
  Serial.println(chosenString);
  resetInputState();
  wordStartTime = millis(); // Reset word start time
}

void resetInputState() {
  memset(inputString, 0, sizeof(inputString));
  inputIndex = 0;
}

void resetLEDs() {
  digitalWrite(RGB_GREEN_PIN, LOW);
  digitalWrite(RGB_RED_PIN, LOW);
  digitalWrite(RGB_BLUE_PIN, LOW);
}

void handleButton1() {
  // Debounced Button 1 handling for starting/stopping the game
  int reading1 = digitalRead(BUTTON_1_PIN);
  if (reading1 != lastButton1Reading)
    lastDebounceTime1 = millis();

  if ((millis() - lastDebounceTime1) > debounceDelay && reading1 != button1State) {
    button1State = reading1;
    if (button1State == LOW) {
      if (!gameRunning)
        startGame();
      else
        stopGame();
    }
  }
  lastButton1Reading = reading1;
}

void handleButton2() {
  // Debounced Button 2 handling for cycling difficulty levels
  int reading2 = digitalRead(BUTTON_2_PIN);
  if (reading2 != lastButton2Reading)
    lastDebounceTime2 = millis();

  if ((millis() - lastDebounceTime2) > debounceDelay && reading2 != button2State) {
    button2State = reading2;
    if (button2State == LOW && !gameRunning)
      updateDifficulty();
  }
  lastButton2Reading = reading2;
}

void checkGameTimeout() {
  if (millis() - gameStartTime >= gameDuration)
    gameOver();
}

void checkWordTimeout() {
  if (millis() - wordStartTime >= wordTimeLimit)
    wordTimeout();
}

void handleSerialInput() {
  // Process input from Serial monitor (characters and backspace)
  while (Serial.available() > 0) {
    char receivedChar = Serial.read();
    if (receivedChar == '\b' || receivedChar == 127)
      handleBackspace();
    else if (receivedChar != '\r' && receivedChar != '\n')
      handleCharacterInput(receivedChar);
  }
}

void handleBackspace() {
  if (inputIndex > 0) {
    inputIndex--;
    inputString[inputIndex] = '\0';
    Serial.print("\b \b"); // Erase character on Serial monitor
    updateInputLEDs();
  }
}

void handleCharacterInput(char receivedChar) {
  // Add character to input array if space is available
  if (inputIndex < (sizeof(inputString) - 1)) {
    inputString[inputIndex++] = receivedChar;
    Serial.print(receivedChar);
    updateInputLEDs();

    // Check if the user has finished typing the word correctly
    if (correctInput && inputIndex == strlen(chosenString))
      handleCorrectWord();
  }
}

void updateInputLEDs() {
  // Update LEDs based on how input matches target word
  correctInput = (strncmp(chosenString, inputString, inputIndex) == 0);
  digitalWrite(RGB_GREEN_PIN, correctInput ? HIGH : LOW);
  digitalWrite(RGB_RED_PIN, correctInput ? LOW : HIGH);
}

void handleCorrectWord() {
  // Handle a correct word entry, reset input, and show next word
  Serial.println("\nCorrect! Next word:");
  correctWordCount++;
  chooseString(random(0, wordNumber));
  Serial.println(chosenString);
  resetInputState();
  wordStartTime = millis(); // Reset time for the next word
}

void setup() {
  Serial.begin(9600);
  Serial.println("Welcome to TypeRacer!");

  pinMode(BUTTON_1_PIN, INPUT_PULLUP);
  pinMode(BUTTON_2_PIN, INPUT_PULLUP);
  pinMode(RGB_GREEN_PIN, OUTPUT);
  pinMode(RGB_BLUE_PIN, OUTPUT);
  pinMode(RGB_RED_PIN, OUTPUT);

  resetLEDs();
  randomSeed(analogRead(A0));
}

void loop() {
  handleButton1();
  handleButton2();

  if (gameRunning) {
    checkGameTimeout();
    checkWordTimeout();
    handleSerialInput();
  }
}