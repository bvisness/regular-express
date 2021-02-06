#include "textinput.h"

#include "../util/math.h"

#include <stdio.h>

const TextInputState DEFAULT_TEXT_INPUT_STATE = (TextInputState) {
    .InsertIndex = 0,
    .CursorIndex = 0,
    .CursorRight = 0,
    .SelectionBase = -1,
};

TextInputState fixupState(TextInputState state) {
    if (state.InsertIndex == state.SelectionBase) {
        return (TextInputState) {
            .InsertIndex = state.InsertIndex,
            .CursorIndex = state.CursorIndex,
            .CursorRight = state.CursorRight,
            .SelectionBase = -1,
        };
    }

    return state;
}

TextInputState TextState_SetInsertIndex(TextInputState state, int i, int select) {
    TextInputState result;

    if (select) {
        result = (TextInputState) {
            .InsertIndex = i,
            .SelectionBase = (state.SelectionBase == -1 ? state.InsertIndex : state.SelectionBase),
        };
    } else {
        result = (TextInputState) {
            .InsertIndex = i,
            .SelectionBase = -1,
        };
    }

    result.CursorIndex = i;
    result.CursorRight = 0;

    return fixupState(result);
}

TextInputState TextState_SetCursorIndex(int i, int cursorRight) {
    return (TextInputState) {
        .InsertIndex = cursorRight ? i + 1 : i,
        .CursorIndex = i,
        .CursorRight = cursorRight,
        .SelectionBase = -1,
    };
}

TextInputState TextState_MoveCursor(TextInputState state, int delta, int select) {
    TextInputState result = TextState_SetInsertIndex(state, state.InsertIndex + delta, select);

    int cursorRight = delta > 0;
    if (result.SelectionBase != -1) {
        cursorRight = result.InsertIndex > result.SelectionBase;
    }
    result = TextState_SetCursorRight(result, cursorRight);

    return result;
}

TextInputState TextState_BumpCursor(TextInputState state, int direction, int select) {
    if (!select && state.SelectionBase != -1) {
        int destination = (direction > 0
            ? imax(state.InsertIndex, state.SelectionBase)
            : imin(state.InsertIndex, state.SelectionBase)
        );
        return TextState_SetCursorRight(
            TextState_SetInsertIndex(state, destination, 0),
            direction > 0
        );
    }

    return TextState_MoveCursor(state, (direction > 0 ? 1 : -1), select);
}

TextInputState TextState_SetCursorRight(TextInputState state, int cursorRight) {
    TextInputState result = state;

    if (cursorRight) {
        result.CursorIndex = state.InsertIndex - 1;
        result.CursorRight = 1;
    } else {
        result.CursorIndex = state.InsertIndex;
        result.CursorRight = 0;
    }

    return result;
}

TextInputState TextState_SelectRange(int min, int max) {
    return (TextInputState) {
        .InsertIndex = max + 1,
        .CursorIndex = max,
        .CursorRight = 1,
        .SelectionBase = min,
    };
}

TextInputState TextState_Clamp(TextInputState state, int minInsertIndex, int maxInsertIndex) {
    TextInputState result = state;

    result.InsertIndex = iclamp(result.InsertIndex, minInsertIndex, maxInsertIndex);

    if (result.CursorIndex < minInsertIndex) {
        result.CursorIndex = minInsertIndex;
        result.CursorRight = 0;
    }

    if (result.CursorIndex >= maxInsertIndex) {
        result.CursorIndex = maxInsertIndex - 1;
        result.CursorRight = 1;
    }

    return result;
}

int TextState_IsSelecting(TextInputState state) {
    return state.SelectionBase != -1;
}

int TextState_SelectionStart(TextInputState state) {
    return imin(state.InsertIndex, state.SelectionBase);
}

int TextState_SelectionEnd(TextInputState state) {
    return imax(state.InsertIndex, state.SelectionBase) - 1;
}

int TextState_IsSelected(TextInputState state, int index) {
    if (state.SelectionBase == -1) {
        return 0;
    }

    int min = imin(state.InsertIndex, state.SelectionBase);
    int max = imax(state.InsertIndex, state.SelectionBase);
    return min <= index && index < max;
}

void TextState_Print(TextInputState state) {
    printf("Text input state:\n- Insert index: %d\n- Cursor index: %d\n- Cursor right? %d\n- Selection base: %d", state.InsertIndex, state.CursorIndex, state.CursorRight, state.SelectionBase);
}

TextEditResult TextState_DeleteBackwards(TextInputState state) {
    TextEditResult result = {0};

    result.DoDelete = 1;

    if (state.SelectionBase != -1) {
        result.DeleteMin = imin(state.InsertIndex, state.SelectionBase);
        result.DeleteMax = imax(state.InsertIndex, state.SelectionBase);

        result.ResultState = (TextInputState) {
            .InsertIndex = imin(state.InsertIndex, state.SelectionBase),
            .SelectionBase = -1,
        };
    } else {
        result.DeleteMin = state.InsertIndex - 1;
        result.DeleteMax = state.InsertIndex;

        result.ResultState = (TextInputState) {
            .InsertIndex = state.InsertIndex - 1,
            .SelectionBase = -1,
        };
    }

    result.ResultState.CursorRight = 1;
    result.ResultState.CursorIndex = result.ResultState.InsertIndex - 1;

    return result;
}

TextEditResult TextState_DeleteForwards(TextInputState state) {
    TextEditResult result = {0};

    result.DoDelete = 1;

    if (state.SelectionBase != -1) {
        result.DeleteMin = imin(state.InsertIndex, state.SelectionBase);
        result.DeleteMax = imax(state.InsertIndex, state.SelectionBase);

        result.ResultState = (TextInputState) {
            .InsertIndex = imin(state.InsertIndex, state.SelectionBase),
            .SelectionBase = -1,
        };
    } else {
        result.DeleteMin = state.InsertIndex;
        result.DeleteMax = state.InsertIndex + 1;

        result.ResultState = (TextInputState) {
            .InsertIndex = state.InsertIndex,
            .SelectionBase = -1,
        };
    }

    result.ResultState.CursorRight = 0;
    result.ResultState.CursorIndex = result.ResultState.InsertIndex;

    return result;
}

TextEditResult TextState_InsertString(TextInputState state) {
    TextEditResult result = {0};

    int insertAt = state.InsertIndex;

    if (state.SelectionBase != -1) {
        result.DoDelete = 1;
        result.DeleteMin = imin(state.InsertIndex, state.SelectionBase);
        result.DeleteMax = imax(state.InsertIndex, state.SelectionBase);

        insertAt = result.DeleteMin;
    }

    result.DoInput = 1;
    result.ResultState = (TextInputState) {
        .InsertIndex = insertAt,
        .CursorIndex = state.CursorIndex, // these cursor properties don't really matter, but when something else decides to do its own thing with text input instead of just inserting text and manually bumping the cursor, we don't want to lose their original cursor state.
        .CursorRight = state.CursorRight,
        .SelectionBase = -1,
    };

    return result;
}

TextEditResult StandardTextInput(mu_Context* ctx, TextInputState textState, int maxIndex) {
    int doSelection = ctx->key_down & MU_KEY_SHIFT;

    TextEditResult result;

    if (ctx->key_pressed & MU_KEY_ARROWLEFT) {
        ctx->key_pressed &= ~MU_KEY_ARROWLEFT;
        result = (TextEditResult) { .ResultState = TextState_BumpCursor(textState, -1, doSelection) };
    } else if (ctx->key_pressed & MU_KEY_ARROWRIGHT) {
        ctx->key_pressed &= ~MU_KEY_ARROWRIGHT;
        result = (TextEditResult) { .ResultState = TextState_BumpCursor(textState, 1, doSelection) };
    } else if (ctx->key_pressed & MU_KEY_HOME) {
        ctx->key_pressed &= ~MU_KEY_HOME;
        result = (TextEditResult) { .ResultState = TextState_SetInsertIndex(textState, 0, doSelection) };
    } else if (ctx->key_pressed & MU_KEY_END) {
        ctx->key_pressed &= ~MU_KEY_END;
        result = (TextEditResult) {
            .ResultState = TextState_SetCursorRight(
                TextState_SetInsertIndex(textState, maxIndex, doSelection),
                1
            ),
        };
    } else if (ctx->key_pressed & MU_KEY_BACKSPACE) {
        ctx->key_pressed &= ~MU_KEY_BACKSPACE;
        result = TextState_DeleteBackwards(textState);
    } else if (ctx->key_pressed & MU_KEY_DELETE) {
        ctx->key_pressed &= ~MU_KEY_DELETE;
        result = TextState_DeleteForwards(textState);
    } else if (ctx->key_down & MU_KEY_CTRL && ctx->input_text[0] == 'a') {
        ctx->input_text[0] = 0;
        result = (TextEditResult) {
            .ResultState = TextState_SelectRange(0, maxIndex),
        };
    } else if (
        ctx->key_down & (MU_KEY_CTRL | MU_KEY_ALT)
        && ctx->input_text[0] != 0
    ) {
        result = (TextEditResult) {
            .DoInput = 1,
            .ResultState = textState,
        };
    } else if (ctx->input_text[0] != 0) {
        result = TextState_InsertString(textState);
    } else {
        result = (TextEditResult) { .ResultState = textState };
    }

    // Since this is standard text input, we can assume the bounds
    // are from 0 to max index and correct accordingly.
    result.ResultState = TextState_Clamp(result.ResultState, 0, maxIndex);

    return result;
}
