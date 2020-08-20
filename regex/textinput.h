#ifndef TEXTSELECTION_H
#define TEXTSELECTION_H

typedef struct TextInputState {
    int CursorPosition;
    int SelectionBase;
} TextInputState;

extern const TextInputState DEFAULT_TEXT_INPUT_STATE;

TextInputState setCursorPosition(TextInputState state, int i, int select);
TextInputState moveCursor(TextInputState state, int delta, int select);
TextInputState bumpCursor(TextInputState state, int direction, int select);

int isSelected(TextInputState state, int index);

typedef struct TextEditResult {
    int DoDelete;
    int DeleteMin;
    int DeleteMax;

    TextInputState ResultState;
} TextEditResult;

TextEditResult deleteBackwards(TextInputState state);
TextEditResult deleteForwards(TextInputState state);
TextEditResult insertString(TextInputState state);

#endif
