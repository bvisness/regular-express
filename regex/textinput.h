#ifndef TEXTINPUT_H
#define TEXTINPUT_H

#include "../microui.h"

typedef struct TextInputState {
    int CursorPosition;
    int SelectionBase;
} TextInputState;

extern const TextInputState DEFAULT_TEXT_INPUT_STATE;

TextInputState TextState_SetCursorPosition(TextInputState state, int i, int select);
TextInputState TextState_MoveCursor(TextInputState state, int delta, int select);
TextInputState TextState_BumpCursor(TextInputState state, int direction, int select);

int TextState_IsSelecting(TextInputState state);
int TextState_SelectionStart(TextInputState state);
int TextState_SelectionEnd(TextInputState state);
int TextState_IsSelected(TextInputState state, int index);

typedef struct TextEditResult {
    int DoDelete;
    int DeleteMin;
    int DeleteMax;

    int DoInput;

    TextInputState ResultState;

    /*
    Insert data is not included here; it's up to the control to actually
    perform the insert after calling TextState_InsertString and handling
    whatever actions it was told to do.
    */
} TextEditResult;

TextEditResult TextState_DeleteBackwards(TextInputState state);
TextEditResult TextState_DeleteForwards(TextInputState state);
TextEditResult TextState_InsertString(TextInputState state);

TextEditResult StandardTextInput(mu_Context* ctx, TextInputState textState, int maxIndex);

#endif
