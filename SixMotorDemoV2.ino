#include <Servo.h>

/**
 * V2 (Enhanced): Real-time Text-Based Control via Serial Monitor.
 *
 * Changelog:
 *  - You no longer have to type "S4=120" every time.
 *  - Instead, you select a servo first by typing, e.g., "S4" (which sets currentServo to 4).
 *  - Then simply type the angle as a number (e.g., "120"), and it moves that servo by that angle.
 *  - To change the servo, type "S1", then type angles (e.g., 50, 150, etc.).
 *  - You can still type "SPEED=XX" to set the servo movement delay.
 *
 * Usage Example:
 *   1) "S2"  -> sets currentServo = 2.
 *   2) "120" -> moves servo #2 to 120 degrees, one degree at a time.
 *   3) "60"  -> now moves servo #2 to 60 degrees.
 *   4) "S1"  -> sets currentServo = 1.
 *   5) "100" -> moves servo #1 to 100 degrees.
 *   6) "SPEED=10" -> servo moves faster (10 ms per degree).
 *
 */

// ----------------- Configuration -----------------
static const int NUM_SERVOS = 6;
static const int servoPins[NUM_SERVOS] = {2, 3, 4, 5, 6, 7};

#include <string.h> // for strncmp, etc.

Servo servos[NUM_SERVOS];

// Buffer for incoming serial commands
#define CMD_BUFFER_SIZE 32
char cmdBuffer[CMD_BUFFER_SIZE];
byte cmdIndex = 0;

// Global speed (delay in ms between increments)
int g_moveDelay = 20; // 20 ms is a moderate default

// Current servo selected by the user
int currentServo = -1; // invalid by default

void setup() {
  Serial.begin(9600);

  // Attach servos and set them all to 90 at startup
  for (int i = 0; i < NUM_SERVOS; i++) {
    servos[i].attach(servoPins[i]);
    servos[i].write(90);
  }

  // Print instructions
  Serial.println("=== Robotic Arm Control (V2 Enhanced) ===");
  Serial.println("Use commands in Serial Monitor (newline-terminated, 9600 baud):");
  Serial.println(" - 'Sx': select servo x (0..5)");
  Serial.println(" - Then type a number (e.g. '120') to move that servo to that angle.");
  Serial.println(" - 'SPEED=XX': set move delay to XX ms per degree (0..200)");
  Serial.println("Example:");
  Serial.println("  S2  (select servo #2)");
  Serial.println("  120 (move servo #2 to 120)");
  Serial.println("  60  (move servo #2 to 60)");
  Serial.println("  S1  (select servo #1)");
  Serial.println("  150 (move servo #1 to 150)");
  Serial.println("----------------------------------------------------");
}

void loop() {
  // Continuously read serial data and parse commands
  while (Serial.available() > 0) {
    char incomingByte = Serial.read();

    // If we get a newline, parse the command
    if (incomingByte == '\n') {
      cmdBuffer[cmdIndex] = '\0'; // null-terminate the string
      parseCommand(cmdBuffer);
      cmdIndex = 0; // reset buffer
    } else {
      // Add incoming character to buffer if there's space
      if (cmdIndex < CMD_BUFFER_SIZE - 1) {
        cmdBuffer[cmdIndex++] = incomingByte;
      }
    }
  }
}

// ----------------- Parse and Act on Commands --------------------
void parseCommand(char* command) {
  // Trim leading/trailing whitespace (optional)
  // For simplicity, assume user input is well-formed.

  // If command is empty, ignore.
  if (strlen(command) == 0) {
    return;
  }

  // 1) Check if the command sets the SPEED.
  if (strncmp(command, "SPEED=", 6) == 0) {
    int newSpeed = atoi(command + 6);
    if (newSpeed < 0) newSpeed = 0;
    // you might limit to some upper bound if desired, e.g. 200
    if (newSpeed > 200) newSpeed = 200;
    g_moveDelay = newSpeed;
    Serial.print("Global move delay set to ");
    Serial.print(g_moveDelay);
    Serial.println(" ms");
    return;
  }

  // 2) Check if the command is selecting a servo, i.e. 'Sx'
  //    We'll assume up to 2 digits for servo index but you only have 6 anyway.
  if (command[0] == 'S') {
    // skip the 'S', parse the number
    int sIndex = atoi(&command[1]);
    if (sIndex < 0 || sIndex >= NUM_SERVOS) {
      Serial.println("Invalid servo index!");
      return;
    }
    currentServo = sIndex;
    Serial.print("Now controlling servo ");
    Serial.println(currentServo);
    return;
  }

  // 3) Otherwise, we assume the user typed an angle.
  //    If no servo is selected, print error.
  if (currentServo < 0 || currentServo >= NUM_SERVOS) {
    Serial.println("No valid servo selected. Type something like 'S2' first.");
    return;
  }

  // Parse angle
  int targetAngle = atoi(command);
  if (targetAngle < 0) targetAngle = 0;
  if (targetAngle > 180) targetAngle = 180;

  // Move the servo more slowly, degree by degree
  int currentAngle = servos[currentServo].read();

  if (currentAngle < targetAngle) {
    for (int a = currentAngle; a <= targetAngle; a++) {
      servos[currentServo].write(a);
      delay(g_moveDelay);
    }
  } else if (currentAngle > targetAngle) {
    for (int a = currentAngle; a >= targetAngle; a--) {
      servos[currentServo].write(a);
      delay(g_moveDelay);
    }
  }

  // Optional feedback
  Serial.print("Servo ");
  Serial.print(currentServo);
  Serial.print(" moved to ");
  Serial.println(targetAngle);
}