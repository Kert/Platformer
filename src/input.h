#ifndef _input_h_
#define _input_h_ 

#include <SDL.h>

int GetCodeFromInputEvent(SDL_Keycode key, Uint8 jbutton);

bool OnBindPress(int bind); 
void OnBindHold(int bind);
void OnBindUnpress(int bind);

void OnKeyPress(SDL_Keycode key, Uint8 jbutton);
void OnKeyHold(SDL_Keycode key, Uint8 jbutton);
void OnKeyUnpress(SDL_Keycode key, Uint8 jbutton);
void OnHardcodedKeyPress(SDL_Keycode key);

void ProcessInput();
void InitInput();
void InputCleanup();

bool IsBindPressed(int bind);

#endif
