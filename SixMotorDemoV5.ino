#include <Servo.h>

/**
 * V5 (Revised):
 *  - "go": 
 *     * Moves to pos0, waits 5s
 *     * Moves pos1->pos14 in sequence
 *     * 0.5s pause after each position except after pos2 & pos8, 
 *       where it waits 2s
 *  - "no": forced stop of in-progress movement
 *  - "0..14": immediate move to that position
 */

static const int NUM_SERVOS = 6;
static const int servoPins[NUM_SERVOS] = {2, 3, 4, 5, 6, 7};

// 15 positions (0..14)
int positions[15][NUM_SERVOS] = {
  {170,165,175, 90, 90, 90},  // pos0
  {170,165,175, 42,145, 60},  // pos1
  {0,  165,175, 42,145, 60},  // pos2
  {0,  165,175, 10,120, 60},  // pos3
  {0,  165,175, 15, 60, 60},  // pos4
  {0,  165,175, 80, 60, 60},  // pos5
  {0,  165,175, 80, 60,125},  // pos6
  {0,  165,175,  5,100,125},  // pos7
  {170,165,175,  5,100,125},  // pos8
  {170,165,175,  5, 80,110},  // pos9
  {170,165,175,  5, 80, 95},  // pos10
  {170,165,175,  5,110, 75},  // pos11
  {170,165,175,  5,110,127},  // pos12
  {170,165,175, 45,100,105},  // pos13
  {170,165,175, 90, 90, 90}   // pos14
};

// Delay between each single degree step
int g_moveDelay = 30;  

#define CMD_BUFFER_SIZE 32
char cmdBuffer[CMD_BUFFER_SIZE];
byte cmdIndex = 0;

Servo servos[NUM_SERVOS];

// Global to detect forced stop
volatile bool stopMotion = false;

void setup() {
  Serial.begin(9600);

  // Attach servos and set them all to 90 at startup
  for (int i = 0; i < NUM_SERVOS; i++) {
    servos[i].attach(servoPins[i]);
    servos[i].write(90);
  }

  Serial.println("=== Robotic Arm Control V4 (Revised) ===");
  Serial.println("Commands:");
  Serial.println("  0..14 : Move immediately to that position");
  Serial.println("  go    : Move to pos0, wait 5s, then pos1..14 with pauses");
  Serial.println("  no    : Force stop any movement");
  Serial.println("-------------------------------------------");
}

void loop() {
  readSerial(); // Always watch for new commands
}

// ----------------- Serial Reading & Parsing ------------------
void readSerial() {
  while (Serial.available() > 0) {
    char incomingByte = Serial.read();
    if (incomingByte == '\n') {
      cmdBuffer[cmdIndex] = '\0';
      parseCommand(cmdBuffer);
      cmdIndex = 0;
    } else {
      if (cmdIndex < CMD_BUFFER_SIZE - 1) {
        cmdBuffer[cmdIndex++] = incomingByte;
      }
    }
  }
}

void parseCommand(char* command) {
  if (strlen(command) == 0) return;

  // Check for "go"
  if (strcmp(command, "go") == 0) {
    stopMotion = false;
    startSequence();
    return;
  }

  // Check for "no"
  if (strcmp(command, "no") == 0) {
    stopMotion = true;
    Serial.println("Force stop received. Movement halted.");
    return;
  }

  // Otherwise, parse as integer for posIndex
  int posIndex = atoi(command);
  if (posIndex >= 0 && posIndex < 15) {
    stopMotion = false;
    moveToPosition(posIndex);
  } else {
    Serial.println("Invalid command. Use 0..14, 'go', or 'no'.");
  }
}

// ----------------- "go" Sequence Logic ------------------
void startSequence() {
  // 1) Move to pos0
  moveToPosition(0);
  if (stopMotion) return;

  // 2) Wait 5s
  if (!waitWithStopCheck(5000)) {
    Serial.println("Stopped during 5s wait.");
    return; 
  }

  // 3) Now move from pos1..14, pausing after each
  for (int i = 1; i <= 14; i++) {
    if (stopMotion) {
      Serial.print("Stopped before moving to pos");
      Serial.println(i);
      return;
    }
    moveToPosition(i);
    if (stopMotion) return;

    // After each position, pause
    // Special 2s pause after pos2 and pos8, otherwise 0.5s
    if (i == 2 || i == 8) {
      if (!waitWithStopCheck(2000)) {
        Serial.print("Stopped during post-pos");
        Serial.print(i);
        Serial.println(" wait.");
        return;
      }
    } else {
      if (!waitWithStopCheck(300)) {
        Serial.print("Stopped during post-pos");
        Serial.print(i);
        Serial.println(" wait.");
        return;
      }
    }
  }

  Serial.println("Sequence complete. Ended at pos14.");
}

// Wait the specified milliseconds, but check for forced stop
bool waitWithStopCheck(unsigned long msToWait) {
  unsigned long startTime = millis();
  while (millis() - startTime < msToWait) {
    readSerial();      // check if 'no' was typed
    if (stopMotion) {
      return false;    // indicates we were forced to stop
    }
    delay(10);         // small wait to avoid a tight spin
  }
  return true;         // completed without stop
}

// ----------------- Movement Function ------------------
void moveToPosition(int posIndex) {
  // 1) Read current angles
  int currentAngles[NUM_SERVOS];
  for (int i = 0; i < NUM_SERVOS; i++) {
    currentAngles[i] = servos[i].read();
  }

  // 2) Move until all servos reach their target, or forced stop
  bool allAtTarget = false;
  while (!allAtTarget) {
    allAtTarget = true;

    // Check forced stop each iteration
    readSerial();
    if (stopMotion) {
      Serial.println("Movement interrupted by 'no'.");
      return;
    }

    for (int i = 0; i < NUM_SERVOS; i++) {
      int targetAngle = positions[posIndex][i];
      if (currentAngles[i] < targetAngle) {
        currentAngles[i]++;
        servos[i].write(currentAngles[i]);
        allAtTarget = false;
      } else if (currentAngles[i] > targetAngle) {
        currentAngles[i]--;
        servos[i].write(currentAngles[i]);
        allAtTarget = false;
      }
    }

    if (!allAtTarget) {
      delay(g_moveDelay);
    }
  }

  Serial.print("Moved to position ");
  Serial.println(posIndex);
}
