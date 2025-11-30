
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

#ifdef _WIN32
#define EXPORT __declspec(dllexport)
#else
#define EXPORT
#endif

#define NUM_GEARS 5

typedef struct {
    double min_speed;
    double max_speed;
} GearWindow;

static const GearWindow gear_windows[NUM_GEARS] = {
    {0.0, 32.0},
    {32.0, 64.0},
    {64.0, 96.0},
    {96.0, 128.0},
    {128.0, 160.0}
};

static const double anticipatory_margin = 3.0;
static const double accel_rate = 25.0;
static const double brake_rate = 45.0;
static const double coast_drag = 4.0;
static const double max_speed = 160.0;

static double g_speed = 0.0;
static int g_gear = 1;

static int clamp_gear(int gear) {
    if (gear < 1) return 1;
    if (gear > NUM_GEARS) return NUM_GEARS;
    return gear;
}

static void apply_shift_logic(void) {
    g_gear = clamp_gear(g_gear);
    const GearWindow *window = &gear_windows[g_gear - 1];

    if (g_gear < NUM_GEARS && g_speed >= window->max_speed - anticipatory_margin) {
        g_gear++;
        g_gear = clamp_gear(g_gear);  // Ensure gear doesn't exceed NUM_GEARS
    } else if (g_gear > 1 && g_speed <= window->min_speed + anticipatory_margin) {
        g_gear--;
        g_gear = clamp_gear(g_gear);  // Ensure gear doesn't go below 1
    }
}

EXPORT void gearbox_reset(void) {
    g_speed = 0.0;
    g_gear = 1;
}

EXPORT void gearbox_step(int accelerating, int braking, double dt_seconds) {
    if (dt_seconds <= 0) dt_seconds = 0.05;
    double delta_v = 0.0;
    if (accelerating) {
        delta_v += accel_rate * dt_seconds;
    } else {
        delta_v -= coast_drag * dt_seconds;
    }
    if (braking) {
        delta_v -= brake_rate * dt_seconds;
    }

    g_speed += delta_v;
    if (g_speed < 0.0) g_speed = 0.0;
    if (g_speed > max_speed) g_speed = max_speed;

    apply_shift_logic();
}

EXPORT double gearbox_get_speed(void) {
    return g_speed;
}

EXPORT int gearbox_get_gear(void) {
    g_gear = clamp_gear(g_gear);  // Final safety clamp before returning
    return g_gear;
}

static int is_key_down(int virtual_key) {
    return (GetAsyncKeyState(virtual_key) & 0x8000) != 0;
}

static void draw_speedometer(double speed) {
    const int bar_width = 40;
    int filled = (int)((speed / max_speed) * bar_width);
    if (filled > bar_width) filled = bar_width;
    if (filled < 0) filled = 0;

    printf("Speedometer: [");
    for (int i = 0; i < bar_width; ++i) {
        if (i < filled) printf("#");
        else printf(" ");
    }
    printf("] %.1f km/h\n", speed);
}

#ifndef GEARBOX_DLL_ONLY
int main(void) {
    const double dt_seconds = 0.06;
    const double tick_ms = dt_seconds * 1000.0;

    gearbox_reset();
    printf("Interactive Gearbox Simulator (Arrow keys)\n");
    printf("Hold Up to accelerate, Down to brake, Esc to exit.\n");
    Sleep(1200);

    while (1) {
        if (is_key_down(VK_ESCAPE)) break;

        int accelerating = is_key_down(VK_UP);
        int braking = is_key_down(VK_DOWN);
        gearbox_step(accelerating, braking, dt_seconds);

        system("cls");
        printf("Interactive Gearbox Simulator\n");
        printf("[UP] Accelerate  [DOWN] Brake  [ESC] Quit\n\n");
        draw_speedometer(gearbox_get_speed());
        printf("Current Gear: %d\n", gearbox_get_gear());
        printf("Accelerating: %s\n", accelerating ? "YES" : "NO");
        printf("Braking: %s\n", braking ? "YES" : "NO");
        printf("\nGear ranges:\n");
        for (int i = 0; i < NUM_GEARS; ++i) {
            printf("G%-2d: %3.0f - %3.0f km/h\n", i + 1, gear_windows[i].min_speed, gear_windows[i].max_speed);
        }

        Sleep((DWORD)tick_ms);
    }

    system("cls");
    printf("Simulation ended.\n");
    return 0;
}
#endif