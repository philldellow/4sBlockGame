# 4sBlockGame
ESP-32 Based Game Deveopment
---
title: "Creating a simple block game on the ESP-32"
slug: "/esp-32-block-game"
authors: [pdellow]
tags: [embedded, esp32, arduino, game]
description: "ESP-32 with an in-builtOLED screen running a bespoke built block game"
date: 2025-03-29
---
import VideoPlayer from "@site/src/components/VideoPlayer";

<VideoPlayer url="/videos/IMG_20250329_164536.mp4" />

# Building a Block Game on the ESP-32

## Introduction

This project started as a fun experiment with a block game running on an **ESP32-WROOM-32 module**. The exact model was the **ESP32-DevKitC V4**, which I bought from AliExpress. [Here's the link to the exact model](https://www.aliexpress.com/item/1005004389428930.html). This board features Wi-Fi/Bluetooth capabilities, making it ideal for compact projects like a block game.

### Why This Board?
The **ESP32-DevKitC V4** offers a good balance of performance and features:
- Dual-core processor with Wi-Fi and Bluetooth.
- Flexible GPIO pins for connecting peripherals like OLED displays.

## Steps We Took

### Setting Up the Development Environment
- Installed the **Arduino IDE** with the appropriate ESP32 board definitions.
- Imported required libraries for the OLED display (`Adafruit GFX` and `Adafruit SSD1306`) along with basic I/O handling.
- Verified the board was correctly recognized and programmable through USB.

### Wiring and Connecting the Display
- The display was connected to the ESP32 using I2C communication. Initializing the display required setting the correct I2C address (`0x3C`) in the code.
- Initial attempts to draw on the screen were met with failure due to incorrect library settings. This was resolved by setting the right resolution and ensuring compatibility with the OLED library.

### Coding the Block Game
- The game logic was built around a **simple loop** involving player-controlled blocks and moving obstacles.
- Implemented basic **collision detection** using bounding box checks. The resolution of the OLED display made this efficient and straightforward.
- Drawing to the display was initially slow due to using standard functions. We optimized this by rewriting drawing routines for rectangles and text.

### Troubleshooting and Pitfalls
- **Power Supply Issues:** Initially powered the ESP32 from a USB port which was insufficient for stable operation. Switching to a regulated power source solved the problem.
- **OLED Freezing:** A persistent issue where the display would freeze during gameplay. This was traced to the ESP32’s watchdog timer. Adjusting the game loop to periodically yield control resolved the issue.
- **Library Compatibility:** The default libraries for the OLED were not optimized for fast drawing. Implementing direct buffer manipulation improved performance.
- **Graphics Flicker:** When updating the display too frequently, flicker was introduced. This was mitigated by implementing double-buffering techniques.

## Lessons Learned
- Optimizing graphics rendering on low-resolution displays is essential for smooth gameplay.
- Managing the ESP32’s watchdog timer is critical when running intensive loops.
- Handling power supply issues early prevents countless headaches later on.

## What’s Next?
Moving forward, we could add:
- **Sound effects** using the DAC pins.
- **Bluetooth control** to allow remote gameplay or multiplayer features.

Stay tuned for more updates as we continue to enhance this block game experience.


## Conclusion
This project was a fantastic way to dive into embedded systems and game development. The ESP32’s versatility and the OLED’s simplicity made it an ideal platform for this project.
