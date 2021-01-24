#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define UNDO_DATA_SIZE 2048
#define UNDO_STACK_SIZE 2048 // TODO: Convert the undo stack to dynamic memory allocation. It sucks to have a hard cap.

void write(char* buf, char* s, int* i);
void sprintBytes(char* buf, void* p, size_t size, int maxEntries);
void printBytes(void* p, size_t size, int maxEntries);

typedef struct UndoItem {
    unsigned char Data[UNDO_DATA_SIZE];
    void* Pointer;
    size_t Size;
    const char* Desc;
} UndoItem;

typedef struct UndoItemStack {
    UndoItem Items[UNDO_STACK_SIZE];
    size_t Len;
} UndoItemStack;

UndoItemStack undoStack;
UndoItemStack redoStack;
UndoItemStack tempStack;

size_t UndoStackSize = 0;
size_t RedoStackSize = 0;
size_t TempStackSize = 0;

void push_item(UndoItemStack* stack, UndoItem item) {
    assert(stack->Len < UNDO_STACK_SIZE);
    stack->Items[stack->Len++] = item;
}

UndoItem pop_item(UndoItemStack* stack) {
    assert(stack->Len > 0);
    return stack->Items[--stack->Len];
}

void clear(UndoItemStack* stack) {
    stack->Len = 0;
}

UndoItem new_undo_item(void* ptr, size_t size, const char* desc) {
    assert(size <= UNDO_DATA_SIZE);

    UndoItem result = (UndoItem) {
        .Pointer = ptr,
        .Size = size,
        .Desc = desc,
    };
    memcpy(result.Data, ptr, size);

    return result;
}

void Undo_Push(void* ptr, size_t size, const char* desc) {
    assert(size <= UNDO_DATA_SIZE);

    for (size_t i = 0; i < tempStack.Len; i++) {
        if (tempStack.Items[i].Pointer == ptr) {
            return;
        }
    }

    push_item(&tempStack, new_undo_item(ptr, size, desc));
}

void Undo_Commit() {
    int didSeeChange = 0;

    for (int i = tempStack.Len-1; i >= 0; i--) {
        UndoItem item = pop_item(&tempStack);
        if (memcmp(item.Data, item.Pointer, item.Size)) {
            if (!didSeeChange) {
                didSeeChange = 1;

                push_item(&undoStack, (UndoItem){0});
                clear(&redoStack);
            }

            push_item(&undoStack, item);
        }
    }
}

void apply_stack(UndoItemStack* source, UndoItemStack* dest) {
    int didPushNull = 0;

    while (source->Len) {
        UndoItem item = pop_item(source);
        if (!item.Pointer) {
            // null item, stop undoing
            break;
        }

        // put item on redo stack
        if (!didPushNull) {
            push_item(dest, (UndoItem){0});
            didPushNull = 1;
        }
        push_item(dest, new_undo_item(item.Pointer, item.Size, item.Desc));

        // copy data to real memory
        memcpy(item.Pointer, item.Data, item.Size);
    }
}

void Undo_Undo() {
    apply_stack(&undoStack, &redoStack);
}

void Undo_Redo() {
    apply_stack(&redoStack, &undoStack);
}

void PrintUndoItem(UndoItem* item) {
    if (!item->Pointer) {
        printf("(UNDO ITEM) (null)");
    } else {
        const char* desc = item->Desc ? item->Desc : "";

        char storedBuf[128];
        char currentBuf[128];
        sprintBytes(storedBuf, item->Data, item->Size, 8);
        sprintBytes(currentBuf, item->Pointer, item->Size, 8);

        printf("(UNDO ITEM) [%s]\n  @%p, Size: %zu\n  Stored: %s\n  Current: %s", desc, item->Pointer, item->Size, storedBuf, currentBuf);
    }
}

void Undo_Reset() {
    clear(&undoStack);
    clear(&redoStack);
    clear(&tempStack);
}

void PrintUndoData() {
    printf("Undo stack size: %zu", undoStack.Len);
    for (int i = 0; i < undoStack.Len; i++) {
        PrintUndoItem(&undoStack.Items[i]);
    }
    printf("Redo stack size: %zu", redoStack.Len);
    for (int i = 0; i < redoStack.Len; i++) {
        PrintUndoItem(&redoStack.Items[i]);
    }
    printf("Temp stack size: %zu", tempStack.Len);
    for (int i = 0; i < tempStack.Len; i++) {
        PrintUndoItem(&redoStack.Items[i]);
    }
}

void write(char* buf, char* s, int* i) {
    int j = 0;
    char c = s[j];
    while (c) {
        buf[*i] = c;

        (*i)++;
        j++;
        c = s[j];
    }
}

void sprintBytes(char* buf, void* p, size_t size, int maxEntries) {
    int i = 0;
    write(buf, "[", &i);
    for (int j = 0; j < maxEntries + 1 && j < size; j++) {
        if (j > 0) {
            write(buf, ", ", &i);
        }
        if (j == maxEntries) {
            write(buf, "...", &i);
        } else {
            char byteBuf[16];
            sprintf(byteBuf, "0x%x", *((unsigned char*)p + j));
            write(buf, byteBuf, &i);
        }
    }
    write(buf, "]", &i);
    buf[i] = 0;
}

void printBytes(void* p, size_t size, int maxEntries) {
    char buf[128];
    sprintBytes(buf, p, size, maxEntries);
    printf("%s", buf);
}
