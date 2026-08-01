#include <SDL2/SDL.h>
#include "void.h"

bool void_input_is_key_pressed(const VoidKeyCode key) {
    const uint8 *state = (uint8*)SDL_GetKeyboardState(NULL);
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
        //
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
        //
        case VOID_KEY_UP:    return state[SDL_SCANCODE_UP];
        case VOID_KEY_DOWN:  return state[SDL_SCANCODE_DOWN];
        case VOID_KEY_LEFT:  return state[SDL_SCANCODE_LEFT];
        case VOID_KEY_RIGHT: return state[SDL_SCANCODE_RIGHT];
        //
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
        //
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
