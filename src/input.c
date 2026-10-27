#include <math.h>
#include <string.h>
#include "internal.h"
#include "void.h"

#define MAX_GAMEPADS 4
#define NUM_GAMEPAD_BUTTONS 15

static SDL_GameController *s_controllers[MAX_GAMEPADS] = {NULL, NULL, NULL, NULL};
static float  s_gamepad_deadzone = 0.15f;

static uint8  s_prev_keys[SDL_NUM_SCANCODES] = {0};
static uint32 s_prev_mouse_mask = 0;
static uint16 s_prev_pad_buttons[MAX_GAMEPADS] = {0};
static float  s_mouse_wheel_x = 0.0f;
static float  s_mouse_wheel_y = 0.0f;

// ============================================================================
// INTERNAL MAPPING HELPERS
// ============================================================================

static SDL_Scancode key_to_scancode(const VoidKeyCode key) {
    switch (key) {
        case VOID_KEY_A: return SDL_SCANCODE_A;
        case VOID_KEY_B: return SDL_SCANCODE_B;
        case VOID_KEY_C: return SDL_SCANCODE_C;
        case VOID_KEY_D: return SDL_SCANCODE_D;
        case VOID_KEY_E: return SDL_SCANCODE_E;
        case VOID_KEY_F: return SDL_SCANCODE_F;
        case VOID_KEY_G: return SDL_SCANCODE_G;
        case VOID_KEY_H: return SDL_SCANCODE_H;
        case VOID_KEY_I: return SDL_SCANCODE_I;
        case VOID_KEY_J: return SDL_SCANCODE_J;
        case VOID_KEY_K: return SDL_SCANCODE_K;
        case VOID_KEY_L: return SDL_SCANCODE_L;
        case VOID_KEY_M: return SDL_SCANCODE_M;
        case VOID_KEY_N: return SDL_SCANCODE_N;
        case VOID_KEY_O: return SDL_SCANCODE_O;
        case VOID_KEY_P: return SDL_SCANCODE_P;
        case VOID_KEY_Q: return SDL_SCANCODE_Q;
        case VOID_KEY_R: return SDL_SCANCODE_R;
        case VOID_KEY_S: return SDL_SCANCODE_S;
        case VOID_KEY_T: return SDL_SCANCODE_T;
        case VOID_KEY_U: return SDL_SCANCODE_U;
        case VOID_KEY_V: return SDL_SCANCODE_V;
        case VOID_KEY_W: return SDL_SCANCODE_W;
        case VOID_KEY_X: return SDL_SCANCODE_X;
        case VOID_KEY_Y: return SDL_SCANCODE_Y;
        case VOID_KEY_Z: return SDL_SCANCODE_Z;
        case VOID_KEY_0: return SDL_SCANCODE_0;
        case VOID_KEY_1: return SDL_SCANCODE_1;
        case VOID_KEY_2: return SDL_SCANCODE_2;
        case VOID_KEY_3: return SDL_SCANCODE_3;
        case VOID_KEY_4: return SDL_SCANCODE_4;
        case VOID_KEY_5: return SDL_SCANCODE_5;
        case VOID_KEY_6: return SDL_SCANCODE_6;
        case VOID_KEY_7: return SDL_SCANCODE_7;
        case VOID_KEY_8: return SDL_SCANCODE_8;
        case VOID_KEY_9: return SDL_SCANCODE_9;
        case VOID_KEY_UP:        return SDL_SCANCODE_UP;
        case VOID_KEY_DOWN:      return SDL_SCANCODE_DOWN;
        case VOID_KEY_LEFT:      return SDL_SCANCODE_LEFT;
        case VOID_KEY_RIGHT:     return SDL_SCANCODE_RIGHT;
        case VOID_KEY_SPACE:     return SDL_SCANCODE_SPACE;
        case VOID_KEY_ENTER:     return SDL_SCANCODE_RETURN;
        case VOID_KEY_ESCAPE:    return SDL_SCANCODE_ESCAPE;
        case VOID_KEY_TAB:       return SDL_SCANCODE_TAB;
        case VOID_KEY_BACKSPACE: return SDL_SCANCODE_BACKSPACE;
        case VOID_KEY_LSHIFT:    return SDL_SCANCODE_LSHIFT;
        case VOID_KEY_RSHIFT:    return SDL_SCANCODE_RSHIFT;
        case VOID_KEY_LCTRL:     return SDL_SCANCODE_LCTRL;
        case VOID_KEY_RCTRL:     return SDL_SCANCODE_RCTRL;
        case VOID_KEY_LALT:      return SDL_SCANCODE_LALT;
        case VOID_KEY_RALT:      return SDL_SCANCODE_RALT;
        case VOID_KEY_F1:        return SDL_SCANCODE_F1;
        case VOID_KEY_F2:        return SDL_SCANCODE_F2;
        case VOID_KEY_F3:        return SDL_SCANCODE_F3;
        case VOID_KEY_F4:        return SDL_SCANCODE_F4;
        case VOID_KEY_F5:        return SDL_SCANCODE_F5;
        case VOID_KEY_F6:        return SDL_SCANCODE_F6;
        case VOID_KEY_F7:        return SDL_SCANCODE_F7;
        case VOID_KEY_F8:        return SDL_SCANCODE_F8;
        case VOID_KEY_F9:        return SDL_SCANCODE_F9;
        case VOID_KEY_F10:       return SDL_SCANCODE_F10;
        case VOID_KEY_F11:       return SDL_SCANCODE_F11;
        case VOID_KEY_F12:       return SDL_SCANCODE_F12;
        default:                 return SDL_SCANCODE_UNKNOWN;
    }
}

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

static SDL_GameControllerButton map_gamepad_button(const VoidGamepadButton button) {
    switch (button) {
        case VOID_GAMEPAD_BUTTON_SOUTH:         return SDL_CONTROLLER_BUTTON_A;
        case VOID_GAMEPAD_BUTTON_EAST:          return SDL_CONTROLLER_BUTTON_B;
        case VOID_GAMEPAD_BUTTON_WEST:          return SDL_CONTROLLER_BUTTON_X;
        case VOID_GAMEPAD_BUTTON_NORTH:         return SDL_CONTROLLER_BUTTON_Y;
        case VOID_GAMEPAD_BUTTON_BACK:          return SDL_CONTROLLER_BUTTON_BACK;
        case VOID_GAMEPAD_BUTTON_GUIDE:         return SDL_CONTROLLER_BUTTON_GUIDE;
        case VOID_GAMEPAD_BUTTON_START:         return SDL_CONTROLLER_BUTTON_START;
        case VOID_GAMEPAD_BUTTON_LEFTSTICK:     return SDL_CONTROLLER_BUTTON_LEFTSTICK;
        case VOID_GAMEPAD_BUTTON_RIGHTSTICK:    return SDL_CONTROLLER_BUTTON_RIGHTSTICK;
        case VOID_GAMEPAD_BUTTON_LEFTSHOULDER:  return SDL_CONTROLLER_BUTTON_LEFTSHOULDER;
        case VOID_GAMEPAD_BUTTON_RIGHTSHOULDER: return SDL_CONTROLLER_BUTTON_RIGHTSHOULDER;
        case VOID_GAMEPAD_BUTTON_DPAD_UP:       return SDL_CONTROLLER_BUTTON_DPAD_UP;
        case VOID_GAMEPAD_BUTTON_DPAD_DOWN:     return SDL_CONTROLLER_BUTTON_DPAD_DOWN;
        case VOID_GAMEPAD_BUTTON_DPAD_LEFT:     return SDL_CONTROLLER_BUTTON_DPAD_LEFT;
        case VOID_GAMEPAD_BUTTON_DPAD_RIGHT:    return SDL_CONTROLLER_BUTTON_DPAD_RIGHT;
        default:                                return SDL_CONTROLLER_BUTTON_INVALID;
    }
}

void void_input_begin_frame(void) {
    int num_keys = 0;
    const uint8 *kb = SDL_GetKeyboardState(&num_keys);
    if (kb && num_keys > 0) {
        const int copy_count = (num_keys < SDL_NUM_SCANCODES) ? num_keys : SDL_NUM_SCANCODES;
        memcpy(s_prev_keys, kb, (size_t)copy_count);
    }
    s_prev_mouse_mask = SDL_GetMouseState(NULL, NULL);
    for (int idx = 0; idx < MAX_GAMEPADS; ++idx) {
        uint16 mask = 0;
        SDL_GameController *pad = s_controllers[idx];
        if (pad && SDL_GameControllerGetAttached(pad)) {
            for (int b = 0; b < NUM_GAMEPAD_BUTTONS; ++b) {
                if (SDL_GameControllerGetButton(pad, map_gamepad_button((VoidGamepadButton)b))) {
                    mask |= (uint16)(1u << b);
                }
            }
        }
        s_prev_pad_buttons[idx] = mask;
    }
    s_mouse_wheel_x = 0.0f;
    s_mouse_wheel_y = 0.0f;
}

void void_input_on_mouse_wheel(const float x, const float y) {
    s_mouse_wheel_x += x;
    s_mouse_wheel_y += y;
}

// ============================================================================
// 1. KEYBOARD
// ============================================================================

bool void_input_is_key_pressed(const VoidKeyCode key) {
    const SDL_Scancode sc = key_to_scancode(key);
    if (sc == SDL_SCANCODE_UNKNOWN) return false;
    const uint8 *state = SDL_GetKeyboardState(NULL);
    return state[sc] != 0;
}

bool void_input_is_key_just_pressed(const VoidKeyCode key) {
    const SDL_Scancode sc = key_to_scancode(key);
    if (sc == SDL_SCANCODE_UNKNOWN) return false;
    const uint8 *state = SDL_GetKeyboardState(NULL);
    return (state[sc] != 0) && (s_prev_keys[sc] == 0);
}

bool void_input_is_key_just_released(const VoidKeyCode key) {
    const SDL_Scancode sc = key_to_scancode(key);
    if (sc == SDL_SCANCODE_UNKNOWN) return false;
    const uint8 *state = SDL_GetKeyboardState(NULL);
    return (state[sc] == 0) && (s_prev_keys[sc] != 0);
}

// ============================================================================
// 2. MOUSE (Logical Resolution Aware & Wheel)
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

void void_input_get_mouse_wheel(float *x, float *y) {
    if (x) *x = s_mouse_wheel_x;
    if (y) *y = s_mouse_wheel_y;
}

bool void_input_is_mouse_pressed(const VoidMouseButton button) {
    const uint32 mask = SDL_GetMouseState(NULL, NULL);
    return (mask & SDL_BUTTON((uint32)button)) != 0;
}

bool void_input_is_mouse_just_pressed(const VoidMouseButton button) {
    const uint32 bit = SDL_BUTTON((uint32)button);
    const uint32 curr = SDL_GetMouseState(NULL, NULL);
    return ((curr & bit) != 0) && ((s_prev_mouse_mask & bit) == 0);
}

bool void_input_is_mouse_just_released(const VoidMouseButton button) {
    const uint32 bit = SDL_BUTTON((uint32)button);
    const uint32 curr = SDL_GetMouseState(NULL, NULL);
    return ((curr & bit) == 0) && ((s_prev_mouse_mask & bit) != 0);
}

// ============================================================================
// 3. GAMEPADS (Hot-plugging, Deadzone, Edge Detection & Rumble)
// ============================================================================

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
    const float norm = (raw < 0) ? ((float)raw / 32768.0f) : ((float)raw / 32767.0f);
    if (fabsf(norm) < s_gamepad_deadzone) {
        return 0.0f;
    }
    const float sign = (norm < 0.0f) ? -1.0f : 1.0f;
    return sign * ((fabsf(norm) - s_gamepad_deadzone) / (1.0f - s_gamepad_deadzone));
}

bool void_input_is_gamepad_pressed(const int gamepad_index, const VoidGamepadButton button) {
    SDL_GameController *pad = get_or_open_controller(gamepad_index);
    if (!pad) return false;
    const SDL_GameControllerButton sdl_btn = map_gamepad_button(button);
    if (sdl_btn == SDL_CONTROLLER_BUTTON_INVALID) return false;
    return SDL_GameControllerGetButton(pad, sdl_btn) != 0;
}

bool void_input_is_gamepad_just_pressed(const int gamepad_index, const VoidGamepadButton button) {
    if (gamepad_index < 0 || gamepad_index >= MAX_GAMEPADS || button < 0 || button >= NUM_GAMEPAD_BUTTONS) return false;
    const bool curr = void_input_is_gamepad_pressed(gamepad_index, button);
    const bool prev = (s_prev_pad_buttons[gamepad_index] & (uint16)(1u << button)) != 0;
    return curr && !prev;
}

bool void_input_is_gamepad_just_released(const int gamepad_index, const VoidGamepadButton button) {
    if (gamepad_index < 0 || gamepad_index >= MAX_GAMEPADS || button < 0 || button >= NUM_GAMEPAD_BUTTONS) return false;
    const bool curr = void_input_is_gamepad_pressed(gamepad_index, button);
    const bool prev = (s_prev_pad_buttons[gamepad_index] & (uint16)(1u << button)) != 0;
    return !curr && prev;
}

bool void_input_rumble_gamepad(const int gamepad_index, float low_freq, float high_freq, const uint32 duration_ms) {
    SDL_GameController *pad = get_or_open_controller(gamepad_index);
    if (!pad) return false;
    if (low_freq < 0.0f)  low_freq = 0.0f;
    if (low_freq > 1.0f)  low_freq = 1.0f;
    if (high_freq < 0.0f) high_freq = 0.0f;
    if (high_freq > 1.0f) high_freq = 1.0f;
    const uint16 low_val  = (uint16)(low_freq * 65535.0f);
    const uint16 high_val = (uint16)(high_freq * 65535.0f);
    return SDL_GameControllerRumble(pad, low_val, high_val, duration_ms) == 0;
}