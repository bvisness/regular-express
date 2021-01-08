#pragma once

#include "../microui.h"

typedef struct TextInputState {
    int InsertIndex; // The index where a new element will be inserted.

    int CursorIndex; // The item the cursor will be rendered on.
    int CursorRight; // If 1, draw the cursor on the right of the item.

    int SelectionBase;
} TextInputState;

extern const TextInputState DEFAULT_TEXT_INPUT_STATE;

TextInputState TextState_SetInsertIndex(TextInputState state, int i, int select);
TextInputState TextState_MoveCursor(TextInputState state, int delta, int select);
TextInputState TextState_BumpCursor(TextInputState state, int direction, int select);
TextInputState TextState_SetCursorRight(TextInputState state, int cursorRight);
TextInputState TextState_Clamp(TextInputState state, int minInsertIndex, int maxInsertIndex);

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
