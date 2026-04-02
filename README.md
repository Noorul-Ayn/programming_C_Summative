# Project 1: Smart Traffic Light Controller

## Project Description
A smart traffic light controller that manages traffic flow at two intersections
using Arduino (simulated in Tinkercad) and a PCB design in EasyEDA.

## Features
- Detects vehicle presence using push buttons
- Automatically adjusts traffic light timing based on traffic conditions
- Manages traffic lights using Red, Yellow, and Green LEDs
- Stores and displays vehicle counts and signal states via serial output
- Simulates concurrent operations using non-blocking programming (millis())
- Provides a serial interface menu for monitoring and manual overrides
- PCB schematic and layout designed in EasyEDA

## Requirements Met
1. Two traffic intersections simulated using LEDs (Intersection A and B)
2. Structures, pointers, and dynamic memory (malloc/free) used to manage traffic data
3. Non-blocking concurrency implemented using millis() instead of delay()
4. Traffic information logged through serial output on every state change
5. PCB schematic and layout designed in EasyEDA
6. Error handling implemented via isValidState() to prevent invalid signal states
7. Code is modular with clearly named functions and inline comments

## Pin Layout
| Component     | Intersection A | Intersection B |
|---------------|----------------|----------------|
| Red LED       | Pin 2          | Pin 8          |
| Yellow LED    | Pin 3          | Pin 9          |
| Green LED     | Pin 4          | Pin 10         |
| Push Button   | Pin 7          | Pin 11         |

## Serial Commands
| Key | Action                          |
|-----|---------------------------------|
| 1   | View full system status         |
| 2   | Toggle manual override A        |
| 3   | Toggle manual override B        |
| 4   | Force GREEN on Intersection A   |
| 5   | Force GREEN on Intersection B   |
| 6   | Reset all vehicle counts        |
| 7   | Show command menu               |

## Timing
| Phase       | Duration        |
|-------------|-----------------|
| Green       | 5s (up to 10s with vehicles) |
| Yellow      | 2s              |
| Green extension per vehicle | +2s |

## Files
- `traffic_light_controller.ino` - Main Arduino source code

## Simulation
Tinkercad simulation link: https://www.tinkercad.com/things/gWnlGV59JuB/editel?returnTo=%2Fdashboard

## PCB Design
EasyEDA project link: [ADD YOUR EASYEDA LINK HERE]

## How to Run
1. Open Tinkercad simulation link
2. Click Start Simulation
3. Open Serial Monitor at 9600 baud
4. Press buttons to simulate vehicle detection
5. Type commands 1-7 in Serial Monitor to interact with the system

## Concepts Demonstrated
- C structs and typedef
- Dynamic memory allocation with malloc() and free()
- Pointers and pointer arithmetic
- Non-blocking programming with millis()
- State machines
- Serial communication
- Basic error handling
