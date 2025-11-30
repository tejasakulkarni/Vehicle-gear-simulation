# Gearbox Simulator

A real-time automotive gearbox simulation with automatic transmission logic, featuring a web-based dashboard visualization. The core simulation engine is written in C, providing precise control over gear shifting mechanics, while a modern HTML/CSS/JavaScript frontend offers an intuitive interface for real-time monitoring and control.

## Features

- **5-Gear Automatic Transmission**: Realistic gear shifting based on speed windows with anticipatory margin
- **Real-time Simulation**: 60ms tick rate for smooth speed and gear transitions
- **Interactive Controls**: Hold-to-accelerate and hold-to-brake controls
- **Beautiful Web Dashboard**: Modern, responsive UI with speed gauge, gear indicator, and telemetry
- **Pure C Backend**: Lightweight HTTP server serving the simulation logic
- **Automatic Gear Shifting**: Smart upshift/downshift logic based on vehicle speed

## Architecture

```
┌─────────────────┐
│   Web Browser   │
│  (index.html)   │
└────────┬────────┘
         │ HTTP GET /step?accelerate=...&brake=...
         │ HTTP GET /reset
         ▼
┌─────────────────┐
│  gear_server.c  │  ← C HTTP Server (port 8080)
│  (Backend API)  │
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│    demo.c       │  ← Core Gearbox Logic
│  (Simulation)   │     - gearbox_step()
│                 │     - gearbox_get_speed()
│                 │     - gearbox_get_gear()
└─────────────────┘
```

## Gear Configuration

The simulator implements a 5-gear automatic transmission with the following speed windows:

| Gear | Speed Range (km/h) |
|------|-------------------|
| 1    | 0 - 32            |
| 2    | 32 - 64           |
| 3    | 64 - 96           |
| 4    | 96 - 128          |
| 5    | 128 - 160         |

Gear shifting occurs automatically with a 3 km/h anticipatory margin to prevent gear hunting.

## Building and Running

### Prerequisites

- **Windows** with MinGW-w64 (or Visual Studio Build Tools)
- GCC compiler with Winsock support
- A modern web browser

### Build Instructions

1. **Clone the repository:**
   ```bash
   git clone <your-repo-url>
   cd programs
   ```

2. **Compile the C server:**
   ```bash
   gcc gear_server.c demo.c -DGEARBOX_DLL_ONLY -lws2_32 -o gear_server.exe
   ```

   For Visual Studio Build Tools:
   ```cmd
   cl /LD gear_server.c demo.c /DGEARBOX_DLL_ONLY /link ws2_32.lib
   ```

3. **Run the server:**
   ```bash
   .\gear_server.exe
   ```

   You should see:
   ```
   Gearbox C server running on http://localhost:8080
   ```

4. **Open the dashboard:**
   - Open your browser and navigate to: `http://localhost:8080/`
   - Use the **Accelerate** and **Brake** buttons to control the simulation
   - The dashboard displays real-time speed, current gear, and telemetry

## Project Structure

```
programs/
├── demo.c              # Core gearbox simulation logic
├── gear_server.c       # C HTTP server implementation
├── index.html          # Web dashboard UI
├── README.md           # This file
└── gear_log.csv        # Optional logging (if implemented)
```

## API Endpoints

The C server exposes the following endpoints:

- **`GET /`** or **`GET /index.html`** - Serves the HTML dashboard
- **`GET /step?accelerate=0|1&brake=0|1`** - Executes one simulation step and returns JSON:
  ```json
  {"speed": 45.6, "gear": 2}
  ```
- **`GET /reset`** - Resets the simulation to initial state (speed=0, gear=1)

## Simulation Parameters

- **Acceleration Rate**: 25.0 km/h/s
- **Braking Rate**: 45.0 km/h/s
- **Coast Drag**: 4.0 km/h/s
- **Maximum Speed**: 160 km/h
- **Tick Rate**: 60ms (0.06 seconds)
- **Anticipatory Margin**: 3.0 km/h

## Technologies

- **Backend**: Pure C (C11) with Winsock2 for networking
- **Frontend**: HTML5, CSS3, JavaScript (ES6+)
- **Protocol**: HTTP/1.1
- **Data Format**: JSON

## License

[Specify your license here]

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request.

## Author

[Your Name]

---

**Note**: This project requires Windows for the Winsock networking library. For Linux/macOS support, modifications to use POSIX sockets would be needed.

