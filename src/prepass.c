#include "prepass.h"

void prepass_Regex(Regex* regex, NoUnionEx* parentEx, Unit* parentUnit) {
    assert(regex != NULL);

    int w = 0;
    int h = 0;
    int wireHeight = 0;

    // delete any empty expressions (after the first one) (that are not focused)
    for (int i = regex->NumUnionMembers-1; i >= 0; i--) {
        NoUnionEx* member = regex->UnionMembers[i];
        if (
            member->NumUnits == 0
            && regex->NumUnionMembers > 1
            && ctx->focus != mu_get_id_noidstack(ctx, &member, sizeof(NoUnionEx*))
        ) {
            NoUnionEx* deleted = Regex_RemoveUnionMember(regex, i);
            NoUnionEx_delete(deleted);
        }
    }

    // set indexes on all the expressions
    for (int i = 0; i < regex->NumUnionMembers; i++) {
        regex->UnionMembers[i]->Index = i;
    }

    for (int i = 0; i < regex->NumUnionMembers; i++) {
        NoUnionEx* member = regex->UnionMembers[i];
        prepass_NoUnionEx(member, regex, parentEx, parentUnit);

        w = imax(w, member->Size.w);
        h += (i != 0 ? UNION_VERTICAL_SPACING : 0) + member->Size.h;

        if (i == 0) {
            wireHeight = member->WireHeight;
        }
    }

    regex->UnionSize = (Vec2i) {
        .w = w,
        .h = h,
    };
    regex->Size = (Vec2i) {
        .w = UNION_GUTTER_WIDTH + regex->UnionSize.w + UNION_GUTTER_WIDTH,
        .h = regex->UnionSize.h + 40,
    };
    regex->WireHeight = wireHeight;
}

void prepass_NoUnionEx(NoUnionEx* ex, Regex* regex, NoUnionEx* parentEx, Unit* parentUnit) {
    mu_Id muid = NoUnionEx_GetID(ex);

    if (ctx->focus == muid) {
        ctx->updated_focus = 1;

        int inputTextLength = strlen(ctx->input_text);

        Unit* previousUnit = NULL;
        if (ex->TextState.InsertIndex > 0) {
            previousUnit = ex->Units[ex->TextState.InsertIndex - 1];
        }

        UnitRange selectedUnits = {0};
        if (TextState_IsSelecting(ex->TextState)) {
            selectedUnits = (UnitRange) {
                .Ex = ex,
                .Start = TextState_SelectionStart(ex->TextState),
                .End = TextState_SelectionEnd(ex->TextState),
            };
        }

        if (ctx->key_pressed & MU_KEY_BACKSPACE
                && !TextState_IsSelecting(ex->TextState)
                && previousUnit
                && previousUnit->Contents.Type == RE_CONTENTS_METACHAR
        ) {
            // break the previous metachar
            ctx->key_pressed &= ~MU_KEY_BACKSPACE;

            char c = previousUnit->Contents.MetaChar.C;
            int insertUnitsAt = ex->TextState.InsertIndex - 1;

            Unit* deletedUnit = NoUnionEx_RemoveUnit(ex, previousUnit->Index);
            Unit_delete(deletedUnit);

            Unit* backslash = Unit_initWithLiteralChar(RE_NEW(Unit), '\\');
            NoUnionEx_AddUnit(ex, backslash, insertUnitsAt);

            Unit* character = Unit_initWithLiteralChar(RE_NEW(Unit), c);
            NoUnionEx_AddUnit(ex, character, insertUnitsAt + 1);

            ex->TextState = TextState_SetCursorRight(
                TextState_SetInsertIndex(ex->TextState, insertUnitsAt + 2, 0),
                1
            );
        } else if (ctx->key_pressed & MU_KEY_BACKSPACE
                && !TextState_IsSelecting(ex->TextState)
                && ex->Index > 0
                && ex->TextState.InsertIndex == 0
        ) {
            // backspace at beginning of expression; collapse into previous
            ctx->key_pressed &= ~MU_KEY_BACKSPACE;
            NoUnionEx* previousEx = regex->UnionMembers[ex->Index - 1];
            int index = previousEx->NumUnits;

            MoveUnitsTo(
                (UnitRange) {
                    .Ex = ex,
                    .Start = 0,
                    .End = ex->NumUnits - 1,
                },
                previousEx,
                index
            );
            mu_set_focus(ctx, NoUnionEx_GetID(previousEx));
            previousEx->TextState = TextState_SetInsertIndex(previousEx->TextState, index, 0);
        } else if (ctx->key_pressed & MU_KEY_DELETE
                && !TextState_IsSelecting(ex->TextState)
                && ex->Index < (regex->NumUnionMembers - 1)
                && ex->TextState.InsertIndex == ex->NumUnits
        ) {
            // delete at end of expression; collapse next into current
            ctx->key_pressed &= ~MU_KEY_DELETE;
            NoUnionEx* nextEx = regex->UnionMembers[ex->Index + 1];
            MoveUnitsTo(
                (UnitRange) {
                    .Ex = nextEx,
                    .Start = 0,
                    .End = nextEx->NumUnits - 1,
                },
                ex,
                ex->NumUnits
            );
        } else if (ctx->key_pressed & MU_KEY_ARROWRIGHT
                && !TextState_IsSelecting(ex->TextState)
                && ex->Index < (regex->NumUnionMembers - 1)
                && ex->TextState.InsertIndex == ex->NumUnits
        ) {
            ctx->key_pressed &= ~MU_KEY_ARROWRIGHT;
            NoUnionEx* nextEx = regex->UnionMembers[ex->Index + 1];
            mu_set_focus(ctx, NoUnionEx_GetID(nextEx));
            nextEx->TextState = TextState_SetCursorIndex(0, 0);
        } else if (ctx->key_pressed & MU_KEY_ARROWLEFT
                && !TextState_IsSelecting(ex->TextState)
                && ex->Index > 0
                && ex->TextState.InsertIndex == 0
        ) {
            ctx->key_pressed &= ~MU_KEY_ARROWLEFT;
            NoUnionEx* previousEx = regex->UnionMembers[ex->Index - 1];
            mu_set_focus(ctx, NoUnionEx_GetID(previousEx));
            previousEx->TextState = TextState_SetCursorIndex(previousEx->NumUnits - 1, 1);
        } else if (ctx->key_down & MU_KEY_SHIFT && ctx->key_pressed & MU_KEY_TAB) {
            // shift-tab
            ctx->key_pressed &= ~MU_KEY_TAB;
            if (ex->Index > 0) {
                NoUnionEx* previousEx = regex->UnionMembers[ex->Index - 1];
                mu_set_focus(ctx, NoUnionEx_GetID(previousEx));
                previousEx->TextState = TextState_SetCursorIndex(0, 0);
            }
        } else if (ctx->key_pressed & MU_KEY_TAB) {
            // tab
            ctx->key_pressed &= ~MU_KEY_TAB;
            if (ex->Index < regex->NumUnionMembers - 1) {
                NoUnionEx* nextEx = regex->UnionMembers[ex->Index + 1];
                mu_set_focus(ctx, NoUnionEx_GetID(nextEx));
                nextEx->TextState = TextState_SetCursorIndex(0, 0);
            }
        } else if (ctx->key_down & MU_KEY_ALT && ctx->key_pressed & MU_KEY_ARROWDOWN) {
            ctx->key_pressed &= ~MU_KEY_ARROWDOWN;
            if (ex->Index < regex->NumUnionMembers - 1) {
                NoUnionEx* nextEx = regex->UnionMembers[ex->Index + 1];
                mu_set_focus(ctx, NoUnionEx_GetID(nextEx));
                nextEx->TextState = ex->TextState;
                nextEx->TextState.SelectionBase = -1;
            }
        } else if (ctx->key_down & MU_KEY_ALT && ctx->key_pressed & MU_KEY_ARROWUP) {
            ctx->key_pressed &= ~MU_KEY_ARROWUP;
            if (ex->Index > 0) {
                NoUnionEx* previousEx = regex->UnionMembers[ex->Index - 1];
                mu_set_focus(ctx, NoUnionEx_GetID(previousEx));
                previousEx->TextState = ex->TextState;
                previousEx->TextState.SelectionBase = -1;
            }
        } else if (ctx->key_pressed & MU_KEY_ARROWDOWN) {
            // Jump down into the group the cursor is on
            Unit* cursorUnit = ex->Units[ex->TextState.CursorIndex];
            if (cursorUnit->Contents.Type == RE_CONTENTS_GROUP && !TextState_IsSelecting(ex->TextState)) {
                ctx->key_pressed &= ~MU_KEY_ARROWDOWN;
                NoUnionEx* destEx = cursorUnit->Contents.Group->Regex->UnionMembers[0]; // TODO: Is it possible that there could be no union members here?
                mu_set_focus(ctx, NoUnionEx_GetID(destEx));

                if (ex->TextState.CursorRight) {
                    destEx->TextState = TextState_SetInsertIndex(destEx->TextState, destEx->NumUnits, 0);
                } else {
                    destEx->TextState = TextState_SetInsertIndex(destEx->TextState, 0, 0);
                }
            } else if (cursorUnit->Contents.Type == RE_CONTENTS_SET && !TextState_IsSelecting(ex->TextState)) {
                ctx->key_pressed &= ~MU_KEY_ARROWDOWN;
                Set* destSet = cursorUnit->Contents.Set;
                mu_set_focus(ctx, Set_GetID(destSet));

                if (ex->TextState.CursorRight) {
                    destSet->TextState = TextState_SetInsertIndex(destSet->TextState, destSet->NumItems, 0);
                } else {
                    destSet->TextState = TextState_SetInsertIndex(destSet->TextState, 0, 0);
                }
            }
        } else if (ctx->key_pressed & MU_KEY_ARROWUP) {
            // Jump up out of the current group
            if (parentUnit) {
                ctx->key_pressed &= ~MU_KEY_ARROWUP;
                mu_set_focus(ctx, NoUnionEx_GetID(parentEx));
                int doCursorRight = ex->TextState.CursorIndex > (ex->NumUnits / 2);
                parentEx->TextState = TextState_SetCursorIndex(parentUnit->Index, doCursorRight);
            }
        } if (ctx->key_pressed & MU_KEY_RETURN) {
            // enter
            ctx->key_pressed &= ~MU_KEY_RETURN;

            if (TextState_IsSelecting(ex->TextState)) {
                // make a group out of the current selection and add a union member
                Unit* newUnit = ConvertRangeToGroup(selectedUnits);
                NoUnionEx* newEx = NoUnionEx_init(RE_NEW(NoUnionEx));
                Regex_AddUnionMember(newUnit->Contents.Group->Regex, newEx, -1);
                mu_set_focus(ctx, NoUnionEx_GetID(newEx));
            } else {
                // split the current expression at the cursor
                NoUnionEx* newEx = NoUnionEx_init(RE_NEW(NoUnionEx));
                MoveUnitsTo(
                    (UnitRange) {
                        .Ex = ex,
                        .Start = ex->TextState.InsertIndex,
                        .End = ex->NumUnits - 1,
                    },
                    newEx,
                    0
                );
                Regex_AddUnionMember(regex, newEx, ex->Index + 1);

                mu_set_focus(ctx, NoUnionEx_GetID(newEx));
            }
        } else if (ctx->key_down & MU_KEY_ALT && inputTextLength == 1) {
            // Alt-Whatever shortcut
            Unit* newUnit = NULL;
            if (
                ctx->input_text[0] == '['
                || strcmp(ctx->input_keycode, "BracketLeft") == 0
            ) {
                if (selectedUnits.Ex) {
                    DeleteRange(selectedUnits);
                }
                newUnit = Unit_init(RE_NEW(Unit));
                UnitContents_SetType(&newUnit->Contents, RE_CONTENTS_SET);
                mu_set_focus(ctx, mu_get_id_noidstack(ctx, &newUnit->Contents.Set, sizeof(Set*)));
            } else if (
                ctx->input_text[0] == '('
                || (ctx->key_down & MU_KEY_SHIFT && strcmp(ctx->input_keycode, "Digit9") == 0)
            ) {
                // open paren (create group)
                if (TextState_IsSelecting(ex->TextState)) {
                    ConvertRangeToGroup(selectedUnits);
                    ex->TextState = TextState_SetInsertIndex(ex->TextState, selectedUnits.Start + 1, 0);
                    ex->TextState = TextState_SetCursorRight(ex->TextState, 1);
                } else {
                    newUnit = Unit_init(RE_NEW(Unit));
                    UnitContents_SetType(&newUnit->Contents, RE_CONTENTS_GROUP);
                    mu_set_focus(ctx, mu_get_id_noidstack(ctx, &newUnit->Contents.Group->Regex->UnionMembers[0], sizeof(NoUnionEx*)));
                }
            } else if (
                ctx->input_text[0] == ')'
                || (ctx->key_down & MU_KEY_SHIFT && strcmp(ctx->input_keycode, "Digit0") == 0)
            ) {
                // close paren (leave group)
                if (parentEx) {
                    mu_set_focus(ctx, mu_get_id_noidstack(ctx, &parentEx, sizeof(NoUnionEx*)));
                    parentEx->TextState = TextState_SetCursorIndex(parentUnit->Index, 1);
                }
            } else if (
                ctx->input_text[0] == '.'
                || strcmp(ctx->input_keycode, "Period") == 0
            ) {
                newUnit = Unit_init(RE_NEW(Unit));
                UnitContents_SetType(&newUnit->Contents, RE_CONTENTS_SPECIAL);
                newUnit->Contents.Special.Type = RE_SPECIAL_ANY;
            } else if (
                ctx->input_text[0] == '^'
                || (ctx->key_down & MU_KEY_SHIFT && strcmp(ctx->input_keycode, "Digit6") == 0)
            ) {
                newUnit = Unit_init(RE_NEW(Unit));
                UnitContents_SetType(&newUnit->Contents, RE_CONTENTS_SPECIAL);
                newUnit->Contents.Special.Type = RE_SPECIAL_STRINGSTART;
            } else if (
                ctx->input_text[0] == '$'
                || (ctx->key_down & MU_KEY_SHIFT && strcmp(ctx->input_keycode, "Digit4") == 0)
            ) {
                newUnit = Unit_init(RE_NEW(Unit));
                UnitContents_SetType(&newUnit->Contents, RE_CONTENTS_SPECIAL);
                newUnit->Contents.Special.Type = RE_SPECIAL_STRINGEND;
            } else if (
                (
                    ctx->input_text[0] == '?'
                    || (ctx->key_down & MU_KEY_SHIFT && strcmp(ctx->input_keycode, "Slash") == 0)
                )
                || (
                    ctx->input_text[0] == '+'
                    || (ctx->key_down & MU_KEY_SHIFT && strcmp(ctx->input_keycode, "Equal") == 0)
                )
                || (
                    ctx->input_text[0] == '*'
                    || (ctx->key_down & MU_KEY_SHIFT && strcmp(ctx->input_keycode, "Digit8") == 0)
                )
            ) {
                Unit* repeatUnit = previousUnit;
                if (selectedUnits.Ex) {
                    if (selectedUnits.End - selectedUnits.Start > 0) {
                        ConvertRangeToGroup(selectedUnits);
                        ex->TextState = TextState_SetInsertIndex(ex->TextState, selectedUnits.Start + 1, 0);
                        ex->TextState = TextState_SetCursorRight(ex->TextState, 1);
                    }
                    repeatUnit = ex->Units[selectedUnits.Start];
                }

                // Normalize all this jank keyboard shortcut stuff
                char modifier = ctx->input_text[0];
                if (ctx->key_down & MU_KEY_SHIFT && strcmp(ctx->input_keycode, "Slash") == 0) {
                    modifier = '?';
                }
                if (ctx->key_down & MU_KEY_SHIFT && strcmp(ctx->input_keycode, "Equal") == 0) {
                    modifier = '+';
                }
                if (ctx->key_down & MU_KEY_SHIFT && strcmp(ctx->input_keycode, "Digit8") == 0) {
                    modifier = '*';
                }

                switch (modifier) {
                case '?': {
                    if (repeatUnit->RepeatMin == 0 && repeatUnit->RepeatMax == 1) {
                        Unit_SetRepeatMin(repeatUnit, 1);
                        Unit_SetRepeatMax(repeatUnit, 1);
                    } else {
                        Unit_SetRepeatMin(repeatUnit, 0);
                        Unit_SetRepeatMax(repeatUnit, 1);
                    }
                } break;
                case '+': {
                    if (repeatUnit->RepeatMin == 1 && repeatUnit->RepeatMax == 0) {
                        Unit_SetRepeatMin(repeatUnit, 1);
                        Unit_SetRepeatMax(repeatUnit, 1);
                    } else {
                        Unit_SetRepeatMin(repeatUnit, 1);
                        Unit_SetRepeatMax(repeatUnit, 0);
                    }
                } break;
                case '*': {
                    if (repeatUnit->RepeatMin == 0 && repeatUnit->RepeatMax == 0) {
                        Unit_SetRepeatMin(repeatUnit, 1);
                        Unit_SetRepeatMax(repeatUnit, 1);
                    } else {
                        Unit_SetRepeatMin(repeatUnit, 0);
                        Unit_SetRepeatMax(repeatUnit, 0);
                    }
                } break;
                }

                ex->TextState = TextState_SetCursorRight(ex->TextState, 1);
            } else if (
                ctx->input_text[0] == '|'
                || (ctx->key_down & MU_KEY_SHIFT && strcmp(ctx->input_keycode, "Backslash") == 0)
            ) {
                // pipe
                NoUnionEx* newEx = NoUnionEx_init(RE_NEW(NoUnionEx));
                MoveUnitsTo(
                    (UnitRange) {
                        .Ex = ex,
                        .Start = ex->TextState.InsertIndex,
                        .End = ex->NumUnits - 1,
                    },
                    newEx,
                    0
                );
                Regex_AddUnionMember(regex, newEx, ex->Index + 1);

                mu_set_focus(ctx, NoUnionEx_GetID(newEx));
            }

            if (newUnit) {
                if (TextState_IsSelecting(ex->TextState)) {
                    TextEditResult result = TextState_DeleteForwards(ex->TextState);
                    NoUnionEx_RemoveUnits(ex, result.DeleteMin, result.DeleteMax);
                    ex->TextState = result.ResultState;
                }

                NoUnionEx_AddUnit(ex, newUnit, ex->TextState.InsertIndex);
                ex->TextState = TextState_MoveCursor(ex->TextState, 1, 0);
            }

            ctx->input_text[0] = 0;
        } else if (inputTextLength > 1) {
            // assume we are pasting and want to parse a regex
            // TODO: We should probably explicitly detect that we are pasting.

            NoUnionEx_RemoveSelection(ex);

            Regex* parseResult = parse(ctx->input_text);
            ctx->input_text[0] = 0;

            if (parseResult->NumUnionMembers == 1) {
                // can insert all units inline
                NoUnionEx* src = parseResult->UnionMembers[0];
                while (src->NumUnits > 0) {
                    Unit* unit = NoUnionEx_RemoveUnit(src, 0);
                    NoUnionEx_AddUnit(ex, unit, ex->TextState.InsertIndex);
                    ex->TextState = TextState_BumpCursor(ex->TextState, 1, 0);
                }

                assert(parseResult->UnionMembers[0]->NumUnits == 0);
                Regex_delete(parseResult);
            } else {
                // must create a group
                Unit* newUnit = Unit_init(RE_NEW(Unit));
                UnitContents_SetType(&newUnit->Contents, RE_CONTENTS_GROUP);
                newUnit->Contents.Group->Regex = parseResult;

                NoUnionEx_AddUnit(ex, newUnit, ex->TextState.InsertIndex);
                ex->TextState = TextState_BumpCursor(ex->TextState, 1, 0);
            }
        } else {
            // type a single key

            TextEditResult result = StandardTextInput(ctx, ex->TextState, ex->NumUnits);

            if (result.DoDelete) {
                NoUnionEx_RemoveUnits(ex, result.DeleteMin, result.DeleteMax);
            }

            ex->TextState = result.ResultState;

            if (result.DoInput && inputTextLength == 1) {
                Unit* newUnit = NULL;
                if (previousUnit
                        && previousUnit->Contents.Type == RE_CONTENTS_LITCHAR
                        && previousUnit->Contents.LitChar.C == '\\'
                        && strchr(LEGAL_METACHARS, ctx->input_text[0])
                ) {
                    // previous unit is a slash, do a metachar
                    Unit* deletedUnit = NoUnionEx_RemoveUnit(ex, previousUnit->Index);
                    Unit_delete(deletedUnit);
                    ex->TextState = TextState_SetCursorRight(
                        TextState_SetInsertIndex(ex->TextState, ex->TextState.InsertIndex - 1, 0),
                        1
                    );

                    newUnit = Unit_init(RE_NEW(Unit));
                    UnitContents_SetType(&newUnit->Contents, RE_CONTENTS_METACHAR);
                    newUnit->Contents.MetaChar.C = ctx->input_text[0];
                } else {
                    newUnit = Unit_initWithLiteralChar(RE_NEW(Unit), ctx->input_text[0]);
                }

                if (newUnit) {
                    NoUnionEx_AddUnit(ex, newUnit, ex->TextState.InsertIndex);
                    ex->TextState = TextState_MoveCursor(ex->TextState, 1, 0);
                }

                ctx->input_text[0] = 0;
            }
        }
    }

    // fix up cursor position
    ex->TextState = TextState_Clamp(ex->TextState, 0, ex->NumUnits);

    // calculate sizes
    int w = 0;
    int h = NOUNIONEX_MIN_HEIGHT;
    int wireHeight = NOUNIONEX_MIN_HEIGHT/2;

    for (int i = 0; i < ex->NumUnits; i++) {
        Unit* unit = ex->Units[i];
        prepass_Unit(unit, ex);

        w += unit->Size.w;
        h = imax(h, unit->Size.h);
        wireHeight = imax(wireHeight, unit->WireHeight);

        unit->Parent = ex;

        if (unit->Index != i) {
            printf("WARNING! Unit %p had index %d, expected %d.", unit, unit->Index, i);
        }
        unit->Index = i;
    }

    ex->Size = (Vec2i) { .w = w, .h = h };
    ex->WireHeight = wireHeight;
}

void prepass_Unit(Unit* unit, NoUnionEx* ex) {
    UnitContents* contents = &unit->Contents;
    prepass_UnitContents(contents, ex, unit);

    int attachmentWidth = Unit_IsNonSingular(unit) ? UNIT_WIRE_ATTACHMENT_ZONE_WIDTH : 0;

    unit->Size = (Vec2i) {
        .w = unit->LeftHandleZoneWidth
            + attachmentWidth
            + contents->Size.w
            + attachmentWidth
            + unit->RightHandleZoneWidth,
        .h = UNIT_REPEAT_WIRE_ZONE_HEIGHT + contents->Size.h + UNIT_REPEAT_WIRE_ZONE_HEIGHT,
    };
    unit->WireHeight = UNIT_REPEAT_WIRE_ZONE_HEIGHT + contents->WireHeight;
}

void prepass_UnitContents(UnitContents* contents, NoUnionEx* ex, Unit* unit) {
    switch (contents->Type) {
        case RE_CONTENTS_LITCHAR: {
            contents->Size = (Vec2i) { .w = UNIT_CONTENTS_LITCHAR_WIDTH, .h = UNIT_CONTENTS_MIN_HEIGHT };
            contents->WireHeight = UNIT_CONTENTS_MIN_HEIGHT/2;
        } break;
        case RE_CONTENTS_METACHAR: {
            char* str = MetaChar_GetHumanString(&contents->MetaChar);
            int width = measureText(str, strlen(str));

            contents->Size = (Vec2i) { .w = width + 20, .h = UNIT_CONTENTS_MIN_HEIGHT };
            contents->WireHeight = UNIT_CONTENTS_MIN_HEIGHT/2;
        } break;
        case RE_CONTENTS_SPECIAL: {
            const char* str = Special_GetHumanString(&contents->Special);
            int width = measureText(str, strlen(str));

            contents->Size = (Vec2i) { .w = width + 20, .h = UNIT_CONTENTS_MIN_HEIGHT };
            contents->WireHeight = UNIT_CONTENTS_MIN_HEIGHT/2;
        } break;
        case RE_CONTENTS_SET: {
            Set* set = contents->Set;

            prepass_Set(set, ex, unit);

            // calculate sizes
            Vec2i contentsSize = {
                .w = 0,
                .h = UNIT_CONTENTS_MIN_HEIGHT,
            };

            for (int i = 0; i < set->NumItems; i++) {
                SetItem* item = set->Items[i];

                if (item->Type == RE_SETITEM_LITCHAR) {
                    item->Size = (Vec2i) { .w = UNIT_CONTENTS_LITCHAR_WIDTH, .h = UNIT_CONTENTS_MIN_HEIGHT };
                } else if (item->Type == RE_SETITEM_METACHAR) {
                    item->Size = (Vec2i) { .w = 25, .h = UNIT_CONTENTS_MIN_HEIGHT };
                } else if (item->Type == RE_SETITEM_RANGE) {
                    item->Size = (Vec2i) {
                        .w = UNIT_CONTENTS_LITCHAR_WIDTH + SET_DASH_WIDTH + UNIT_CONTENTS_LITCHAR_WIDTH,
                        .h = UNIT_CONTENTS_MIN_HEIGHT,
                    };
                }

                contentsSize.w += (i > 0 ? SET_HORIZONTAL_SPACING : 0) + item->Size.w;
                contentsSize.h = imax(contentsSize.h, item->Size.h);
            }

            const char* oneofStr = set->IsNegative ? SET_ONEOF_TEXT_NEG : SET_ONEOF_TEXT;
            int oneofWidth = SET_PADDING*2 + measureText(oneofStr, strlen(oneofStr)) + SET_PADDING*2;

            contents->Size = (Vec2i) {
                .w = SET_PADDING + imax(contentsSize.w, oneofWidth) + SET_PADDING,
                .h = SET_PADDING + SET_ONEOF_HEIGHT + SET_PADDING + contentsSize.h + SET_PADDING,
            };
            contents->WireHeight = SET_PADDING + SET_ONEOF_HEIGHT + SET_PADDING + contentsSize.h/2;
        } break;
        case RE_CONTENTS_GROUP: {
            Group* group = contents->Group;
            prepass_Group(group, ex, unit);
            contents->Size = group->Size;
            contents->WireHeight = group->WireHeight;
        } break;
        case RE_CONTENTS_UNKNOWN: {
            int width = measureText(contents->Unknown.Str, strlen(contents->Unknown.Str));
            contents->Size = (Vec2i) { .w = width + 20, .h = UNIT_CONTENTS_MIN_HEIGHT };
            contents->WireHeight = UNIT_CONTENTS_MIN_HEIGHT/2;
        } break;
    }
}

void prepass_Set(Set* set, NoUnionEx* ex, Unit* unit) {
    mu_Id muid = mu_get_id_noidstack(ctx, &set, sizeof(Set*));

    if (ctx->focus == muid) {
        // Handle keyboard input to set up for next frame
        TextEditResult result = {0};
        int inputTextLength = strlen(ctx->input_text);
        SetItem* itemBeforeCursor = NULL;
        if (set->TextState.CursorRight) {
            itemBeforeCursor = set->Items[set->TextState.CursorIndex];
        } else if (set->TextState.CursorIndex > 0) {
            itemBeforeCursor = set->Items[set->TextState.CursorIndex - 1];
        }

        if (ctx->key_pressed & MU_KEY_BACKSPACE
                && itemBeforeCursor
                && itemBeforeCursor->Type == RE_SETITEM_RANGE
                && itemBeforeCursor->Range.Max.C != 0
        ) {
            // clear the end of a range
            ctx->key_pressed &= ~MU_KEY_BACKSPACE;
            itemBeforeCursor->Range.Max.C = 0;
        } else if (ctx->key_pressed & MU_KEY_BACKSPACE
                && itemBeforeCursor
                && itemBeforeCursor->Type == RE_SETITEM_RANGE
                && itemBeforeCursor->Range.Max.C == 0
        ) {
            // break an incomplete range into a character and a dash
            ctx->key_pressed &= ~MU_KEY_BACKSPACE;
            SetItem_SetType(itemBeforeCursor, RE_SETITEM_LITCHAR);
            itemBeforeCursor->LitChar.C = itemBeforeCursor->Range.Min.C;

            SetItem* newItem = SetItem_init(RE_NEW(SetItem));
            newItem->LitChar.C = '-';
            Set_AddItem(set, newItem, set->TextState.InsertIndex);
            set->TextState = TextState_BumpCursor(set->TextState, 1, 0);
        } else if (ctx->key_pressed & MU_KEY_BACKSPACE
                && itemBeforeCursor
                && itemBeforeCursor->Type == RE_SETITEM_METACHAR
        ) {
            // break the previous metachar
            ctx->key_pressed &= ~MU_KEY_BACKSPACE;
            SetItem_SetType(itemBeforeCursor, RE_SETITEM_LITCHAR);
            itemBeforeCursor->LitChar.C = '\\';

            SetItem* newItem = SetItem_init(RE_NEW(SetItem));
            newItem->LitChar.C = itemBeforeCursor->MetaChar.C;
            Set_AddItem(set, newItem, set->TextState.InsertIndex);
            set->TextState = TextState_BumpCursor(set->TextState, 1, 0);
        } else if (ctx->key_pressed & MU_KEY_ARROWUP) {
            // Jump up out of the current set
            ctx->key_pressed &= ~MU_KEY_ARROWUP;
            mu_set_focus(ctx, NoUnionEx_GetID(ex));
            int doCursorRight = set->TextState.CursorIndex > (set->NumItems / 2);
            ex->TextState = TextState_SetCursorIndex(unit->Index, doCursorRight);
        } else if (
            ctx->key_down & MU_KEY_ALT
            && (
                (ctx->input_text[0] && ctx->input_text[0] == ']')
                || strcmp(ctx->input_keycode, "BracketRight") == 0
            )
        ) {
            // Shortcut to exit the set
            mu_set_focus(ctx, NoUnionEx_GetID(ex));
            ex->TextState = TextState_SetCursorIndex(unit->Index, 1);
        } else if (
            ctx->key_down & MU_KEY_ALT
            && (
                (ctx->input_text[0] && ctx->input_text[0] == '^')
                || (ctx->key_down & MU_KEY_SHIFT && strcmp(ctx->input_keycode, "Digit6") == 0)
            )
        ) {
            // Shortcut to toggle negated-ness
            set->IsNegative = !set->IsNegative;
        } else {
            result = StandardTextInput(ctx, set->TextState, set->NumItems);
            set->TextState = result.ResultState;
        }

        if (result.DoDelete) {
            result.DeleteMin = iclamp(result.DeleteMin, 0, set->NumItems);
            result.DeleteMax = iclamp(result.DeleteMax, 0, set->NumItems);

            for (int i = 0; i < result.DeleteMax - result.DeleteMin; i++) {
                Set_RemoveItem(set, result.DeleteMin);
            }
        }

        if (result.DoInput) {
            for (int i = 0; i < inputTextLength; i++) {
                do {
                    if (inputTextLength == 1) {
                        if (
                            ctx->input_text[i] == '-'
                            && itemBeforeCursor
                            && itemBeforeCursor->Type != RE_SETITEM_RANGE
                        ) {
                            SetItem_SetType(itemBeforeCursor, RE_SETITEM_RANGE);
                            itemBeforeCursor->Range.Min.C = itemBeforeCursor->LitChar.C;
                            break;
                        }

                        if (
                            itemBeforeCursor
                            && itemBeforeCursor->Type == RE_SETITEM_RANGE
                            && itemBeforeCursor->Range.Max.C == 0
                        ) {
                            itemBeforeCursor->Range.Max.C = ctx->input_text[i];
                            break;
                        }

                        if (
                            itemBeforeCursor
                            && itemBeforeCursor->Type == RE_SETITEM_LITCHAR
                            && itemBeforeCursor->LitChar.C == '\\'
                            && strchr(LEGAL_METACHARS, ctx->input_text[0])
                        ) {
                            SetItem_SetType(itemBeforeCursor, RE_SETITEM_METACHAR);
                            itemBeforeCursor->MetaChar.C = ctx->input_text[0];
                            break;
                        }
                    }

                    SetItem* newItem = SetItem_init(RE_NEW(SetItem));
                    newItem->LitChar.C = ctx->input_text[i];
                    Set_AddItem(set, newItem, set->TextState.InsertIndex);
                    set->TextState = TextState_BumpCursor(set->TextState, 1, 0);
                } while (0);
            }
        }

        // fix up cursor position
        set->TextState = TextState_Clamp(set->TextState, 0, set->NumItems);
    }
}

void prepass_Group(Group* group, NoUnionEx* ex, Unit* unit) {
    if (Group_CanRender(group)) {
        Regex* regex = group->Regex;
        prepass_Regex(regex, ex, unit);

        group->Size = (Vec2i) {
            .w = regex->Size.w,
            .h = GROUP_VERTICAL_PADDING + regex->Size.h + GROUP_VERTICAL_PADDING,
        };
        group->WireHeight = GROUP_VERTICAL_PADDING + regex->WireHeight;
    } else {
        int width = measureText(UNKNOWN_CONSTRUCT_TEXT, strlen(UNKNOWN_CONSTRUCT_TEXT));
        group->Size = (Vec2i) { .w = width + 20, .h = UNIT_CONTENTS_MIN_HEIGHT };
        group->WireHeight = UNIT_CONTENTS_MIN_HEIGHT/2;
    }
}
