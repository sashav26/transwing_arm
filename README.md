# Transwing Arm - SBUS RC Servo Controller

**Platform**: Raspberry Pi Pico (RP2040)
**Purpose**: Receives SBUS signals from RC receiver → Controls servo motors via PWM

---

## System Architecture

```
RC Receiver (SBUS) → UART0 RX (GPIO 1) → Pico → PWM (GPIO 15) → Servo Motor
```

**Data Flow**:
1. UART interrupt captures bytes → circular buffer (64 bytes)
2. Frame sync scans for start (0x0F) and end (0x00) markers
3. Extract 16 channels (11-bit each, 0-2047 range)
4. Map SBUS values (172-1811) → servo pulse width (1000-2000 µs)
5. PWM output drives servo at 50Hz

---

## Module Structure

### **config.h** - Hardware Configuration
- Pin assignments: `SERVO_PIN (15)`, `SBUS_RX_PIN (1)`, `LED_PIN (25)`
- Constants: `SERVO_MIN/MAX_US`, `SBUS_CHANNEL_MIN/MAX`, `SBUS_TIMEOUT_MS`
- Debug macros: `SBUS_DEBUG()`, `SERVO_DEBUG()`

### **sbus2.c/h** - SBUS Protocol Handler
**Constants**:
- `SBUS_BAUDRATE (100000)`, `SBUS_FRAME_SIZE (25)`, `SBUS_NUM_CHANNELS (16)`
- `SBUS_START_BYTE (0x0F)`, `SBUS_END_BYTE (0x00)`, `RX_BUFFER_SIZE (64)`

**Key Structure**:
```c
typedef struct {
    uint32_t frames_received;       // Valid frames
    uint32_t frames_invalid;        // Failed validation
    uint32_t sync_events;           // Resync count
    uint32_t buffer_overflows;      // Overflow events
    uint64_t last_valid_frame_us;   // Last frame timestamp
    bool failsafe_active;           // Failsafe status
    bool signal_lost;               // Signal loss flag
} sbus_stats_t;
```

**Major Functions**:
- `sbus_init(tx_pin, rx_pin)` - Initialize UART0 at 100kbps, 8E2 format
- `on_uart_rx()` - **[ISR]** UART interrupt handler, writes to circular buffer
- `get_sbus_frame(frame_out, len)` - Sync & extract 25-byte frame with peek-before-consume
- `validate_sbus_frame(frame)` - Verify start/end bytes
- `sbus_get_channel(frame, ch_idx)` - Extract 11-bit channel value (0-2047)

**Non-Blocking API** (recommended):
- `sbus_frame_available()` - Check if frame ready (no consume)
- `sbus_read_channels(channels[16])` - Read all channels at once
- `sbus_read_single_channel(ch_idx, *value)` - Read one channel

**Statistics API**:
- `sbus_get_stats()` - Get current statistics
- `sbus_reset_stats()` - Reset counters

**Legacy**:
- `process_sbus_data()` - Blocking infinite loop (prints channels)

### **servo.c/h** - PWM Servo Control
**Major Functions**:
- `initServo(pin, start_pulse_us)` - Setup PWM at 50Hz, set initial position
- `setPulseWidth(pin, pulse_us)` - Update servo position (1000-2000 µs)

**Implementation**:
- Calculates PWM wrap value for 50Hz frequency
- Adjusts clock divider (64-256) to fit wrap in 16-bit register
- Converts microseconds to PWM duty cycle

### **led.c/h** - LED Status Indicator
**Functions**:
- `initLED()` - Initialize GPIO 25 (onboard LED)
- `blinkLED()` - Toggle LED state
- `setLED(state)` - Set LED on/off explicitly

### **transwingC.c** - Main Application
**Major Functions**:
- `map_sbus_to_servo(sbus_value)` - Maps SBUS (172-1811) → servo µs (1000-2000)
- `main()` - Main control loop

**Main Loop** (10ms cycle):
1. Call `sbus_read_channels(channels)` (non-blocking)
2. If frame available → map channel 1 to servo pulse width
3. Update servo via `setPulseWidth()`
4. Print stats every 100 frames
5. Detailed statistics report every 5 seconds

---

## Key Features

### Frame Synchronization (Fixed)
- **Problem**: Old code read 25 bytes blindly, causing misalignment
- **Solution**: Peek-before-consume logic:
  1. Scan buffer for `SBUS_START_BYTE (0x0F)`
  2. Verify `SBUS_END_BYTE (0x00)` at position 24
  3. Only consume frame if both valid
  4. Otherwise discard 1 byte and rescan

### Error Handling
- Bounds checking on channel index (0-15)
- Frame validation (start/end bytes)
- Circular buffer overflow detection
- Statistics tracking for all error conditions

### Performance
- Interrupt-driven UART RX (no polling)
- Non-blocking main loop (allows multitasking)
- 10ms cycle time with 100µs sleep when no data

---

## SBUS Protocol Specs

- **Baud Rate**: 100,000 bps
- **Format**: 8 data bits, even parity, 2 stop bits (8E2)
- **Frame**: 25 bytes (0x0F + 22 data + flags + 0x00)
- **Channels**: 16 analog (11-bit each) + 2 digital
- **Range**: 0-2047 (typical: 172 min, 992 center, 1811 max)
- **Update Rate**: ~14ms per frame (~70Hz)

---

## Build & Flash

```bash
mkdir build && cd build
cmake ..
make
# Flash transwingC.uf2 to Pico in bootloader mode
```

---

## Hardware Wiring

| Component | Pico Pin | Notes |
|-----------|----------|-------|
| SBUS RX | GPIO 1 (UART0 RX) | 3.3V logic (use inverter if needed) |
| SBUS TX | GPIO 0 (UART0 TX) | Not used (RX only) |
| Servo PWM | GPIO 15 | 50Hz PWM output |
| LED | GPIO 25 | Onboard LED (status) |

**IMPORTANT**: SBUS is inverted serial - may need hardware inverter between receiver and Pico.

---

## Usage Example

```c
// Initialize
sbus_init(SBUS_TX_PIN, SBUS_RX_PIN);
initServo(SERVO_PIN, SERVO_CENTER_US);

// Main loop
uint16_t channels[16];
while (true) {
    if (sbus_read_channels(channels)) {
        // Channel 0 = stick input (172-1811)
        float pulse = map_sbus_to_servo(channels[0]);
        setPulseWidth(SERVO_PIN, pulse);  // 1000-2000 µs
    }
    sleep_ms(10);
}
```

---

## Debug Output (USB Serial)

```
=== Transwing Arm Controller ===
Initializing hardware...
[INFO] Servo initialized on GPIO 15.
[INFO] SBUS initialized on UART0 (TX: GPIO 0, RX: GPIO 1)
Initialization complete. Waiting for SBUS data...

Frame #100 | Ch1: 992 -> 1500.0 us | Failsafe: NO

--- SBUS Statistics ---
Frames received: 350
Invalid frames:  2
Sync events:     1
Buffer overflows: 0
Failsafe: NO | Signal lost: NO
----------------------
```

---

## Files Overview

| File | Lines | Purpose |
|------|-------|---------|
| **transwingC.c** | 90 | Main application & servo integration |
| **sbus2.c** | 240 | SBUS protocol + frame sync + statistics |
| **servo.c** | 70 | PWM servo control |
| **led.c** | 35 | LED utilities |
| **config.h** | 45 | Hardware config & constants |
| **CMakeLists.txt** | 35 | Build configuration |

---

## Notes

- **sbus.c/h**: Old implementation (not compiled), kept for reference
- **Debug Mode**: Uncomment `#define DEBUG_SBUS` in config.h for verbose output
- **Multi-Servo**: Call `initServo()` for each GPIO, all share 50Hz clock
- **Failsafe**: SBUS byte 23 bit 0 indicates receiver failsafe (not yet implemented)