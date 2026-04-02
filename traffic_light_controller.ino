#include <Arduino.h>
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

Intersection* createIntersection(const char* name, uint8_t rPin, uint8_t yPin, uint8_t gPin, uint8_t btnPin);
void  initIntersection(Intersection* inter, bool startGreen);
void  setSignal(Intersection* inter, uint8_t newState);
void  detectVehicle(Intersection* inter);
void  updateIntersection(Intersection* inter, Intersection* opposing);
void  logStatus(Intersection* inter);
void  printAllStatus(void);
void  printMenu(void);
void  handleSerialMenu(void);
bool  isValidState(uint8_t state);
void  freeAllIntersections(void);

void setup() {
  Serial.begin(9600);
  while (!Serial);

  Serial.println("TRAFFIC CONTROLLER START");

  intersections[0] = createIntersection("A", 2, 3, 4, 7);
  intersections[1] = createIntersection("B", 8, 9, 10, 11);

  for (int i = 0; i < NUM_INTERSECTIONS; i++) {
    if (intersections[i] == NULL) {
      while (1);
    }
  }

  initIntersection(intersections[0], true);
  initIntersection(intersections[1], false);

  printMenu();
}

void loop() {
  for (int i = 0; i < NUM_INTERSECTIONS; i++) {
    detectVehicle(intersections[i]);
  }

  updateIntersection(intersections[0], intersections[1]);
  updateIntersection(intersections[1], intersections[0]);

  if (Serial.available() > 0) {
    handleSerialMenu();
  }
}

Intersection* createIntersection(const char* name, uint8_t rPin, uint8_t yPin, uint8_t gPin, uint8_t btnPin) {
  Intersection* inter = (Intersection*) malloc(sizeof(Intersection));
  if (inter == NULL) return NULL;

  inter->name            = name;
  inter->redPin          = rPin;
  inter->yellowPin       = yPin;
  inter->greenPin        = gPin;
  inter->buttonPin       = btnPin;
  inter->state           = STATE_RED;
  inter->vehicleCount    = 0;
  inter->lastChangeTime  = 0;
  inter->greenDuration   = BASE_GREEN_MS;
  inter->manualOverride  = false;
  inter->vehicleDetected = false;

  pinMode(inter->redPin,    OUTPUT);
  pinMode(inter->yellowPin, OUTPUT);
  pinMode(inter->greenPin,  OUTPUT);
  pinMode(inter->buttonPin, INPUT_PULLUP);

  digitalWrite(inter->redPin,    LOW);
  digitalWrite(inter->yellowPin, LOW);
  digitalWrite(inter->greenPin,  LOW);

  return inter;
}

void initIntersection(Intersection* inter, bool startGreen) {
  if (inter == NULL) return;
  setSignal(inter, startGreen ? STATE_GREEN : STATE_RED);
}

void setSignal(Intersection* inter, uint8_t newState) {
  if (inter == NULL) return;
  if (!isValidState(newState)) return;

  inter->state = newState;

  digitalWrite(inter->redPin,    LOW);
  digitalWrite(inter->yellowPin, LOW);
  digitalWrite(inter->greenPin,  LOW);

  switch (newState) {
    case STATE_RED:    digitalWrite(inter->redPin,    HIGH); break;
    case STATE_YELLOW: digitalWrite(inter->yellowPin, HIGH); break;
    case STATE_GREEN:  digitalWrite(inter->greenPin,  HIGH); break;
  }

  inter->lastChangeTime = millis();
  logStatus(inter);
}

void detectVehicle(Intersection* inter) {
  if (inter == NULL || inter->manualOverride) return;

  bool pressed = (digitalRead(inter->buttonPin) == LOW);

  if (pressed && !inter->vehicleDetected) {
    inter->vehicleDetected = true;
    inter->vehicleCount++;

    if (inter->state == STATE_GREEN) {
      uint32_t extended = inter->greenDuration + EXTENSION_MS;
      inter->greenDuration = (extended > MAX_GREEN_MS) ? MAX_GREEN_MS : extended;
      Serial.print("Vehicle at "); Serial.print(inter->name);
      Serial.print(" - Count: "); Serial.println(inter->vehicleCount);
    } else {
      Serial.print("Vehicle waiting at "); Serial.println(inter->name);
    }

  } else if (!pressed) {
    inter->vehicleDetected = false;
  }
}

void updateIntersection(Intersection* inter, Intersection* opposing) {
  if (inter == NULL || inter->manualOverride) return;

  uint32_t elapsed = millis() - inter->lastChangeTime;

  switch (inter->state) {
    case STATE_GREEN:
      if (elapsed >= inter->greenDuration) {
        inter->greenDuration = BASE_GREEN_MS;
        setSignal(inter, STATE_YELLOW);
      }
      break;

    case STATE_YELLOW:
      if (elapsed >= YELLOW_MS) {
        setSignal(inter, STATE_RED);
        if (opposing != NULL && opposing->state == STATE_RED && !opposing->manualOverride) {
          setSignal(opposing, STATE_GREEN);
        }
      }
      break;

    case STATE_RED:
      break;
  }
}

void logStatus(Intersection* inter) {
  if (inter == NULL) return;

  const char* stateStr;
  switch (inter->state) {
    case STATE_RED:    stateStr = "RED"; break;
    case STATE_GREEN:  stateStr = "GREEN"; break;
    case STATE_YELLOW: stateStr = "YELLOW"; break;
    default:           stateStr = "???";
  }

  Serial.print(inter->name);
  Serial.print(": ");
  Serial.print(stateStr);
  Serial.print(" | Count: ");
  Serial.println(inter->vehicleCount);
}

void printAllStatus(void) {
  Serial.println("\n--- STATUS ---");
  for (int i = 0; i < NUM_INTERSECTIONS; i++) {
    Intersection* inter = intersections[i];
    const char* stateStr;
    switch (inter->state) {
      case STATE_RED:    stateStr = "RED"; break;
      case STATE_GREEN:  stateStr = "GREEN"; break;
      case STATE_YELLOW: stateStr = "YELLOW"; break;
      default:           stateStr = "???";
    }
    Serial.print(inter->name);
    Serial.print(": ");
    Serial.print(stateStr);
    Serial.print(" | Vehicles: ");
    Serial.print(inter->vehicleCount);
    Serial.print(" | Override: ");
    Serial.println(inter->manualOverride ? "ON" : "OFF");
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
  Serial.println("7: Menu\n");
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
        setSignal(intersections[0], STATE_GREEN);
        setSignal(intersections[1], STATE_RED);
      }
      break;
    case '5':
      if (intersections[1]->manualOverride) {
        setSignal(intersections[1], STATE_GREEN);
        setSignal(intersections[0], STATE_RED);
      }
      break;
    case '6':
      for (int i = 0; i < NUM_INTERSECTIONS; i++) intersections[i]->vehicleCount = 0;
      Serial.println("Counts reset");
      break;
    case '7': printMenu(); break;
  }
}

bool isValidState(uint8_t state) {
  return (state == STATE_RED || state == STATE_GREEN || state == STATE_YELLOW);
}

void freeAllIntersections(void) {
  for (int i = 0; i < NUM_INTERSECTIONS; i++) {
    if (intersections[i] != NULL) {
      free(intersections[i]);
      intersections[i] = NULL;
    }
  }
}

