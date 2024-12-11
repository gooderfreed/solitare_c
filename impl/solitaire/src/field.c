/*
 * Copyright 2024 Cheban Sergiy
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
*/

/*
 * Field implementation
 * Handles field object management and card placement
 */
#include "../inc/solitare.h"

/*
 * Print field content to screen
 * Draws the field with cards and borders
 */
static void print_field(void *field_pointer, Screen *screen, const Cursor *hovered_card) {
    Field *field = (Field *)field_pointer;
    int contentHeight = 2 * FIELD_HEIGHT - 1 + CARD_HEIGHT - 2;
    int contentWidth = SCREEN_WIDTH - BORDER_OFFSET_X - 1;

    fill_area(screen, DECK_OFFSET + BORDER_OFFSET_Y, BORDER_OFFSET_X, contentHeight, contentWidth, ' ', COLOR_GREEN, COLOR_RESET);

    int y_offset_base = DECK_OFFSET + BORDER_OFFSET_Y;
    int hovered_x = hovered_card->coords.x;
    int hovered_y = hovered_card->coords.y;

    for (int y = 0; y < FIELD_HEIGHT; ++y) {
        int y_0 = y * CARD_COVERED_HEIGHT + y_offset_base;
        int is_hovered_row = y > hovered_y;

        for (int x = 0; x < FIELD_WIDTH; ++x) {
            int additional_offset = (is_hovered_row && x == hovered_x && hovered_card->subject == field_pointer);
            int x_0 = x * CARD_WIDTH + BORDER_OFFSET_X;

            Card *current_card = field->field[y][x];

            if (y == 0 && !current_card) {
                add_borders(screen, y_offset_base, x_0, CARD_HEIGHT, CARD_WIDTH, COLOR_GREEN, COLOR_WHITE, fat_border);
            }
            else {
                print_card(screen, current_card, y_0 + additional_offset, x_0, CARD_HEIGHT, CARD_WIDTH);
            }
        }
    }
}

/*
 * Get last card y position in field
 * Finds the highest y position of a card in the specified column
 */
int get_last_card_y(const Field *field, int x) {
    for (int i = FIELD_HEIGHT - 1; i >= 0; --i)
        if (field->field[i][x]) return i;
    return 0;
}

/*
 * Place cursor in field
 * Calculates target coordinates based on cursor position and updates them
 */
static void place_cursor_in_field(void *field_pointer, Coords cursor_coords, Coords *target_coords) {
    Field *field = (Field *)field_pointer;
    int part = BORDER_OFFSET_Y + 1 + cursor_coords.y * 2 + BORDER_OFFSET_Y;
    bool is_last_card = FIELD_HEIGHT == cursor_coords.y || !field->field[cursor_coords.y + 1][cursor_coords.x];

    target_coords->y += part + (is_last_card ? CARD_HEIGHT : CARD_COVERED_HEIGHT + 1);
    target_coords->x += BORDER_OFFSET_X - 1;
}

/*
 * Move cursor in field
 * Updates cursor coordinates based on delta and ensures they are within valid bounds
 */
static void move_in_field(void *field_pointer, Coords *coords, Coords delta) {
    short new_x = coords->x + delta.x;
    short new_y = coords->y + delta.y;
    Field *field = (Field *)field_pointer;

    coords->x = (new_x + FIELD_WIDTH) % FIELD_WIDTH;
    if (new_y >= 0 && new_y < FIELD_HEIGHT && field->field[new_y][coords->x] && !field->field[new_y][coords->x]->hidden) coords->y = new_y;
    if (delta.y == 0) coords->y = (short)get_last_card_y(field, coords->x);
}

/*
 * Get cards from field
 * Removes selected cards from field and marks them as not selected
 */
static void get_cards_in_field(void *field_pointer, Container *container) {
    Field *field = (Field *)field_pointer;

    for (int i = 0; i < container->size; i++) {
        Card *card = (Card *)container_get_element(container, i);
        card->selected = false;
        field->field[card->coords.y][card->coords.x] = NULL;
        if (card->coords.y - 1 >= 0 && field->field[card->coords.y - 1][card->coords.x]) 
            field->field[card->coords.y - 1][card->coords.x]->hidden = false;
    }
}

/*
 * Select cards in field
 * Adds selected cards to container and marks them as selected
 */
static void select_cards_in_field(void *field_pointer, Coords cursor_coords, Container *container) {
    Field *field = (Field *)field_pointer;
    // If container is not empty, clear it
    if (!container_is_empty(container)) {
        while (!container_is_empty(container)) {
            ((Card *)container_pop_element(container))->selected = false;
        }
        return;
    }

    // Select cards in field
    for (int i = cursor_coords.y; field->field[i][cursor_coords.x] && i < FIELD_HEIGHT; ++i) {
        Card *current_card = field->field[i][cursor_coords.x];
        Card *next_card = (i + 1 < FIELD_HEIGHT) ? field->field[i + 1][cursor_coords.x] : NULL;

        // If move is valid, select card
        if (!next_card ||
            (current_card->numeral - next_card->numeral == 1 && 
            current_card->suit - next_card->suit % 2 != 0)) {
            current_card->selected = true;
            container_add_element(container, current_card);
        }
        else {
            // If move is not valid, clear container
            while (!container_is_empty(container)) {
                ((Card *)container_pop_element(container))->selected = false;
            }
            return;
        }
    }
    container_set_source(container, field_pointer);
}

/*
 * Check if cursor can place cards in field
 * Checks if move is valid
 */
static bool can_place_in_field(void *field_pointer, Coords cursor_coords, Container *container) {
    Field *field = (Field *)field_pointer;

    if (cursor_coords.y + container->size >= FIELD_HEIGHT) return false;
    if (field->field[cursor_coords.y + 1][cursor_coords.x] != NULL) return false;
    if (field->field[cursor_coords.y][cursor_coords.x] != NULL && field->field[cursor_coords.y][cursor_coords.x]->selected) return false;

    Card *card = (Card *)container_get_element(container, 0);
    Card *target_card = field->field[cursor_coords.y][cursor_coords.x];
    if (target_card == NULL) {
        if (card->numeral != King) return false;
    }
    else {
        if (target_card->suit % 2 == card->suit % 2) return false;
        if (target_card->numeral - card->numeral != 1) return false;
    }
    
    return true;
}

/*
 * Place cards in field
 * Places selected cards in field at specified cursor coordinates
 */
static void place_cards_in_field(void *field_pointer, Coords cursor_coord, Container *container) {
    Field *field = (Field *)field_pointer;
    bool y_offset = (field->field[cursor_coord.y][cursor_coord.x]);

    while (!container_is_empty(container)) {
        Card *card = container_pop_element(container);
        card->object = Field_enum;
        card->coords.x = cursor_coord.x;
        card->coords.y = (short)(cursor_coord.y + y_offset + container->size);
        field->field[cursor_coord.y + y_offset + container->size][cursor_coord.x] = card;
    }
}

/*
 * Check if cursor is on the same card in field
 * Compares cursor position to field card
 */
static bool is_same_card_in_field(void *field_pointer, Coords cursor_coords, Card *card) {
    Field *field = (Field *)field_pointer;
    return field->field[cursor_coords.y][cursor_coords.x] == card;
}

/*
 * Save current position in field
 * Sets current coordinates to restore coordinates
 */
static void save_current_pos_in_field(void *field_pointer, Coords current_coords) {
    Field *field = (Field *)field_pointer;
    GET_RESTORE_COORDS(field) = current_coords;
}

/*
 * Restore position in field
 * Sets restore coordinates to current coordinates and calculates last card y position
 */
static void restore_pos_in_field(void *field_pointer, Coords *current_coords) {
    Field *field = (Field *)field_pointer;
    current_coords->x = GET_RESTORE_COORDS(field).x;
    current_coords->y = (short)get_last_card_y(field, current_coords->x);
}

/*
 * Check if card is useful for field
 * Checks if card can be placed in field
 */
bool is_card_useful_for_field(Field *field, Card *card) {
    for (int x = 0; x < FIELD_WIDTH; x++) {
        int y = get_last_card_y(field, x);
        Card *target = field->field[y][x];
        if (!target) continue;
        
        Container container = {0};
        container_add_element(&container, card);
        if (can_place_in_field(field, (Coords) {.x = (short)x, .y = (short)(y)}, &container)) {
            return true;
        }
    }
    return false;
}

/*
 * Initialize field with cards from deck
 * Sets up field structure and interfaces
 */
Field init_field(Deck *deck) {
    Field field = {0};

    static const Drawable drawable = {
        .print = print_field
    };

    static const Interactable interactable = {
        .place_cursor = place_cursor_in_field,
        .move         = move_in_field
    };

    static const CardHandler card_handler = {
        .can_give_cards = true,
        .select_cards   = select_cards_in_field,
        .is_same_card   = is_same_card_in_field,
        .get_cards      = get_cards_in_field,

        .can_take_cards = true,
        .can_place      = can_place_in_field,
        .place_cards    = place_cards_in_field,
    };

    static PositionHandler position_handler = {
        .restore_coords = {0},
        .save_current_pos = save_current_pos_in_field,
        .restore_pos = restore_pos_in_field
    };

    field.interfaces = (ObjectInterfaces) {
        .name           = "Field",
        .capabilities = {
            .can_hold_cards  = true,
            .is_drawable     = true,
            .is_interactable = true,
            .is_positionable = true
        },
        .drawable =     &drawable,
        .interactable = &interactable,
        .card_handler = &card_handler,
        .position_handler = &position_handler
    };

    for (short row = 0; row < FIELD_WIDTH; row++) {
        for (short col = 0; col < FIELD_HEIGHT; col++) {
            if (col > row) field.field[col][row] = NULL;
            else {
                Card *card = draw_card(deck);
                card->coords.x = row;
                card->coords.y = col;
                card->object = Field_enum;
                card->hidden = col == row ? false : true;
                field.field[col][row] = card;
            }
        }
    }

    field.interfaces.position_handler->restore_coords = (Coords) {.x = 0, .y = 0};

    return field;
}
