#pragma once

void Undo_Push(void* ptr, int size, const char* desc);
void Undo_Commit();
void Undo_Undo();
void Undo_Redo();

void Undo_Reset();

void PrintUndoData();
