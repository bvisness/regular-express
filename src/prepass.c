#include "prepass.h"

void prepass_Regex(Regex* regex, NoUnionEx* parentEx) {
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
        prepass_NoUnionEx(member, regex, parentEx);

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

void prepass_NoUnionEx(NoUnionEx* ex, Regex* regex, NoUnionEx* parentEx) {
    mu_Id muid = mu_get_id_noidstack(ctx, &ex, sizeof(NoUnionEx*));

    if (ctx->focus == muid) {
        ctx->updated_focus = 1;

        int inputTextLength = strlen(ctx->input_text);

        Unit* previousUnit = NULL;
        if (ex->TextState.InsertIndex > 0) {
            previousUnit = ex->Units[ex->TextState.InsertIndex - 1];
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
                && ex->TextState.InsertIndex == 0
        ) {
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
            ctx->focus = mu_get_id_noidstack(ctx, &previousEx, sizeof(NoUnionEx*));
            previousEx->TextState.InsertIndex = index;
        } else if (inputTextLength > 1) {
            // assume we are pasting and want to parse a regex
            // TODO: We should probably explicitly detect that we are pasting.
            Regex* parseResult = parse(ctx->input_text);

            if (parseResult->NumUnionMembers == 1) {
                // can insert all units inline
                NoUnionEx* src = parseResult->UnionMembers[0];
                while (src->NumUnits > 0) {
                    Unit* unit = NoUnionEx_RemoveUnit(src, 0);
                    NoUnionEx_AddUnit(ex, unit, ex->TextState.InsertIndex);
                    ex->TextState = TextState_BumpCursor(ex->TextState, 1, 0);
                }
            } else {
                // must create a group
                Unit* newUnit = Unit_init(RE_NEW(Unit));
                UnitContents_SetType(&newUnit->Contents, RE_CONTENTS_GROUP);
                newUnit->Contents.Group->Regex = parseResult;

                NoUnionEx_AddUnit(ex, newUnit, ex->TextState.InsertIndex);
            }

            Regex_delete(parseResult);
        } else {
            TextEditResult result = StandardTextInput(ctx, ex->TextState, ex->NumUnits);

            if (result.DoDelete) {
                result.DeleteMin = iclamp(result.DeleteMin, 0, ex->NumUnits);
                result.DeleteMax = iclamp(result.DeleteMax, 0, ex->NumUnits);

                for (int i = 0; i < result.DeleteMax - result.DeleteMin; i++) {
                    Unit* deleted = NoUnionEx_RemoveUnit(ex, result.DeleteMin);
                    Unit_delete(deleted);
                }
            }

            ex->TextState = result.ResultState;

            if (result.DoInput && inputTextLength == 1) {
                Unit* newUnit = NULL;
                if (ctx->key_down & MU_KEY_ALT && ctx->input_text[0] == '[') {
                    newUnit = Unit_init(RE_NEW(Unit));
                    UnitContents_SetType(&newUnit->Contents, RE_CONTENTS_SET);
                    mu_set_focus(ctx, mu_get_id_noidstack(ctx, &newUnit->Contents.Set, sizeof(Set*)));
                } else if (ctx->key_down & MU_KEY_ALT && ctx->input_text[0] == '9') {
                    newUnit = Unit_init(RE_NEW(Unit));
                    UnitContents_SetType(&newUnit->Contents, RE_CONTENTS_GROUP);
                    mu_set_focus(ctx, mu_get_id_noidstack(ctx, &newUnit->Contents.Group->Regex->UnionMembers[0], sizeof(NoUnionEx*)));
                } else if (ctx->key_down & MU_KEY_ALT && ctx->input_text[0] == '0') {
                    if (parentEx) {
                        mu_set_focus(ctx, mu_get_id_noidstack(ctx, &parentEx, sizeof(NoUnionEx*)));
                    }
                } else if (ctx->key_down & MU_KEY_ALT && ctx->input_text[0] == '6') {
                    // caret
                    newUnit = Unit_init(RE_NEW(Unit));
                    UnitContents_SetType(&newUnit->Contents, RE_CONTENTS_SPECIAL);
                    newUnit->Contents.Special->Type = RE_SPECIAL_STRINGSTART;
                } else if (ctx->key_down & MU_KEY_ALT && ctx->input_text[0] == '4') {
                    // dollar sign
                    newUnit = Unit_init(RE_NEW(Unit));
                    UnitContents_SetType(&newUnit->Contents, RE_CONTENTS_SPECIAL);
                    newUnit->Contents.Special->Type = RE_SPECIAL_STRINGEND;
                } else if (ctx->key_down & MU_KEY_ALT && ctx->input_text[0] == '.') {
                    newUnit = Unit_init(RE_NEW(Unit));
                    UnitContents_SetType(&newUnit->Contents, RE_CONTENTS_SPECIAL);
                    newUnit->Contents.Special->Type = RE_SPECIAL_ANY;
                } else if (ctx->key_down & MU_KEY_ALT && ctx->input_text[0] == '/') {
                    // question mark
                    Unit_SetRepeatMin(previousUnit, 0);
                    Unit_SetRepeatMax(previousUnit, 1);
                } else if (ctx->key_down & MU_KEY_ALT && ctx->input_text[0] == '=') {
                    // plus
                    Unit_SetRepeatMin(previousUnit, 1);
                    Unit_SetRepeatMax(previousUnit, -1);
                } else if (ctx->key_down & MU_KEY_ALT && ctx->input_text[0] == '8') {
                    // asterisk
                    Unit_SetRepeatMin(previousUnit, 0);
                    Unit_SetRepeatMax(previousUnit, -1);
                } else if (ctx->key_down & MU_KEY_ALT && ctx->input_text[0] == '\\') {
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

                    ctx->focus = mu_get_id_noidstack(ctx, &newEx, sizeof(NoUnionEx*));
                } else if (previousUnit
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
    prepass_UnitContents(contents, ex);

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

void prepass_UnitContents(UnitContents* contents, NoUnionEx* ex) {
    switch (contents->Type) {
        case RE_CONTENTS_LITCHAR: {
            contents->Size = (Vec2i) { .w = UNIT_CONTENTS_LITCHAR_WIDTH, .h = UNIT_CONTENTS_MIN_HEIGHT };
            contents->WireHeight = UNIT_CONTENTS_MIN_HEIGHT/2;
        } break;
        case RE_CONTENTS_METACHAR: {
            contents->Size = (Vec2i) { .w = 25, .h = UNIT_CONTENTS_MIN_HEIGHT };
            contents->WireHeight = UNIT_CONTENTS_MIN_HEIGHT/2;
        } break;
        case RE_CONTENTS_SPECIAL: {
            const char* str = Special_GetHumanString(contents->Special);
            int width = measureText(str, strlen(str));

            contents->Size = (Vec2i) { .w = width + 20, .h = UNIT_CONTENTS_MIN_HEIGHT };
            contents->WireHeight = UNIT_CONTENTS_MIN_HEIGHT/2;
        } break;
        case RE_CONTENTS_SET: {
            Set* set = contents->Set;

            prepass_Set(set, ex);

            // calculate sizes
            Vec2i contentsSize = {0};

            for (int i = 0; i < set->NumItems; i++) {
                SetItem* item = set->Items[i];

                if (item->Type == RE_SETITEM_LITCHAR) {
                    item->Size = (Vec2i) { .w = UNIT_CONTENTS_LITCHAR_WIDTH, .h = UNIT_CONTENTS_MIN_HEIGHT };
                } else if (item->Type == RE_SETITEM_RANGE) {
                    item->Size = (Vec2i) {
                        .w = UNIT_CONTENTS_LITCHAR_WIDTH + SET_DASH_WIDTH + UNIT_CONTENTS_LITCHAR_WIDTH,
                        .h = UNIT_CONTENTS_MIN_HEIGHT,
                    };
                }

                contentsSize.w += (i > 0 ? SET_HORIZONTAL_SPACING : 0) + item->Size.w;
                contentsSize.h = imax(contentsSize.h, item->Size.h);
            }

            contents->Size = (Vec2i) {
                .w = SET_PADDING + contentsSize.w + SET_PADDING,
                .h = SET_PADDING + contentsSize.h + SET_PADDING,
            };
            contents->WireHeight = UNIT_CONTENTS_MIN_HEIGHT/2;
        } break;
        case RE_CONTENTS_GROUP: {
            Group* group = contents->Group;
            prepass_Group(group, ex);
            contents->Size = group->Size;
            contents->WireHeight = group->WireHeight;
        }
    }
}

void prepass_Set(Set* set, NoUnionEx* ex) {
    mu_Id muid = mu_get_id_noidstack(ctx, &set, sizeof(Set*));

    if (ctx->focus == muid) {
        // Handle keyboard input to set up for next frame
        TextEditResult result = {0};
        int inputTextLength = strlen(ctx->input_text);
        SetItem* itemBeforeCursor = (set->TextState.InsertIndex > 0
            ? set->Items[set->TextState.InsertIndex - 1]
            : NULL
        );

        if (ctx->key_pressed & MU_KEY_BACKSPACE
                && itemBeforeCursor
                && itemBeforeCursor->Type == RE_SETITEM_RANGE
                && itemBeforeCursor->Range.Max.C != 0
        ) {
            itemBeforeCursor->Range.Max.C = 0;
        } else if (ctx->key_pressed & MU_KEY_BACKSPACE
                && itemBeforeCursor
                && itemBeforeCursor->Type == RE_SETITEM_RANGE
                && itemBeforeCursor->Range.Max.C == 0
        ) {
            itemBeforeCursor->Type = RE_SETITEM_LITCHAR;
            itemBeforeCursor->LitChar.C = itemBeforeCursor->Range.Min.C;

            SetItem* newItem = SetItem_init(RE_NEW(SetItem));
            newItem->LitChar.C = '-';
            Set_AddItem(set, newItem, set->TextState.InsertIndex);
            set->TextState = TextState_BumpCursor(set->TextState, 1, 0);
        } else {
            result = StandardTextInput(ctx, set->TextState, set->NumItems);
        }

        if (result.DoDelete) {
            result.DeleteMin = iclamp(result.DeleteMin, 0, set->NumItems);
            result.DeleteMax = iclamp(result.DeleteMax, 0, set->NumItems);

            for (int i = 0; i < result.DeleteMax - result.DeleteMin; i++) {
                Set_RemoveItem(set, result.DeleteMin);
            }
        }

        set->TextState = result.ResultState;

        if (result.DoInput) {
            for (int i = 0; i < inputTextLength; i++) {
                do {
                    if (inputTextLength == 1) {
                        if (ctx->key_down & MU_KEY_ALT && ctx->input_text[i] == ']') {
                            mu_set_focus(ctx, mu_get_id_noidstack(ctx, &ex, sizeof(NoUnionEx*)));
                            break;
                        }

                        if (
                            ctx->input_text[i] == '-'
                            && itemBeforeCursor
                            && itemBeforeCursor->Type != RE_SETITEM_RANGE
                        ) {
                            itemBeforeCursor->Type = RE_SETITEM_RANGE;
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
                    }

                    SetItem* newItem = SetItem_init(RE_NEW(SetItem));
                    newItem->LitChar.C = ctx->input_text[i];
                    Set_AddItem(set, newItem, set->TextState.InsertIndex);
                    set->TextState = TextState_BumpCursor(set->TextState, 1, 0);
                } while (0);
            }
        }

        // fix up cursor position
        set->TextState.InsertIndex = iclamp(set->TextState.InsertIndex, 0, set->NumItems);
    }
}

void prepass_Group(Group* group, NoUnionEx* ex) {
    Regex* regex = group->Regex;
    prepass_Regex(regex, ex);

    group->Size = (Vec2i) {
        .w = regex->Size.w,
        .h = GROUP_VERTICAL_PADDING + regex->Size.h + GROUP_VERTICAL_PADDING,
    };
    group->WireHeight = GROUP_VERTICAL_PADDING + regex->WireHeight;
}