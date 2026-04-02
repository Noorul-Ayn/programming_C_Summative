#include <stdlib.h>

#define NUM_INTERSECTIONS  2
#define BASE_GREEN_MS      5000UL
#define YELLOW_MS          2000UL
#define EXTENSION_MS       2000UL
#define MAX_GREEN_MS      10000UL

#define STATE_RED     0
#define STATE_GREEN   1
#define STATE_YELLOW  2

typedef struct {
  const char* name;
  uint8_t     redPin;
  uint8_t     yellowPin;
  uint8_t     greenPin;
  uint8_t     buttonPin;
  uint8_t     state;
  uint32_t    vehicleCount;
  uint32_t    lastChangeTime;
  uint32_t    greenDuration;
  bool        manualOverride;
  bool        vehicleDetected;
} Intersection;

Intersection* intersections[NUM_INTERSECTIONS];

void setupIntersection(int idx, const char* name, uint8_t rPin, uint8_t yPin, uint8_t gPin, uint8_t btnPin) {
  intersections[idx] = (Intersection*) malloc(sizeof(Intersection));
  if (intersections[idx] == NULL) return;

  intersections[idx]->name            = name;
  intersections[idx]->redPin          = rPin;
  intersections[idx]->yellowPin       = yPin;
  intersections[idx]->greenPin        = gPin;
  intersections[idx]->buttonPin       = btnPin;
  intersections[idx]->state           = STATE_RED;
  intersections[idx]->vehicleCount    = 0;
  intersections[idx]->lastChangeTime  = 0;
  intersections[idx]->greenDuration   = BASE_GREEN_MS;
  intersections[idx]->manualOverride  = false;
  intersections[idx]->vehicleDetected = false;

  pinMode(intersections[idx]->redPin,    OUTPUT);
  pinMode(intersections[idx]->yellowPin, OUTPUT);
  pinMode(intersections[idx]->greenPin,  OUTPUT);
  pinMode(intersections[idx]->buttonPin, INPUT_PULLUP);

  digitalWrite(intersections[idx]->redPin,    LOW);
  digitalWrite(intersections[idx]->yellowPin, LOW);
  digitalWrite(intersections[idx]->greenPin,  LOW);
}

bool isValidState(uint8_t state) {
  return (state == STATE_RED || state == STATE_GREEN || state == STATE_YELLOW);
}

void logStatus(int idx) {
  const char* s;
  switch (intersections[idx]->state) {
    case STATE_RED:    s = "RED";    break;
    case STATE_GREEN:  s = "GREEN";  break;
    case STATE_YELLOW: s = "YELLOW"; break;
    default:           s = "???";
  }
  Serial.print(intersections[idx]->name);
  Serial.print(": ");
  Serial.print(s);
  Serial.print(" | Count: ");
  Serial.println(intersections[idx]->vehicleCount);
}

void setSignal(int idx, uint8_t newState) {
  if (!isValidState(newState)) return;

  intersections[idx]->state = newState;

  digitalWrite(intersections[idx]->redPin,    LOW);
  digitalWrite(intersections[idx]->yellowPin, LOW);
  digitalWrite(intersections[idx]->greenPin,  LOW);

  switch (newState) {
    case STATE_RED:    digitalWrite(intersections[idx]->redPin,    HIGH); break;
    case STATE_YELLOW: digitalWrite(intersections[idx]->yellowPin, HIGH); break;
    case STATE_GREEN:  digitalWrite(intersections[idx]->greenPin,  HIGH); break;
  }

  intersections[idx]->lastChangeTime = millis();
  logStatus(idx);
}

void detectVehicle(int idx) {
  if (intersections[idx]->manualOverride) return;

  bool pressed = (digitalRead(intersections[idx]->buttonPin) == LOW);

  if (pressed && !intersections[idx]->vehicleDetected) {
    intersections[idx]->vehicleDetected = true;
    intersections[idx]->vehicleCount++;

    if (intersections[idx]->state == STATE_GREEN) {
      uint32_t extended = intersections[idx]->greenDuration + EXTENSION_MS;
      intersections[idx]->greenDuration = (extended > MAX_GREEN_MS) ? MAX_GREEN_MS : extended;
      Serial.print("Vehicle at "); Serial.print(intersections[idx]->name);
      Serial.print(" - Count: "); Serial.println(intersections[idx]->vehicleCount);
    } else {
      Serial.print("Vehicle waiting at "); Serial.println(intersections[idx]->name);
    }

  } else if (!pressed) {
    intersections[idx]->vehicleDetected = false;
  }
}

void updateIntersection(int idx, int opposingIdx) {
  if (intersections[idx]->manualOverride) return;

  uint32_t elapsed = millis() - intersections[idx]->lastChangeTime;

  switch (intersections[idx]->state) {
    case STATE_GREEN:
      if (elapsed >= intersections[idx]->greenDuration) {
        intersections[idx]->greenDuration = BASE_GREEN_MS;
        setSignal(idx, STATE_YELLOW);
      }
      break;

    case STATE_YELLOW:
      if (elapsed >= YELLOW_MS) {
        setSignal(idx, STATE_RED);
        if (intersections[opposingIdx]->state == STATE_RED &&
            !intersections[opposingIdx]->manualOverride) {
          setSignal(opposingIdx, STATE_GREEN);
        }
      }
      break;

    case STATE_RED:
      break;
  }
}

void printAllStatus(void) {
  Serial.println("\n--- STATUS ---");
  for (int i = 0; i < NUM_INTERSECTIONS; i++) {
    const char* s;
    switch (intersections[i]->state) {
      case STATE_RED:    s = "RED";    break;
      case STATE_GREEN:  s = "GREEN";  break;
      case STATE_YELLOW: s = "YELLOW"; break;
      default:           s = "???";
    }
    Serial.print(intersections[i]->name);
    Serial.print(": "); Serial.print(s);
    Serial.print(" | Vehicles: "); Serial.print(intersections[i]->vehicleCount);
    Serial.print(" | Override: "); Serial.println(intersections[i]->manualOverride ? "ON" : "OFF");
  }
  Serial.println("--------------\n");
}

void printMenu(void) {
  Serial.println("\nCommands:");
  Serial.println("1: Status");
  Serial.println("2: Toggle Override A");
  Serial.println("3: Toggle Override B");
  Serial.println("4: Force A Green");
  Serial.println("5: Force B Green");
  Serial.println("6: Reset Counts");
  Serial.println("7: Menu");
}

void handleSerialMenu(void) {
  char cmd = (char) Serial.read();
  switch (cmd) {
    case '1': printAllStatus(); break;
    case '2':
      intersections[0]->manualOverride = !intersections[0]->manualOverride;
      Serial.print("Override A: "); Serial.println(intersections[0]->manualOverride);
      break;
    case '3':
      intersections[1]->manualOverride = !intersections[1]->manualOverride;
      Serial.print("Override B: "); Serial.println(intersections[1]->manualOverride);
      break;
    case '4':
      if (intersections[0]->manualOverride) {
        setSignal(0, STATE_GREEN);
        setSignal(1, STATE_RED);
      }
      break;
    case '5':
      if (intersections[1]->manualOverride) {
        setSignal(1, STATE_GREEN);
        setSignal(0, STATE_RED);
      }
      break;
    case '6':
      for (int i = 0; i < NUM_INTERSECTIONS; i++) intersections[i]->vehicleCount = 0;
      Serial.println("Counts reset");
      break;
    case '7': printMenu(); break;
  }
}

void setup() {
  Serial.begin(9600);
  Serial.println("TRAFFIC CONTROLLER START");

  setupIntersection(0, "A", 2, 3, 4, 7);
  setupIntersection(1, "B", 8, 9, 10, 11);

  for (int i = 0; i < NUM_INTERSECTIONS; i++) {
    if (intersections[i] == NULL) { while (1); }
  }

  setSignal(0, STATE_GREEN);
  setSignal(1, STATE_RED);

  printMenu();
}

void loop() {
  for (int i = 0; i < NUM_INTERSECTIONS; i++) {
    detectVehicle(i);
  }

  updateIntersection(0, 1);
  updateIntersection(1, 0);

  if (Serial.available() > 0) {
    handleSerialMenu();
  }
}
