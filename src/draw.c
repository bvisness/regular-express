#include "draw.h"

void draw_arbitrary_text(mu_Context* ctx, const char* str, mu_Vec2 pos, mu_Color color) {
    mu_draw_text(ctx, NULL, str, -1, pos, color);
}

// TODO: Maybe change these to use mu_Vec2.
static float START_ANGLES[4] = {     PI, 3*PI/2,    0, PI/2 };
static float END_ANGLES[4]   = { 3*PI/2,      0, PI/2,   PI };

int getQuadrantXY(int dx, int dy) {
    if (dx < 0) {
        if (dy < 0) {
            return 0;
        } else {
            return 3;
        }
    } else {
        if (dy < 0) {
            return 1;
        } else {
            return 2;
        }
    }
}

int getQuadrantYX(int dy, int dx) {
    if (dy < 0) {
        if (dx < 0) {
            return 2;
        } else {
            return 3;
        }
    } else {
        if (dx < 0) {
            return 1;
        } else {
            return 0;
        }
    }
}

void drawVerticalConnector(int startX, int startY, int inx, int dy, int outx) {
    int numArcs = (inx == 0 ? 0 : 1) + (outx == 0 ? 0 : 1);

    int q1 = getQuadrantXY(inx, dy);
    int q2 = getQuadrantYX(dy, outx);

    int r = imin(UNIT_REPEAT_WIRE_RADIUS, iabs(dy/numArcs));

    int r1offsety = inx == 0 ? 0 : isign(dy) * r;
    int r2offsety = outx == 0 ? 0 : -isign(dy) * r;

    mu_draw_arc(
        ctx,
        startX + -isign(inx) * r, startY + isign(dy) * r,
        r,
        START_ANGLES[q1], END_ANGLES[q1],
        COLOR_WIRE,
        WIRE_THICKNESS
    );
    mu_draw_line(
        ctx,
        startX, startY + r1offsety,
        startX, startY + dy + r2offsety,
        COLOR_WIRE,
        WIRE_THICKNESS
    );
    mu_draw_arc(
        ctx,
        startX + isign(outx) * r, startY + dy + -isign(dy) * r,
        r,
        START_ANGLES[q2], END_ANGLES[q2],
        COLOR_WIRE,
        WIRE_THICKNESS
    );
}

void drawConnector(int startX, int startY, int inx, int dy1, int dx, int dy2, int outx) {
    int dy1segments = inx == 0 ? 1 : 2;
    int dy2segments = outx == 0 ? 1 : 2;

    int q1 = getQuadrantXY(inx, dy1);
    int r1 = imin(UNIT_REPEAT_WIRE_RADIUS, iabs(dy1/dy1segments));
    int r1offsety = inx == 0 ? 0 : isign(dy1) * r1;

    int q2 = getQuadrantYX(dy1, dx);
    int r2 = imin(UNIT_REPEAT_WIRE_RADIUS, imin(iabs(dy1/dy1segments), iabs(dx/2)));

    int q3 = getQuadrantXY(dx, dy2);
    int r3 = imin(UNIT_REPEAT_WIRE_RADIUS, imin(iabs(dx/2), iabs(dy2)));

    int q4 = getQuadrantYX(dy2, outx);
    int r4 = imin(UNIT_REPEAT_WIRE_RADIUS, iabs(dy2/dy2segments));
    int r4offsety = outx == 0 ? 0 : -isign(dy2) * r4;

    if (inx != 0) {
        mu_draw_arc(
            ctx,
            startX + -isign(inx) * r1, startY + isign(dy1) * r1,
            r1,
            START_ANGLES[q1], END_ANGLES[q1],
            COLOR_WIRE,
            WIRE_THICKNESS
        );
    }

    mu_draw_line(
        ctx,
        startX, startY + r1offsety, startX, startY + (isign(dy1) * (iabs(dy1) - r2)),
        COLOR_WIRE,
        WIRE_THICKNESS
    );
    mu_draw_arc(
        ctx,
        startX + isign(dx) * r2, startY + dy1 + -isign(dy1) * r2,
        r2,
        START_ANGLES[q2], END_ANGLES[q2],
        COLOR_WIRE,
        WIRE_THICKNESS
    );
    mu_draw_line(
        ctx,
        startX + isign(dx) * r2, startY + dy1, startX + (isign(dx) * (iabs(dx) - r3)), startY + dy1,
        COLOR_WIRE,
        WIRE_THICKNESS
    );
    mu_draw_arc(
        ctx,
        startX + dx + -isign(dx) * r3, startY + dy1 + isign(dy2) * r3,
        r3,
        START_ANGLES[q3], END_ANGLES[q3],
        COLOR_WIRE,
        WIRE_THICKNESS
    );
    mu_draw_line(
        ctx,
        startX + dx, startY + dy1 + isign(dy2) * r3, startX + dx, startY + dy1 + dy2 + r4offsety,
        COLOR_WIRE,
        WIRE_THICKNESS
    );

    if (outx != 0) {
        mu_draw_arc(
            ctx,
            startX + dx + isign(outx) * r4, startY + dy1 + dy2 + -isign(dy2) * r4,
            r4,
            START_ANGLES[q4], END_ANGLES[q4],
            COLOR_WIRE,
            WIRE_THICKNESS
        );
    }
}

void drawRailroad_Regex(Regex* regex, Vec2i origin, int unitDepth) {
    mu_push_id(ctx, &regex, sizeof(Regex*));

    Vec2i memberOrigin = (Vec2i) {
        .x = origin.x + UNION_GUTTER_WIDTH,
        .y = origin.y,
    };

    // TODO: What if the union is empty?

    int wireY = origin.y + regex->WireHeight;

    // entry/exit wires (half gutter width)
    mu_draw_line(
        ctx,
        origin.x, wireY,
        origin.x + UNION_GUTTER_WIDTH/2, wireY,
        COLOR_WIRE,
        WIRE_THICKNESS
    );
    mu_draw_line(
        ctx,
        origin.x + regex->Size.x, wireY,
        origin.x + regex->Size.x - UNION_GUTTER_WIDTH/2, wireY,
        COLOR_WIRE,
        WIRE_THICKNESS
    );

    int finalMemberWireY = wireY;

    int leftConnectorX = origin.x + UNION_GUTTER_WIDTH/2;
    int rightConnectorX = origin.x + UNION_GUTTER_WIDTH + regex->UnionSize.w + UNION_GUTTER_WIDTH/2;

    // union members
    for (int i = 0; i < regex->NumUnionMembers; i++) {
        NoUnionEx* member = regex->UnionMembers[i];
        drawRailroad_NoUnionEx(member, memberOrigin, unitDepth);

        int memberWireY = memberOrigin.y + member->WireHeight;

        drawVerticalConnector(
            leftConnectorX, wireY, // TODO: Is wireY one pixel too high? (yes)
            10,
            memberWireY - wireY,
            10
        );

        mu_draw_line(
            ctx,
            origin.x + UNION_GUTTER_WIDTH/2 + (i == 0 ? 0 : UNIT_REPEAT_WIRE_RADIUS), memberWireY,
            origin.x + UNION_GUTTER_WIDTH, memberWireY,
            COLOR_WIRE,
            WIRE_THICKNESS
        );
        // TODO: The end x is the biggest wtf
        mu_draw_line(
            ctx,
            origin.x + UNION_GUTTER_WIDTH + member->Size.w, memberWireY,
            origin.x + regex->Size.w - UNION_GUTTER_WIDTH/2 - (i == 0 ? 0 : UNIT_REPEAT_WIRE_RADIUS), memberWireY,
            COLOR_WIRE,
            WIRE_THICKNESS
        );

        drawVerticalConnector(
            rightConnectorX, wireY, // TODO: Is wireY one pixel too high? (yes)
            -10,
            memberWireY - wireY,
            -10
        );

        finalMemberWireY = memberWireY;
        memberOrigin.y += UNION_VERTICAL_SPACING + member->Size.h;
    }

    const int PLUS_BUTTON_WIDTH = 30;
    mu_layout_set_next(ctx, mu_rect(origin.x + regex->Size.x/2 - PLUS_BUTTON_WIDTH/2, memberOrigin.y + 10, PLUS_BUTTON_WIDTH, 20), 0);
    if (mu_button(ctx, "+")) {
        ctx->animating = 1;

        NoUnionEx* newMember = NoUnionEx_init(RE_NEW(NoUnionEx));
        Regex_AddUnionMember(regex, newMember, -1);

        const char* initialString = "hello";
        for (int i = 0; i < strlen(initialString); i++) {
            Unit* unit = Unit_initWithLiteralChar(RE_NEW(Unit), initialString[i]);
            NoUnionEx_AddUnit(newMember, unit, -1);
        }

        mu_set_focus(ctx, NoUnionEx_GetID(newMember));
        newMember->TextState = TextState_SelectRange(0, newMember->NumUnits-1);
    }

    mu_pop_id(ctx);
}

void drawRailroad_NoUnionEx(NoUnionEx* ex, Vec2i origin, int unitDepth) {
    mu_Id muid = mu_get_id_noidstack(ctx, &ex, sizeof(NoUnionEx*));

    ex->ClickedUnitIndex = -1;

    // check if we were selected by a box select
    if (drag.Type == DRAG_TYPE_BOX_SELECT && drag.BoxSelect.Result.Ex == ex) {
        mu_set_focus(ctx, muid);

        TextInputState newState = DEFAULT_TEXT_INPUT_STATE;
        newState = TextState_SetInsertIndex(newState, drag.BoxSelect.Result.Start, 0);
        newState = TextState_MoveCursor(newState, drag.BoxSelect.Result.End - drag.BoxSelect.Result.Start + 1, 1);
        ex->TextState = newState;

        drag.BoxSelect.Result = (UnitRange) {0};
    }

    UnitRange selection = {0};
    if (ctx->focus == muid && TextState_IsSelecting(ex->TextState)) {
        selection = (UnitRange) {
            .Ex = ex,
            .Start = TextState_SelectionStart(ex->TextState),
            .End = TextState_SelectionEnd(ex->TextState),
        };
    }

    int unitX = origin.x;
    for (int i = 0; i < ex->NumUnits; i++) {
        Unit* unit = ex->Units[i];

        drawRailroad_Unit(
            unit,
            ex,
            (Vec2i) {
                .x = unitX,
                .y = origin.y + ex->WireHeight - unit->WireHeight,
            },
            unitDepth,
            selection.Ex ? &selection : NULL
        );

        unitX += unit->Size.w;
    }

    if (ex->ClickedUnitIndex != -1) {
        ex->TextState = TextState_SetInsertIndex(ex->TextState, ex->ClickedUnitIndex, ctx->key_down & MU_KEY_SHIFT);
        // TODO: Detect a click on the right half and use TEXT_CURSOR_RIGHT
    }

    if (ctx->focus == muid) {
        lastFocusedEx = ex;

        // draw cursor
        Unit* cursorUnit = ex->Units[ex->TextState.CursorIndex];
        mu_Rect contentsRect = cursorUnit->Contents.LastRect;

        if (ex->TextState.CursorRight) {
            mu_draw_rect(
                ctx,
                mu_rect(
                    contentsRect.x + contentsRect.w - CURSOR_THICKNESS,
                    contentsRect.y + CURSOR_VERTICAL_PADDING,
                    CURSOR_THICKNESS,
                    contentsRect.h - CURSOR_VERTICAL_PADDING*2
                ),
                COLOR_CURSOR
            );
        } else {
            mu_draw_rect(
                ctx,
                mu_rect(
                    contentsRect.x,
                    contentsRect.y + CURSOR_VERTICAL_PADDING,
                    CURSOR_THICKNESS,
                    contentsRect.h - CURSOR_VERTICAL_PADDING*2
                ),
                COLOR_CURSOR
            );
        }

        // draw floating UI
        if (selection.Ex) {
            // selection bounding box
            // TODO: UnitRange Helper
            mu_Rect sbb = selection.Ex->Units[selection.Start]->LastRect;
            for (int i = selection.Start + 1; i <= selection.End; i++) {
                sbb = rect_union(sbb, selection.Ex->Units[i]->LastRect);
            }

            const int BUTTON_WIDTH = 100;

            mu_layout_set_next(ctx, mu_rect(sbb.x + sbb.w/2 - BUTTON_WIDTH/2, sbb.y - 20, BUTTON_WIDTH, 20), 0);
            if (!drag.Type && mu_button(ctx, "Make Group")) {
                Unit* newUnit = ConvertRangeToGroup(selection);
                mu_set_focus(ctx, NoUnionEx_GetID(selection.Ex));
                selection.Ex->TextState = TextState_SetCursorIndex(newUnit->Index, 1);
            }
        }
    }
}

void drawRailroad_Unit(Unit* unit, NoUnionEx* parent, Vec2i origin, int depth, UnitRange* selection) {
    mu_Rect rect = mu_rect(origin.x, origin.y, unit->Size.w, unit->Size.h);
    int isHover = !(drag.Type == DRAG_TYPE_BOX_SELECT) && mu_mouse_over(ctx, rect);
    int isWireDragOrigin = (
        (drag.Type == DRAG_TYPE_WIRE && drag.Wire.OriginUnit == unit)
        || (drag.Type == DRAG_TYPE_CREATE_UNION && drag.CreateUnion.OriginUnit == unit)
    );
    int nonSingular = Unit_IsNonSingular(unit);
    int isSelected = selection ? TextState_IsSelected(parent->TextState, unit->Index) : 0;

    mu_Rect contentsRect = mu_rect(
        origin.x + unit->LeftHandleZoneWidth + (nonSingular ? UNIT_WIRE_ATTACHMENT_ZONE_WIDTH : 0),
        origin.y + UNIT_REPEAT_WIRE_ZONE_HEIGHT,
        unit->Contents.Size.w,
        unit->Contents.Size.h
    );
    int isContentHover = isHover && mu_mouse_over(ctx, contentsRect);

    // save some of 'em for the next pass
    unit->LastRect = rect;
    unit->IsHover = isHover;
    unit->IsContentHover = isContentHover;
    unit->IsWireDragOrigin = isWireDragOrigin;
    unit->IsSelected = isSelected;

    int middleY = origin.y + UNIT_REPEAT_WIRE_ZONE_HEIGHT + unit->Contents.WireHeight;

    // thru-wires
    mu_draw_rect(
        ctx,
        mu_rect(
            origin.x,
            middleY - WIRE_THICKNESS/2,
            unit->LeftHandleZoneWidth + (nonSingular ? UNIT_WIRE_ATTACHMENT_ZONE_WIDTH : 0),
            WIRE_THICKNESS
        ),
        COLOR_WIRE
    );
    mu_draw_rect(
        ctx,
        mu_rect(
            origin.x
                + unit->LeftHandleZoneWidth
                + (nonSingular ? UNIT_WIRE_ATTACHMENT_ZONE_WIDTH : 0)
                + unit->Contents.Size.x,
            middleY - WIRE_THICKNESS/2,
            (nonSingular ? UNIT_WIRE_ATTACHMENT_ZONE_WIDTH : 0) + unit->RightHandleZoneWidth,
            WIRE_THICKNESS
        ),
        COLOR_WIRE
    );

    int handleY = middleY - HANDLE_SIZE/2;

    mu_Rect leftHandleRect;
    mu_Rect rightHandleRect;
    int overLeftHandle = 0;
    int overRightHandle = 0;
    int shouldShowLeftHandle = Unit_ShouldShowLeftHandle(unit);
    int shouldShowRightHandle = Unit_ShouldShowRightHandle(unit);

    if (drag.Type == DRAG_TYPE_BOX_SELECT) {
        shouldShowLeftHandle = 0;
        shouldShowRightHandle = 0;
    }

    unit->IsShowingLeftHandle = shouldShowLeftHandle;
    unit->IsShowingRightHandle = shouldShowRightHandle;

    // left handle
    if (shouldShowLeftHandle) {
        int handleX = origin.x + unit->LeftHandleZoneWidth/2 - HANDLE_SIZE/2;
        mu_Rect handleRect = mu_rect(handleX, handleY, HANDLE_SIZE, HANDLE_SIZE);
        mu_draw_rounded_rect(ctx, handleRect, COLOR_WIRE, 2);
        leftHandleRect = handleRect;
        overLeftHandle = mu_mouse_over(ctx, handleRect);
    }
    // right handle
    if (shouldShowRightHandle) {
        int handleX = origin.x
            + unit->LeftHandleZoneWidth
            + (nonSingular ? UNIT_WIRE_ATTACHMENT_ZONE_WIDTH : 0)
            + unit->Contents.Size.x
            + (nonSingular ? UNIT_WIRE_ATTACHMENT_ZONE_WIDTH : 0)
            + unit->RightHandleZoneWidth/2
            - HANDLE_SIZE/2;
        mu_Rect handleRect = mu_rect(handleX, handleY, HANDLE_SIZE, HANDLE_SIZE);
        mu_draw_rounded_rect(ctx, handleRect, COLOR_WIRE, 2);
        rightHandleRect = handleRect;
        overRightHandle = mu_mouse_over(ctx, handleRect);
    }

    unit->IsLeftWireHover = mu_mouse_over(ctx, mu_rect(
        origin.x,
        contentsRect.y,
        unit->LeftHandleZoneWidth + (nonSingular ? UNIT_WIRE_ATTACHMENT_ZONE_WIDTH : 0),
        unit->Contents.Size.y
    ));
    unit->IsRightWireHover = mu_mouse_over(ctx, mu_rect(
        contentsRect.x + contentsRect.w,
        contentsRect.y,
        (nonSingular ? UNIT_WIRE_ATTACHMENT_ZONE_WIDTH : 0)
            + unit->RightHandleZoneWidth
            + (Unit_Next(unit) && Unit_IsNonSingular(Unit_Next(unit)) ? UNIT_WIRE_ATTACHMENT_ZONE_WIDTH : 0),
        unit->Contents.Size.y
    ));

    int leftWireX = origin.x
        + unit->LeftHandleZoneWidth
        + UNIT_WIRE_ATTACHMENT_ZONE_WIDTH/2;
    int rightWireX = origin.x
        + unit->LeftHandleZoneWidth
        + UNIT_WIRE_ATTACHMENT_ZONE_WIDTH
        + unit->Contents.Size.x
        + UNIT_WIRE_ATTACHMENT_ZONE_WIDTH/2;

    if (unit->RepeatMin < 1) {
        // draw the skip wire
        int skipWireHeight = unit->Contents.WireHeight + UNIT_REPEAT_WIRE_MARGIN;

        drawConnector(
            leftWireX, middleY,
            10,
            -skipWireHeight,
            rightWireX - leftWireX,
            skipWireHeight,
            10
        );
    }

    if (unit->RepeatMax != 1) {
        // draw the repeat wire
        int repeatWireHeight = unit->Contents.Size.y
            - unit->Contents.WireHeight
            + UNIT_REPEAT_WIRE_MARGIN;

        drawConnector(
            rightWireX, middleY,
            10,
            repeatWireHeight,
            leftWireX - rightWireX,
            -repeatWireHeight,
            10
        );
    }

    drawRailroad_UnitContents(
        &unit->Contents,
        (Vec2i) {
            .x = contentsRect.x,
            .y = contentsRect.y,
        },
        depth,
        isSelected
    );

    int targetLeftHandleZoneWidth = shouldShowLeftHandle ? UNIT_HANDLE_ZONE_WIDTH : 0;
    int targetRightHandleZoneWidth = shouldShowRightHandle ? UNIT_HANDLE_ZONE_WIDTH : 0;

    int animating = 0;
    if (drag.Type != DRAG_TYPE_BOX_SELECT) {
        unit->LeftHandleZoneWidth = interp_linear(ctx->dt, unit->LeftHandleZoneWidth, targetLeftHandleZoneWidth, 160, &animating);
        ctx->animating |= animating;
        unit->RightHandleZoneWidth = interp_linear(ctx->dt, unit->RightHandleZoneWidth, targetRightHandleZoneWidth, 160, &animating);
        ctx->animating |= animating;
    }

    mu_Rect leftHandleZoneRect = mu_rect(
        rect.x,
        rect.y,
        unit->LeftHandleZoneWidth,
        rect.h
    );
    mu_Rect rightHandleZoneRect = mu_rect(
        contentsRect.x + contentsRect.w,
        rect.y,
        unit->RightHandleZoneWidth,
        rect.h
    );

    if (ctx->mouse_released & MU_MOUSE_LEFT && mu_mouse_over(ctx, contentsRect) && !drag.Type) {
        if (unit->Contents.Type == RE_CONTENTS_SET) {
            // do nothing, let the set handle the click
        } else {
            ctx->mouse_released &= ~MU_MOUSE_LEFT;
            mu_set_focus(ctx, mu_get_id_noidstack(ctx, &parent, sizeof(NoUnionEx*)));
            parent->ClickedUnitIndex = unit->Index;
        }
    }

    if (ctx->mouse_down == MU_MOUSE_LEFT) {
        if (!drag.Type && ctx->mouse_started_drag) {
            // maybe start drag
            if (overLeftHandle) {
                if (ctx->key_down & MU_KEY_ALT) {
                    UnitRange units = (UnitRange) {
                        .Ex = parent,
                        .Start = unit->Index,
                        .End = unit->Index,
                    };
                    if (selection && selection->Start == units.Start) {
                        units = *selection;
                    }

                    drag = (DragContext) {
                        .Type = DRAG_TYPE_CREATE_UNION,
                        .CreateUnion = (DragCreateUnion) {
                            .Units = units,
                            .OriginUnit = unit,
                            .WhichHandle = DRAG_WIRE_LEFT_HANDLE,
                        },
                    };
                } else {
                    drag = (DragContext) {
                        .Type = DRAG_TYPE_WIRE,
                        .Wire = (DragWire) {
                            .OriginUnit = unit,
                            .UnitBeforeHandle = Unit_Previous(unit),
                            .UnitAfterHandle = unit,
                            .WhichHandle = DRAG_WIRE_LEFT_HANDLE,
                        },
                    };
                }
            } else if (overRightHandle) {
                if (ctx->key_down & MU_KEY_ALT) {
                    Unit* next = Unit_Next(unit);
                    if (next) {
                        UnitRange units = (UnitRange) {
                            .Ex = parent,
                            .Start = next->Index,
                            .End = next->Index,
                        };
                        if (selection && selection->Start == units.Start) {
                            units = *selection;
                        }

                        drag = (DragContext) {
                            .Type = DRAG_TYPE_CREATE_UNION,
                            .CreateUnion = (DragCreateUnion) {
                                .Units = units,
                                .OriginUnit = unit,
                                .WhichHandle = DRAG_WIRE_RIGHT_HANDLE,
                            },
                        };
                    }
                } else {
                    drag = (DragContext) {
                        .Type = DRAG_TYPE_WIRE,
                        .Wire = (DragWire) {
                            .OriginUnit = unit,
                            .UnitBeforeHandle = unit,
                            .UnitAfterHandle = Unit_Next(unit),
                            .WhichHandle = DRAG_WIRE_RIGHT_HANDLE,
                        },
                    };
                }
            } else if (mu_mouse_over(ctx, contentsRect)) {
                moveUnitsEx = (NoUnionEx) {0};

                UnitRange unitsToMove = (UnitRange) {
                    .Ex = parent,
                    .Start = unit->Index,
                    .End = unit->Index,
                };

                if (isSelected) {
                    unitsToMove = *selection;
                }

                MoveUnitsTo(unitsToMove, &moveUnitsEx, 0);
                mu_set_focus(ctx, NoUnionEx_GetID(&moveUnitsEx));

                drag = (DragContext) {
                    .Type = DRAG_TYPE_MOVE_UNITS,
                    .MoveUnits = (DragMoveUnits) {
                        .OriginEx = parent,
                        .OriginIndex = unitsToMove.Start,
                    },
                };
            }

            if (drag.Type) {
                // consume the mouse input so no other drags start
                ctx->mouse_pressed &= ~MU_MOUSE_LEFT;
            }
        }
    } else if (!(ctx->mouse_down & MU_MOUSE_LEFT) && drag.Type) {
        int didHandleDrag = 0;

        if (drag.Type == DRAG_TYPE_WIRE) {
            // drag finished
            ctx->animating = 1;

            // seek back and forth to figure out what the heck kind of drag this is
            UnitRange groupUnits = {0};
            int isForward = 0; // vs. backward

            // seek left from the drag origin to find repetition
            if (drag.Wire.UnitBeforeHandle) {
                NoUnionEx* ex = drag.Wire.UnitBeforeHandle->Parent;
                for (int i = drag.Wire.UnitBeforeHandle->Index; i >= 0; i--) {
                    Unit* visitingUnit = ex->Units[i];

                    if (
                        (unit == visitingUnit && overLeftHandle)
                        || (Unit_Next(unit) == visitingUnit && overRightHandle)
                    ) {
                        groupUnits = (UnitRange) {
                            .Ex = parent,
                            .Start = visitingUnit->Index,
                            .End = drag.Wire.UnitBeforeHandle->Index,
                        };
                        isForward = 0;
                        break;
                    }
                }
            }

            // seek right from the drag origin to find skips
            if (drag.Wire.UnitAfterHandle) {
                NoUnionEx* ex = drag.Wire.UnitAfterHandle->Parent;
                for (int i = drag.Wire.UnitAfterHandle->Index; i < ex->NumUnits; i++) {
                    Unit* visitingUnit = ex->Units[i];

                    if (
                        (unit == visitingUnit && overRightHandle)
                        || (Unit_Previous(unit) == visitingUnit && overLeftHandle)
                    ) {
                        groupUnits = (UnitRange) {
                            .Ex = parent,
                            .Start = drag.Wire.UnitAfterHandle->Index,
                            .End = visitingUnit->Index,
                        };
                        isForward = 1;
                        break;
                    }
                }
            }

            if (groupUnits.Ex) {
                didHandleDrag = 1;

                if (groupUnits.Start == groupUnits.End) {
                    // drag onto this same unit. no group shenanigans!
                    if (isForward) {
                        // forward drag, skip
                        Unit_SetRepeatMin(groupUnits.Ex->Units[groupUnits.Start], 0);
                    } else {
                        // backward drag, repeat
                        Unit_SetRepeatMax(groupUnits.Ex->Units[groupUnits.Start], 0);
                    }
                } else {
                    Unit* newUnit = ConvertRangeToGroup(groupUnits);

                    if (isForward) {
                        Unit_SetRepeatMin(newUnit, 0);
                    } else {
                        Unit_SetRepeatMax(newUnit, 0);
                    }
                }
            }
        } else if (drag.Type == DRAG_TYPE_MOVE_UNITS) {
            if (mu_mouse_over(ctx, leftHandleZoneRect)) {
                didHandleDrag = 1;
                MoveAllUnitsTo(unit->Parent, unit->Index);
            } else if (mu_mouse_over(ctx, rightHandleZoneRect)) {
                didHandleDrag = 1;
                MoveAllUnitsTo(unit->Parent, unit->Index+1);
            }
        }

        if (didHandleDrag) {
            drag = (DragContext) {0};
        }
    }

    if (ctx->mouse_down & MU_MOUSE_LEFT && drag.Type) {
        if (drag.Type == DRAG_TYPE_WIRE && drag.Wire.OriginUnit == unit) {
            if (drag.Wire.WhichHandle == DRAG_WIRE_LEFT_HANDLE) {
                drag.Wire.WireStartPos = (Vec2i) {
                    .x = leftHandleRect.x + leftHandleRect.w/2,
                    .y = leftHandleRect.y + leftHandleRect.h/2,
                };
            } else if (drag.Wire.WhichHandle == DRAG_WIRE_RIGHT_HANDLE) {
                drag.Wire.WireStartPos = (Vec2i) {
                    .x = rightHandleRect.x + rightHandleRect.w/2,
                    .y = rightHandleRect.y + rightHandleRect.h/2,
                };
            }
        } else if (drag.Type == DRAG_TYPE_CREATE_UNION && drag.CreateUnion.OriginUnit == unit) {
            if (drag.CreateUnion.WhichHandle == DRAG_WIRE_LEFT_HANDLE) {
                drag.CreateUnion.WireStartPos = (Vec2i) {
                    .x = leftHandleRect.x + leftHandleRect.w/2,
                    .y = leftHandleRect.y + leftHandleRect.h/2,
                };
            } else if (drag.CreateUnion.WhichHandle == DRAG_WIRE_RIGHT_HANDLE) {
                drag.Wire.WireStartPos = (Vec2i) {
                    .x = rightHandleRect.x + rightHandleRect.w/2,
                    .y = rightHandleRect.y + rightHandleRect.h/2,
                };
            }
        } else if (drag.Type == DRAG_TYPE_BOX_SELECT) {
            if (
                rect_overlaps(rect, drag.BoxSelect.Rect)
                && !rect_contains(rect, drag.BoxSelect.Rect)
            ) {
                PotentialSelect* potential = &drag.BoxSelect.Potentials[depth];
                if (!potential->Range.Ex) {
                    // initialize a selection at this level
                    (*potential) = (PotentialSelect) {
                        .Valid = 1,
                        .Range = (UnitRange) {
                            .Ex = parent,
                            .Start = unit->Index,
                            .End = unit->Index,
                        },
                    };
                } else {
                    // we already have a selection at this level;
                    // either continue it or destroy it
                    if (Unit_Previous(unit) && potential->Range.End == Unit_Previous(unit)->Index) { // TODO: This check feels very wrong but I don't feel like understanding it right now.
                        potential->Range.End = unit->Index;
                    } else {
                        potential->Valid = 0;
                    }
                }
            }
        }
    }
}

void drawRailroad_UnitContents(UnitContents* contents, Vec2i origin, int unitDepth, int selected) {
    mu_Rect r = mu_rect(origin.x, origin.y, contents->Size.w, contents->Size.h);
    contents->LastRect = r;

    mu_Color backgroundColor = selected ? COLOR_SELECTED_BACKGROUND : COLOR_UNIT_BACKGROUND;

    mu_layout_set_next(ctx, r, 0);
    switch (contents->Type) {
        case RE_CONTENTS_LITCHAR: {
            mu_draw_rect(ctx, r, backgroundColor);

            if (contents->LitChar.C == ' ') {
                // spaces get special treatment
                mu_draw_rect(ctx,
                    mu_rect(origin.x + 3, origin.y + contents->Size.h - 4, contents->Size.w - 6, 1),
                    COLOR_RE_TEXT_DIM
                );
            } else {
                char* str = contents->LitChar._buf;
                mu_Vec2 pos = mu_position_text(ctx, str, mu_layout_next(ctx), NULL, MU_OPT_ALIGNCENTER);
                draw_arbitrary_text(ctx, str, pos, COLOR_RE_TEXT);
            }
        } break;
        case RE_CONTENTS_METACHAR: {
            mu_draw_rect(ctx, r, selected ? backgroundColor : COLOR_METACHAR_BACKGROUND);

            char* str = MetaChar_GetHumanString(&contents->MetaChar);
            mu_Vec2 pos = mu_position_text(ctx, str, mu_layout_next(ctx), NULL, MU_OPT_ALIGNCENTER);
            draw_arbitrary_text(ctx, str, pos, COLOR_RE_TEXT);
        } break;
        case RE_CONTENTS_SPECIAL: {
            mu_draw_rect(ctx, r, selected ? backgroundColor : COLOR_SPECIAL_BACKGROUND);

            const char* str = Special_GetHumanString(&contents->Special);
            mu_Vec2 pos = mu_position_text(ctx, str, mu_layout_next(ctx), NULL, MU_OPT_ALIGNCENTER);
            draw_arbitrary_text(ctx, str, pos, COLOR_RE_TEXT);
        } break;
        case RE_CONTENTS_SET: {
            mu_draw_rect(ctx, r, backgroundColor);
            drawRailroad_Set(contents->Set, contents, origin);
        } break;
        case RE_CONTENTS_GROUP: {
            drawRailroad_Group(contents->Group, origin, unitDepth, selected);
        } break;
        case RE_CONTENTS_UNKNOWN: {
            mu_draw_rect(ctx, r, selected ? backgroundColor : COLOR_UNKNOWN_CONSTRUCT_BACKGROUND);
            mu_Vec2 pos = mu_position_text(ctx, contents->Unknown.Str, mu_layout_next(ctx), NULL, MU_OPT_ALIGNCENTER);
            draw_arbitrary_text(ctx, contents->Unknown.Str, pos, COLOR_RE_TEXT);
        } break;
    }
}

void drawRailroad_Set(Set* set, UnitContents* parent, Vec2i origin) {
    mu_Id muid = mu_get_id_noidstack(ctx, &set, sizeof(Set*));

    // Draw "one of"
    {
        const char* oneofStr = set->IsNegative ? SET_ONEOF_TEXT_NEG : SET_ONEOF_TEXT;
        mu_Rect rect = mu_rect(
            origin.x + SET_PADDING + 1,
            origin.y + SET_PADDING + 1,
            parent->Size.x - 2*SET_PADDING - 2,
            SET_ONEOF_HEIGHT - 2
        );
        mu_layout_set_next(ctx, rect, 0);
        if (mu_button(ctx, oneofStr)) {
            set->IsNegative = !set->IsNegative;
        }
    }

    int itemX = origin.x + parent->Size.w/2 - set->ItemsSize.w/2;
    int itemY = origin.y + SET_PADDING + SET_ONEOF_HEIGHT + SET_PADDING;

    const char* dashStr = "-";

    for (int i = 0; i < set->NumItems; i++) {
        SetItem* item = set->Items[i];

        int selected = TextState_IsSelected(set->TextState, i);

        mu_Rect itemRect = mu_rect(itemX, itemY, item->Size.w, item->Size.h);
        mu_draw_rect(ctx, itemRect, selected ? COLOR_SELECTED_BACKGROUND : COLOR_SET_ITEM_BACKGROUND);

        if (item->Type == RE_SETITEM_LITCHAR) {
            mu_layout_set_next(ctx, itemRect, 0);
            char* str = item->LitChar._buf;
            mu_Vec2 pos = mu_position_text(ctx, str, mu_layout_next(ctx), NULL, MU_OPT_ALIGNCENTER);
            draw_arbitrary_text(ctx, str, pos, COLOR_RE_TEXT);
        } else if (item->Type == RE_SETITEM_METACHAR) {
            mu_layout_set_next(ctx, itemRect, 0);
            char* str = &item->MetaChar._backslash;
            mu_Vec2 pos = mu_position_text(ctx, str, mu_layout_next(ctx), NULL, MU_OPT_ALIGNCENTER);
            draw_arbitrary_text(ctx, str, pos, COLOR_RE_TEXT);
        } else if (item->Type == RE_SETITEM_RANGE) {
            {
                mu_Rect r = mu_rect(itemX, itemY, UNIT_CONTENTS_LITCHAR_WIDTH, itemRect.h);
                mu_layout_set_next(ctx, r, 0);
                char* str = item->Range.Min._buf;
                mu_Vec2 pos = mu_position_text(ctx, str, mu_layout_next(ctx), NULL, MU_OPT_ALIGNCENTER);
                draw_arbitrary_text(ctx, str, pos, COLOR_RE_TEXT);
            }

            {
                mu_Rect r = mu_rect(itemX + UNIT_CONTENTS_LITCHAR_WIDTH, itemY, SET_DASH_WIDTH, itemRect.h);
                mu_layout_set_next(ctx, r, 0);
                mu_Vec2 pos = mu_position_text(ctx, dashStr, mu_layout_next(ctx), NULL, MU_OPT_ALIGNCENTER);
                draw_arbitrary_text(ctx, dashStr, pos, COLOR_RE_TEXT);
            }

            {
                mu_Rect r = mu_rect(itemX + UNIT_CONTENTS_LITCHAR_WIDTH + SET_DASH_WIDTH, itemY, UNIT_CONTENTS_LITCHAR_WIDTH, itemRect.h);
                mu_layout_set_next(ctx, r, 0);
                char* str = item->Range.Max._buf;
                mu_Vec2 pos = mu_position_text(ctx, str, mu_layout_next(ctx), NULL, MU_OPT_ALIGNCENTER);
                draw_arbitrary_text(ctx, str, pos, COLOR_RE_TEXT);
            }
        }

        item->LastRect = itemRect;

        itemX += itemRect.w + SET_HORIZONTAL_SPACING;

        if (ctx->mouse_released & MU_MOUSE_LEFT && mu_mouse_over(ctx, itemRect)) {
            ctx->mouse_released &= ~MU_MOUSE_LEFT;
            mu_set_focus(ctx, muid);
            set->TextState = TextState_SetInsertIndex(set->TextState, i, 0);
        }
    }

    if (ctx->focus == muid) {
        ctx->updated_focus = 1;

        if (set->NumItems == 0) {
            mu_draw_rect(
                ctx,
                mu_rect(
                    origin.x + parent->Size.w/2 - CURSOR_THICKNESS/2,
                    itemY,
                    CURSOR_THICKNESS,
                    UNIT_CONTENTS_MIN_HEIGHT
                ),
                COLOR_CURSOR
            );
        } else {
            SetItem* cursorItem = set->Items[set->TextState.CursorIndex];
            mu_Rect itemRect = cursorItem->LastRect;

            if (set->TextState.CursorRight) {
                mu_draw_rect(
                    ctx,
                    mu_rect(
                        itemRect.x + itemRect.w - CURSOR_THICKNESS,
                        itemRect.y + CURSOR_VERTICAL_PADDING,
                        CURSOR_THICKNESS,
                        itemRect.h - CURSOR_VERTICAL_PADDING*2
                    ),
                    COLOR_CURSOR
                );
            } else {
                mu_draw_rect(
                    ctx,
                    mu_rect(
                        itemRect.x,
                        itemRect.y + CURSOR_VERTICAL_PADDING,
                        CURSOR_THICKNESS,
                        itemRect.h - CURSOR_VERTICAL_PADDING*2
                    ),
                    COLOR_CURSOR
                );
            }
        }
    }
}

void drawRailroad_Group(Group* group, Vec2i origin, int unitDepth, int selected) {
    mu_Rect r = mu_rect(origin.x, origin.y, group->Size.w, group->Size.h);

    if (Group_CanRender(group)) {
        mu_draw_rounded_rect(
            ctx,
            r,
            selected ? COLOR_SELECTED_BACKGROUND : COLOR_GROUP_BACKGROUND,
            4
        );

        drawRailroad_Regex(
            group->Regex,
            (Vec2i) {
                .x = origin.x,
                .y = GROUP_VERTICAL_PADDING + origin.y,
            },
            unitDepth + 1
        );
    } else {
        mu_draw_rect(ctx, r, selected ? COLOR_SELECTED_BACKGROUND : COLOR_UNKNOWN_CONSTRUCT_BACKGROUND);
        mu_Vec2 pos = mu_position_text(ctx, UNKNOWN_CONSTRUCT_TEXT, mu_layout_next(ctx), NULL, MU_OPT_ALIGNCENTER);
        draw_arbitrary_text(ctx, UNKNOWN_CONSTRUCT_TEXT, pos, COLOR_RE_TEXT);
    }
}
