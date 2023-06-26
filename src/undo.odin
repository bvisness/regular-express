package main

import "core:fmt"
import "core:intrinsics"

undo_stack: [dynamic]^UndoEntry
redo_stack: [dynamic]^UndoEntry

UndoProc :: proc(entry: ^UndoEntry)

UndoEntry :: struct {
    undo: UndoProc
    redo: UndoProc
    destroy: UndoProc
}

done :: proc(entry: ^UndoEntry) {
    append(&undo_stack, entry)

    for redo_entry in redo_stack {
        if redo_entry.destroy != nil {
            redo_entry->destroy()
        }
    }
    clear(&redo_stack)
}

UndoSingleValueEntry :: struct($T: typeid) {
    using entry: UndoEntry
    data_ref: ^T
    data: T
}

// Pointers to pointers are forbidden because footgun.
single_value :: proc(data: ^$T) -> ^UndoSingleValueEntry(T) where !intrinsics.type_is_pointer(T) {
    return new_clone(UndoSingleValueEntry(T) {
        data_ref = data,
        data = data^,

        undo = proc(entry: ^UndoEntry) {
            entry := (^UndoSingleValueEntry(T))(entry)
            swap(entry.data_ref, &entry.data)
        },
        redo = proc(entry: ^UndoEntry) {
            entry := (^UndoSingleValueEntry(T))(entry)
            swap(entry.data_ref, &entry.data)
        },
    })
}

// TODO: someday this could be very cool and do an xor swap.
swap :: proc(a, b: ^$T) {
    tmp := a^
    a^ = b^
    b^ = tmp
}

undo :: proc() -> bool {
    if len(undo_stack) == 0 {
        return false
    }

    entry := pop(&undo_stack)
    entry->undo()
    append(&redo_stack, entry)

    return true
}

redo :: proc() -> bool {
    if len(redo_stack) == 0 {
        return false
    }

    entry := pop(&redo_stack)
    entry->redo()
    append(&undo_stack, entry)

    return true
}
