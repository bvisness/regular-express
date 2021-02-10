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
TextInputState TextState_SetCursorIndex(int i, int cursorRight);
TextInputState TextState_MoveCursor(TextInputState state, int delta, int select);
TextInputState TextState_BumpCursor(TextInputState state, int direction, int select);
TextInputState TextState_SetCursorRight(TextInputState state, int cursorRight);
TextInputState TextState_SelectRange(int min, int max);
TextInputState TextState_Clamp(TextInputState state, int minInsertIndex, int maxInsertIndex);

int TextState_IsSelecting(TextInputState state);
int TextState_SelectionStart(TextInputState state);
int TextState_SelectionEnd(TextInputState state);
int TextState_IsSelected(TextInputState state, int index);

void TextState_Print(TextInputState state);

typedef struct TextEditResult {
    /*
    If DoDelete is true, the caller should delete the range of
    items from DeleteMin to DeleteMax (exclusive). This should
    happen before any input is performed.
    */
    int DoDelete;
    int DeleteMin;
    int DeleteMax;

    /*
    The desired text state after deletion but before insertion.
    You may always set your text state to this no matter the value
    of DoDelete.
    */
    TextInputState ResultState;

    /*
    If DoInput is true, then the caller should insert the input
    text from microui. The actual insert data is not included here
    because the details of insertion are specific to the caller and
    the actual input text is in microui.

    If a modifier key is pressed and the shortcut is not recognized,
    this will _not_ be set, so check for nonstandard modifiers and
    handle them yourself regardless of whether DoInput is true.
    */
    int DoInput;
} TextEditResult;

TextEditResult TextState_DeleteBackwards(TextInputState state);
TextEditResult TextState_DeleteForwards(TextInputState state);
TextEditResult TextState_InsertString(TextInputState state);

TextEditResult StandardTextInput(mu_Context* ctx, TextInputState textState, int maxIndex);
