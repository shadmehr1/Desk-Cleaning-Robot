#include <Servo.h>

/**
 * V3: Predefined Positions via Serial Monitor
 *
 * Usage:
 *   - Type "POS=0" -> Move all servos to 90 degrees.
 *   - Type "POS=1" -> Move servos to [0, 165, 175, 50, 145, 90].
 *   - Type "POS=2" -> Move servos to [0, 165, 175, 0, 120, 90].
 *   - Type "POS=3" -> Move servos to [0, 165, 175, 0, 60, 90].
 *   - Type "POS=4" -> Move servos to [0, 165, 175, 80, 60, 90].
 *
 * A global variable g_moveDelay controls how slow or fast the arm moves.
 * You can adjust it in code (default 40 ms).
 *
 * The code increments or decrements each servo from its current angle
 * to the target angle, one degree at a time.
 *
 * Set the Serial Monitor to 9600 baud and send commands with Newline.
 */

// ----------------- Configuration -----------------
static const int NUM_SERVOS = 6;
static const int servoPins[NUM_SERVOS] = {2, 3, 4, 5, 6, 7};

// Predefined positions
// (index 0 is all servos at 90, etc.)
// Each row has angles for S0..S5.
int positions[5][NUM_SERVOS] = {
  // POS 0: all 90
  {90, 90, 90, 90, 90, 90},
  // POS 1: S0=0, S1=165, S2=175, S3=50, S4=145, S5=90
  {0, 165, 175, 50, 145, 90},
  // POS 2: S0=0, S1=165, S2=175, S3=0, S4=120, S5=90
  {0, 165, 175, 0, 120, 90},
  // POS 3: S0=0, S1=165, S2=175, S3=0, S4=60, S5=90
  {0, 165, 175, 0, 60, 90},
  // POS 4: S0=0, S1=165, S2=175, S3=80, S4=60, S5=90
  {0, 165, 175, 80, 60, 90}
};

// Global delay between increments (ms)
// You can edit this value to make movement slower or faster.
int g_moveDelay = 40;  // 40 ms is fairly slow.

// Buffer for incoming serial commands
#define CMD_BUFFER_SIZE 32
char cmdBuffer[CMD_BUFFER_SIZE];
byte cmdIndex = 0;

Servo servos[NUM_SERVOS];

void setup() {
  Serial.begin(9600);

  // Attach servos and set them all to 90 at startup
  for (int i = 0; i < NUM_SERVOS; i++) {
    servos[i].attach(servoPins[i]);
    servos[i].write(90);
  }

  Serial.println("=== Robotic Arm Control (V3) ===");
  Serial.println("Type 'POS=x' to move to a predefined position:");
  Serial.println("  POS=0 -> all servos 90 deg");
  Serial.println("  POS=1 -> [0,165,175,50,145,90]");
  Serial.println("  POS=2 -> [0,165,175,0,120,90]");
  Serial.println("  POS=3 -> [0,165,175,0,60,90]");
  Serial.println("  POS=4 -> [0,165,175,80,60,90]");
  Serial.println("Current movement delay (g_moveDelay): 40 ms.");
  Serial.println("Edit 'g_moveDelay' in code to make movement slower/faster.");
  Serial.println("-------------------------------------------");
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
  // Look for "POS="
  if (strncmp(command, "POS=", 4) == 0) {
    // parse the number after "POS="
    int posIndex = atoi(command + 4);

    // Ensure it's in range [0..4]
    if (posIndex < 0 || posIndex > 4) {
      Serial.println("Invalid position index! Use 0..4.");
      return;
    }

    // Move to that position
    moveToPosition(posIndex);
    return;
  }

  // If we got here, the command isn't recognized.
  if (strlen(command) > 0) {
    Serial.println("Unrecognized command. Use 'POS=x'.");
  }
}

void moveToPosition(int posIndex) {
  // For each servo, increment/decrement to the target angle

  // 1) Read current angles
  int currentAngles[NUM_SERVOS];
  for (int i = 0; i < NUM_SERVOS; i++) {
    currentAngles[i] = servos[i].read();
  }

  // 2) We loop until all servos are at their target
  bool allAtTarget = false;
  while (!allAtTarget) {
    allAtTarget = true;

    // Increment/decrement each servo by 1 degree if needed
    for (int i = 0; i < NUM_SERVOS; i++) {
      int targetAngle = positions[posIndex][i];
      if (currentAngles[i] < targetAngle) {
        currentAngles[i]++;
        allAtTarget = false;
        servos[i].write(currentAngles[i]);
      } else if (currentAngles[i] > targetAngle) {
        currentAngles[i]--;
        allAtTarget = false;
        servos[i].write(currentAngles[i]);
      }
    }

    // Only delay once per iteration for smoother motion
    if (!allAtTarget) {
      delay(g_moveDelay);
    }
  }

  Serial.print("Moved to position ");
  Serial.println(posIndex);
}
