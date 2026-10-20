#include <math.h>
#include "internal.h"
#include "void.h"
#define MAX_GAMEPADS 4

static SDL_GameController *s_controllers[MAX_GAMEPADS] = {NULL, NULL, NULL, NULL};
static float s_gamepad_deadzone = 0.15f;

// ============================================================================
// 1. KEYBOARD
// ============================================================================

bool void_input_is_key_pressed(const VoidKeyCode key) {
    const uint8 *state = SDL_GetKeyboardState(NULL);
    switch (key) {
        // Alphabet
        case VOID_KEY_A: return state[SDL_SCANCODE_A];
        case VOID_KEY_B: return state[SDL_SCANCODE_B];
        case VOID_KEY_C: return state[SDL_SCANCODE_C];
        case VOID_KEY_D: return state[SDL_SCANCODE_D];
        case VOID_KEY_E: return state[SDL_SCANCODE_E];
        case VOID_KEY_F: return state[SDL_SCANCODE_F];
        case VOID_KEY_G: return state[SDL_SCANCODE_G];
        case VOID_KEY_H: return state[SDL_SCANCODE_H];
        case VOID_KEY_I: return state[SDL_SCANCODE_I];
        case VOID_KEY_J: return state[SDL_SCANCODE_J];
        case VOID_KEY_K: return state[SDL_SCANCODE_K];
        case VOID_KEY_L: return state[SDL_SCANCODE_L];
        case VOID_KEY_M: return state[SDL_SCANCODE_M];
        case VOID_KEY_N: return state[SDL_SCANCODE_N];
        case VOID_KEY_O: return state[SDL_SCANCODE_O];
        case VOID_KEY_P: return state[SDL_SCANCODE_P];
        case VOID_KEY_Q: return state[SDL_SCANCODE_Q];
        case VOID_KEY_R: return state[SDL_SCANCODE_R];
        case VOID_KEY_S: return state[SDL_SCANCODE_S];
        case VOID_KEY_T: return state[SDL_SCANCODE_T];
        case VOID_KEY_U: return state[SDL_SCANCODE_U];
        case VOID_KEY_V: return state[SDL_SCANCODE_V];
        case VOID_KEY_W: return state[SDL_SCANCODE_W];
        case VOID_KEY_X: return state[SDL_SCANCODE_X];
        case VOID_KEY_Y: return state[SDL_SCANCODE_Y];
        case VOID_KEY_Z: return state[SDL_SCANCODE_Z];
        // Numbers
        case VOID_KEY_0: return state[SDL_SCANCODE_0];
        case VOID_KEY_1: return state[SDL_SCANCODE_1];
        case VOID_KEY_2: return state[SDL_SCANCODE_2];
        case VOID_KEY_3: return state[SDL_SCANCODE_3];
        case VOID_KEY_4: return state[SDL_SCANCODE_4];
        case VOID_KEY_5: return state[SDL_SCANCODE_5];
        case VOID_KEY_6: return state[SDL_SCANCODE_6];
        case VOID_KEY_7: return state[SDL_SCANCODE_7];
        case VOID_KEY_8: return state[SDL_SCANCODE_8];
        case VOID_KEY_9: return state[SDL_SCANCODE_9];
        // Arrows
        case VOID_KEY_UP:    return state[SDL_SCANCODE_UP];
        case VOID_KEY_DOWN:  return state[SDL_SCANCODE_DOWN];
        case VOID_KEY_LEFT:  return state[SDL_SCANCODE_LEFT];
        case VOID_KEY_RIGHT: return state[SDL_SCANCODE_RIGHT];
        // Modifiers & Control
        case VOID_KEY_SPACE:     return state[SDL_SCANCODE_SPACE];
        case VOID_KEY_ENTER:     return state[SDL_SCANCODE_RETURN];
        case VOID_KEY_ESCAPE:    return state[SDL_SCANCODE_ESCAPE];
        case VOID_KEY_TAB:       return state[SDL_SCANCODE_TAB];
        case VOID_KEY_BACKSPACE: return state[SDL_SCANCODE_BACKSPACE];
        case VOID_KEY_LSHIFT:    return state[SDL_SCANCODE_LSHIFT];
        case VOID_KEY_RSHIFT:    return state[SDL_SCANCODE_RSHIFT];
        case VOID_KEY_LCTRL:     return state[SDL_SCANCODE_LCTRL];
        case VOID_KEY_RCTRL:     return state[SDL_SCANCODE_RCTRL];
        case VOID_KEY_LALT:      return state[SDL_SCANCODE_LALT];
        case VOID_KEY_RALT:      return state[SDL_SCANCODE_RALT];
        // Function Keys
        case VOID_KEY_F1:  return state[SDL_SCANCODE_F1];
        case VOID_KEY_F2:  return state[SDL_SCANCODE_F2];
        case VOID_KEY_F3:  return state[SDL_SCANCODE_F3];
        case VOID_KEY_F4:  return state[SDL_SCANCODE_F4];
        case VOID_KEY_F5:  return state[SDL_SCANCODE_F5];
        case VOID_KEY_F6:  return state[SDL_SCANCODE_F6];
        case VOID_KEY_F7:  return state[SDL_SCANCODE_F7];
        case VOID_KEY_F8:  return state[SDL_SCANCODE_F8];
        case VOID_KEY_F9:  return state[SDL_SCANCODE_F9];
        case VOID_KEY_F10: return state[SDL_SCANCODE_F10];
        case VOID_KEY_F11: return state[SDL_SCANCODE_F11];
        case VOID_KEY_F12: return state[SDL_SCANCODE_F12];
        default: return false;
    }
}

// ============================================================================
// 2. MOUSE (Logical Resolution Aware)
// ============================================================================

void void_input_get_mouse_pos(const VoidWindow *window, float *x, float *y) {
    int win_x = 0, win_y = 0;
    SDL_GetMouseState(&win_x, &win_y);
    if (window && window->render) {
        float log_x = 0.0f, log_y = 0.0f;
        SDL_RenderWindowToLogical(window->render, win_x, win_y, &log_x, &log_y);
        if (x) *x = log_x;
        if (y) *y = log_y;
    } else {
        if (x) *x = (float)win_x;
        if (y) *y = (float)win_y;
    }
}

bool void_input_is_mouse_pressed(const VoidMouseButton button) {
    const uint32 mask = SDL_GetMouseState(NULL, NULL);
    return (mask & SDL_BUTTON((uint32)button)) != 0;
}

// ============================================================================
// 3. GAMEPADS (Hot-plugging & Deadzone)
// ============================================================================

static SDL_GameController *get_or_open_controller(const int index) {
    if (index < 0 || index >= MAX_GAMEPADS) return NULL;
    if (s_controllers[index]) {
        if (SDL_GameControllerGetAttached(s_controllers[index])) {
            return s_controllers[index];
        }
        SDL_GameControllerClose(s_controllers[index]);
        s_controllers[index] = NULL;
    }
    const int num_joysticks = SDL_NumJoysticks();
    int found_gamepad = 0;
    for (int i = 0; i < num_joysticks; ++i) {
        if (SDL_IsGameController(i)) {
            if (found_gamepad == index) {
                s_controllers[index] = SDL_GameControllerOpen(i);
                return s_controllers[index];
            }
            found_gamepad++;
        }
    }
    return NULL;
}

bool void_input_is_gamepad_connected(const int gamepad_index) {
    return get_or_open_controller(gamepad_index) != NULL;
}

void void_input_set_gamepad_deadzone(const float deadzone) {
    if (deadzone >= 0.0f && deadzone < 0.95f) {
        s_gamepad_deadzone = deadzone;
    }
}

float void_input_get_gamepad_axis(const int gamepad_index, const VoidGamepadAxis axis) {
    SDL_GameController *pad = get_or_open_controller(gamepad_index);
    if (!pad) return 0.0f;
    SDL_GameControllerAxis sdl_axis;
    switch (axis) {
        case VOID_GAMEPAD_AXIS_LEFTX:        sdl_axis = SDL_CONTROLLER_AXIS_LEFTX; break;
        case VOID_GAMEPAD_AXIS_LEFTY:        sdl_axis = SDL_CONTROLLER_AXIS_LEFTY; break;
        case VOID_GAMEPAD_AXIS_RIGHTX:       sdl_axis = SDL_CONTROLLER_AXIS_RIGHTX; break;
        case VOID_GAMEPAD_AXIS_RIGHTY:       sdl_axis = SDL_CONTROLLER_AXIS_RIGHTY; break;
        case VOID_GAMEPAD_AXIS_TRIGGERLEFT:  sdl_axis = SDL_CONTROLLER_AXIS_TRIGGERLEFT; break;
        case VOID_GAMEPAD_AXIS_TRIGGERRIGHT: sdl_axis = SDL_CONTROLLER_AXIS_TRIGGERRIGHT; break;
        default: return 0.0f;
    }
    const int16 raw = SDL_GameControllerGetAxis(pad, sdl_axis);
    const float norm = raw < 0 ? (float)raw / 32768.0f : (float)raw / 32767.0f;
    if (fabsf(norm) < s_gamepad_deadzone) {
        return 0.0f;
    }
    const float sign = (norm < 0.0f) ? -1.0f : 1.0f;
    return sign * ((fabsf(norm) - s_gamepad_deadzone) / (1.0f - s_gamepad_deadzone));
}

bool void_input_is_gamepad_pressed(const int gamepad_index, const VoidGamepadButton button) {
    SDL_GameController *pad = get_or_open_controller(gamepad_index);
    if (!pad) return false;
    SDL_GameControllerButton sdl_btn;
    switch (button) {
        case VOID_GAMEPAD_BUTTON_SOUTH:         sdl_btn = SDL_CONTROLLER_BUTTON_A; break;
        case VOID_GAMEPAD_BUTTON_EAST:          sdl_btn = SDL_CONTROLLER_BUTTON_B; break;
        case VOID_GAMEPAD_BUTTON_WEST:          sdl_btn = SDL_CONTROLLER_BUTTON_X; break;
        case VOID_GAMEPAD_BUTTON_NORTH:         sdl_btn = SDL_CONTROLLER_BUTTON_Y; break;
        case VOID_GAMEPAD_BUTTON_BACK:          sdl_btn = SDL_CONTROLLER_BUTTON_BACK; break;
        case VOID_GAMEPAD_BUTTON_GUIDE:         sdl_btn = SDL_CONTROLLER_BUTTON_GUIDE; break;
        case VOID_GAMEPAD_BUTTON_START:         sdl_btn = SDL_CONTROLLER_BUTTON_START; break;
        case VOID_GAMEPAD_BUTTON_LEFTSTICK:     sdl_btn = SDL_CONTROLLER_BUTTON_LEFTSTICK; break;
        case VOID_GAMEPAD_BUTTON_RIGHTSTICK:    sdl_btn = SDL_CONTROLLER_BUTTON_RIGHTSTICK; break;
        case VOID_GAMEPAD_BUTTON_LEFTSHOULDER:  sdl_btn = SDL_CONTROLLER_BUTTON_LEFTSHOULDER; break;
        case VOID_GAMEPAD_BUTTON_RIGHTSHOULDER: sdl_btn = SDL_CONTROLLER_BUTTON_RIGHTSHOULDER; break;
        case VOID_GAMEPAD_BUTTON_DPAD_UP:       sdl_btn = SDL_CONTROLLER_BUTTON_DPAD_UP; break;
        case VOID_GAMEPAD_BUTTON_DPAD_DOWN:     sdl_btn = SDL_CONTROLLER_BUTTON_DPAD_DOWN; break;
        case VOID_GAMEPAD_BUTTON_DPAD_LEFT:     sdl_btn = SDL_CONTROLLER_BUTTON_DPAD_LEFT; break;
        case VOID_GAMEPAD_BUTTON_DPAD_RIGHT:    sdl_btn = SDL_CONTROLLER_BUTTON_DPAD_RIGHT; break;
        default: return false;
    }
    return SDL_GameControllerGetButton(pad, sdl_btn) != 0;
}