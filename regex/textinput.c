#include "textinput.h"

#include "../util/util.h"

const TextInputState DEFAULT_TEXT_INPUT_STATE = (TextInputState) {
    .CursorPosition = 0,
    .SelectionBase = -1,
};

TextInputState fixupState(TextInputState state) {
    if (state.CursorPosition == state.SelectionBase) {
        return (TextInputState) {
            .CursorPosition = state.CursorPosition,
            .SelectionBase = -1,
        };
    }

    return state;
}

TextInputState TextState_SetCursorPosition(TextInputState state, int i, int select) {
    TextInputState result;

    if (select) {
        result = (TextInputState) {
            .CursorPosition = i,
            .SelectionBase = (state.SelectionBase == -1 ? state.CursorPosition : state.SelectionBase),
        };
    } else {
        result = (TextInputState) {
            .CursorPosition = i,
            .SelectionBase = -1,
        };
    }

    return fixupState(result);
}

TextInputState TextState_MoveCursor(TextInputState state, int delta, int select) {
    return TextState_SetCursorPosition(state, state.CursorPosition + delta, select);
}

TextInputState TextState_BumpCursor(TextInputState state, int direction, int select) {
    if (!select && state.SelectionBase != -1) {
        int destination = (direction > 0
            ? imax(state.CursorPosition, state.SelectionBase)
            : imin(state.CursorPosition, state.SelectionBase)
        );
        return TextState_SetCursorPosition(state, destination, 0);
    }

    return TextState_MoveCursor(state, (direction > 0 ? 1 : -1), select);
}

int TextState_IsSelecting(TextInputState state) {
    return state.SelectionBase != -1;
}

int TextState_SelectionStart(TextInputState state) {
    return imin(state.CursorPosition, state.SelectionBase);
}

int TextState_SelectionEnd(TextInputState state) {
    return imax(state.CursorPosition, state.SelectionBase) - 1;
}

int TextState_IsSelected(TextInputState state, int index) {
    if (state.SelectionBase == -1) {
        return 0;
    }

    int min = imin(state.CursorPosition, state.SelectionBase);
    int max = imax(state.CursorPosition, state.SelectionBase);
    return min <= index && index < max;
}

TextEditResult TextState_DeleteBackwards(TextInputState state) {
    TextEditResult result = {0};

    result.DoDelete = 1;

    if (state.SelectionBase != -1) {
        result.DeleteMin = imin(state.CursorPosition, state.SelectionBase);
        result.DeleteMax = imax(state.CursorPosition, state.SelectionBase);

        result.ResultState = (TextInputState) {
            .CursorPosition = imin(state.CursorPosition, state.SelectionBase),
            .SelectionBase = -1,
        };
    } else {
        result.DeleteMin = state.CursorPosition - 1;
        result.DeleteMax = state.CursorPosition;

        result.ResultState = (TextInputState) {
            .CursorPosition = state.CursorPosition - 1,
            .SelectionBase = -1,
        };
    }

    return result;
}

TextEditResult TextState_DeleteForwards(TextInputState state) {
    TextEditResult result = {0};

    result.DoDelete = 1;

    if (state.SelectionBase != -1) {
        result.DeleteMin = imin(state.CursorPosition, state.SelectionBase);
        result.DeleteMax = imax(state.CursorPosition, state.SelectionBase);

        result.ResultState = (TextInputState) {
            .CursorPosition = imin(state.CursorPosition, state.SelectionBase),
            .SelectionBase = -1,
        };
    } else {
        result.DeleteMin = state.CursorPosition;
        result.DeleteMax = state.CursorPosition + 1;

        result.ResultState = (TextInputState) {
            .CursorPosition = state.CursorPosition,
            .SelectionBase = -1,
        };
    }

    return result;
}

TextEditResult TextState_InsertString(TextInputState state) {
    TextEditResult result = {0};

    int insertAt = state.CursorPosition;

    if (state.SelectionBase != -1) {
        result.DoDelete = 1;
        result.DeleteMin = imin(state.CursorPosition, state.SelectionBase);
        result.DeleteMax = imax(state.CursorPosition, state.SelectionBase);

        insertAt = result.DeleteMin;
    }

    result.ResultState = (TextInputState) {
        .CursorPosition = insertAt,
        .SelectionBase = -1,
    };

    return result;
}
