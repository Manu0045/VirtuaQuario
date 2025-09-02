#include <gb/gb.h>
#include <gb/cgb.h>
#include <gbdk/console.h>
#include <gbdk/font.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "TILES/aquarium_ts.h"
#include "TILES/menu_ts.h"
#include "TILES/shop_ts.h"
#include "TILES/bubbles_ts.h"

#include "TILES/clownfish_ts.h"
#include "TILES/triggerfish_ts.h"
#include "TILES/surgeonfish_ts.h"
#include "TILES/gammaloreto_ts.h"
#include "TILES/lionfish_ts.h"

#include "TILES/cursor_ts.h"
#include "TILES/shop_cursor_ts.h"

#include "MAPS/aquarium.h"
#include "MAPS/menu.h"
#include "MAPS/shop.h"

// --- COSTANTI E DEFINIZIONI ---
#define SAVE_ADDR 0xA000
#define SAVE_MAGIC 0x42 // numero a caso per riconoscere salvataggio valido

#define MAX_BUBBLES 5
#define MAX_FISHES  5

// Modifica gli indici di base per evitare sovrapposizioni
#define FISH_SPRITE_ID_BASE   7   // I pesci iniziano da questo indice
#define CURSOR_SPRITE_ID_BASE 2   // Il cursore inizia da questo indice
#define SHOP_CURSOR_SPRITE_ID_BASE 3 // Il cursore dello shop inizia da questo indice
#define BUBBLE_SPRITE_ID_BASE (FISH_SPRITE_ID_BASE + (MAX_FISHES * 4)) // Le bolle vengono dopo tutti i pesci

#define FISH_STAGE0_TILE  7
#define FISH_STAGE1_TILE  8
#define FISH_STAGE2_TILE  9



// --- STRUTTURE ---

typedef struct {
    UINT8 magic;   // deve valere SAVE_MAGIC se i dati sono validi
    UINT16 hunger;
    UINT16 algae;
    UINT16 food;
    UINT16 medicine;
    UINT16 bubbles_currency;
    Fish fishes[MAX_FISHES];
} SaveData;


typedef enum {
    STATE_AQUARIUM,
    STATE_SHOP
} GameState;

typedef struct {
	UINT8 active;
    UINT8 sprite_id;
    UINT8 tile;
    UINT8 x, y;
} Bubble;

typedef enum {
    FISH_CLOWN,
    FISH_TRIGGER,
	FISH_SURGEON,
	FISH_GAMMA,
	FISH_LION
} FishType;

typedef struct {
	FishType type;
    UINT8 active;
    UINT8 frame;
    UINT8 stage;
    UINT8 x, y;
    INT8 dx;
    INT8 dy;
} Fish;

typedef struct {
	UINT8 active;
	UINT8 x, y;
	UINT8 row, col;
} Cursor;

// --- VARIABILI GLOBALI ---
static unsigned int rng_seed = 1;
unsigned char empty_tile = 0;
UINT8 prev_keys = 0;
UINT8 action_cooldown = 0;
UINT8 reset_hold_counter = 0;
GameState game_state = STATE_AQUARIUM;

Cursor cursor;
UINT8 cursor_move_delay = 0;

Cursor shop_cursor;
UINT8 shop_cursor_move_delay = 0;

Bubble bubbles[MAX_BUBBLES];
Fish fishes[MAX_FISHES];

UINT8 last_fish_index = 0;
UINT16 counter = 0;
UINT16 fish_timer = 0;
UINT8 spawn_if_even = 0;

const UINT8 metasprite_offsets[4][2] = {
    {0, 0}, {8, 0}, {0, 8}, {8, 8}
};


UINT16 hunger = 100;
UINT16 last_hunger = 9999;

UINT16 algae = 0;
UINT16 last_algae = 9999;

UINT16 food = 10;
UINT16 last_food = 9999;

UINT16 medicine = 3;
UINT16 last_medicine = 9999;

UINT16 bubbles_currency = 0;
UINT16 last_bubbles_currency = 9999;


// --- PROTOTIPI ---
void srand_gb(unsigned int seed);
int rand_gb(void);

void wait_release(void);

void init_gfx(void);
void update_water_color(void);
void init_ui(void);
void update_ui(void);
void animate_water(void);
void animate_treasure(void);

void init_bubbles_system(void);
void spawn_bubble(UINT8 x, UINT8 y);
void update_bubbles(void);

void init_fish_system(void);
void draw_fish(Fish* f, UINT8 oam_index);
void spawn_fish(UINT8 x, UINT8 y, FishType type);
void evolve_fishes(void);
void update_fishes(void);
void randomize_fish_direction(Fish* f);
UINT8 get_active_fish_count(void);
 
void use_food(void);
void use_medicine(void);

void init_shop_cursor(void); 
void init_shop(void);
void exit_shop(void);

// --- PALETTE ---
const UWORD aquarium_palettes[] = {
    aquarium_tsCGBPal0c0, aquarium_tsCGBPal0c1, aquarium_tsCGBPal0c2, aquarium_tsCGBPal0c3,
    aquarium_tsCGBPal1c0, aquarium_tsCGBPal1c1, aquarium_tsCGBPal1c2, aquarium_tsCGBPal1c3,
    aquarium_tsCGBPal2c0, aquarium_tsCGBPal2c1, aquarium_tsCGBPal2c2, aquarium_tsCGBPal2c3,
    aquarium_tsCGBPal3c0, aquarium_tsCGBPal3c1, aquarium_tsCGBPal3c2, aquarium_tsCGBPal3c3,
    aquarium_tsCGBPal4c0, aquarium_tsCGBPal4c1, aquarium_tsCGBPal4c2, aquarium_tsCGBPal4c3,
    aquarium_tsCGBPal5c0, aquarium_tsCGBPal5c1, aquarium_tsCGBPal5c2, aquarium_tsCGBPal5c3,
    aquarium_tsCGBPal6c0, aquarium_tsCGBPal6c1, aquarium_tsCGBPal6c2, aquarium_tsCGBPal6c3,
    aquarium_tsCGBPal7c0, aquarium_tsCGBPal7c1, aquarium_tsCGBPal7c2, aquarium_tsCGBPal7c3,
};

const UWORD bubbles_palettes[] = {
    bubbles_tsCGBPal0c0, bubbles_tsCGBPal0c1, bubbles_tsCGBPal0c2, bubbles_tsCGBPal0c3,
    bubbles_tsCGBPal1c0, bubbles_tsCGBPal1c1, bubbles_tsCGBPal1c2, bubbles_tsCGBPal1c3,
    bubbles_tsCGBPal2c0, bubbles_tsCGBPal2c1, bubbles_tsCGBPal2c2, bubbles_tsCGBPal2c3,
    bubbles_tsCGBPal3c0, bubbles_tsCGBPal3c1, bubbles_tsCGBPal3c2, bubbles_tsCGBPal3c3,
    bubbles_tsCGBPal4c0, bubbles_tsCGBPal4c1, bubbles_tsCGBPal4c2, bubbles_tsCGBPal4c3,
    bubbles_tsCGBPal5c0, bubbles_tsCGBPal5c1, bubbles_tsCGBPal5c2, bubbles_tsCGBPal5c3,
    bubbles_tsCGBPal6c0, bubbles_tsCGBPal6c1, bubbles_tsCGBPal6c2, bubbles_tsCGBPal6c3,
    bubbles_tsCGBPal7c0, bubbles_tsCGBPal7c1, bubbles_tsCGBPal7c2, bubbles_tsCGBPal7c3,
};

const UWORD cursor_palettes[] = {
    cursor_tsCGBPal0c0, cursor_tsCGBPal0c1, cursor_tsCGBPal0c2, cursor_tsCGBPal0c3,
    cursor_tsCGBPal1c0, cursor_tsCGBPal1c1, cursor_tsCGBPal1c2, cursor_tsCGBPal1c3,
    cursor_tsCGBPal2c0, cursor_tsCGBPal2c1, cursor_tsCGBPal2c2, cursor_tsCGBPal2c3,
    cursor_tsCGBPal3c0, cursor_tsCGBPal3c1, cursor_tsCGBPal3c2, cursor_tsCGBPal3c3,
    cursor_tsCGBPal4c0, cursor_tsCGBPal4c1, cursor_tsCGBPal4c2, cursor_tsCGBPal4c3,
    cursor_tsCGBPal5c0, cursor_tsCGBPal5c1, cursor_tsCGBPal5c2, cursor_tsCGBPal5c3,
    cursor_tsCGBPal6c0, cursor_tsCGBPal6c1, cursor_tsCGBPal6c2, cursor_tsCGBPal6c3,
    cursor_tsCGBPal7c0, cursor_tsCGBPal7c1, cursor_tsCGBPal7c2, cursor_tsCGBPal7c3,
};


const UWORD shop_palettes[] = {
    shop_tsCGBPal0c0, shop_tsCGBPal0c1, shop_tsCGBPal0c2, shop_tsCGBPal0c3,
    shop_tsCGBPal1c0, shop_tsCGBPal1c1, shop_tsCGBPal1c2, shop_tsCGBPal1c3,
    shop_tsCGBPal2c0, shop_tsCGBPal2c1, shop_tsCGBPal2c2, shop_tsCGBPal2c3,
    shop_tsCGBPal3c0, shop_tsCGBPal3c1, shop_tsCGBPal3c2, shop_tsCGBPal3c3,
    shop_tsCGBPal4c0, shop_tsCGBPal4c1, shop_tsCGBPal4c2, shop_tsCGBPal4c3,
    shop_tsCGBPal5c0, shop_tsCGBPal5c1, shop_tsCGBPal5c2, shop_tsCGBPal5c3,
    shop_tsCGBPal6c0, shop_tsCGBPal6c1, shop_tsCGBPal6c2, shop_tsCGBPal6c3,
    shop_tsCGBPal7c0, shop_tsCGBPal7c1, shop_tsCGBPal7c2, shop_tsCGBPal7c3,
};

const UWORD shop_cursor_palettes[] = {
    shop_cursor_tsCGBPal0c0, shop_cursor_tsCGBPal0c1, shop_cursor_tsCGBPal0c2, shop_cursor_tsCGBPal0c3,
    shop_cursor_tsCGBPal1c0, shop_cursor_tsCGBPal1c1, shop_cursor_tsCGBPal1c2, shop_cursor_tsCGBPal1c3,
    shop_cursor_tsCGBPal2c0, shop_cursor_tsCGBPal2c1, shop_cursor_tsCGBPal2c2, shop_cursor_tsCGBPal2c3,
    shop_cursor_tsCGBPal3c0, shop_cursor_tsCGBPal3c1, shop_cursor_tsCGBPal3c2, shop_cursor_tsCGBPal3c3,
    shop_cursor_tsCGBPal4c0, shop_cursor_tsCGBPal4c1, shop_cursor_tsCGBPal4c2, shop_cursor_tsCGBPal4c3,
    shop_cursor_tsCGBPal5c0, shop_cursor_tsCGBPal5c1, shop_cursor_tsCGBPal5c2, shop_cursor_tsCGBPal5c3,
    shop_cursor_tsCGBPal6c0, shop_cursor_tsCGBPal6c1, shop_cursor_tsCGBPal6c2, shop_cursor_tsCGBPal6c3,
    shop_cursor_tsCGBPal7c0, shop_cursor_tsCGBPal7c1, shop_cursor_tsCGBPal7c2, shop_cursor_tsCGBPal7c3,
};


const UWORD clownfish_palettes[] = {
    clownfish_tsCGBPal0c0, clownfish_tsCGBPal0c1, clownfish_tsCGBPal0c2, clownfish_tsCGBPal0c3,
    clownfish_tsCGBPal1c0, clownfish_tsCGBPal1c1, clownfish_tsCGBPal1c2, clownfish_tsCGBPal1c3,
    clownfish_tsCGBPal2c0, clownfish_tsCGBPal2c1, clownfish_tsCGBPal2c2, clownfish_tsCGBPal2c3,
    clownfish_tsCGBPal3c0, clownfish_tsCGBPal3c1, clownfish_tsCGBPal3c2, clownfish_tsCGBPal3c3,
    clownfish_tsCGBPal4c0, clownfish_tsCGBPal4c1, clownfish_tsCGBPal4c2, clownfish_tsCGBPal4c3,
    clownfish_tsCGBPal5c0, clownfish_tsCGBPal5c1, clownfish_tsCGBPal5c2, clownfish_tsCGBPal5c3,
    clownfish_tsCGBPal6c0, clownfish_tsCGBPal6c1, clownfish_tsCGBPal6c2, clownfish_tsCGBPal6c3,
    clownfish_tsCGBPal7c0, clownfish_tsCGBPal7c1, clownfish_tsCGBPal7c2, clownfish_tsCGBPal7c3,
};

const UWORD triggerfish_palettes[] = {
    triggerfish_tsCGBPal0c0, triggerfish_tsCGBPal0c1, triggerfish_tsCGBPal0c2, triggerfish_tsCGBPal0c3,
    triggerfish_tsCGBPal1c0, triggerfish_tsCGBPal1c1, triggerfish_tsCGBPal1c2, triggerfish_tsCGBPal1c3,
    triggerfish_tsCGBPal2c0, triggerfish_tsCGBPal2c1, triggerfish_tsCGBPal2c2, triggerfish_tsCGBPal2c3,
    triggerfish_tsCGBPal3c0, triggerfish_tsCGBPal3c1, triggerfish_tsCGBPal3c2, triggerfish_tsCGBPal3c3,
    triggerfish_tsCGBPal4c0, triggerfish_tsCGBPal4c1, triggerfish_tsCGBPal4c2, triggerfish_tsCGBPal4c3,
    triggerfish_tsCGBPal5c0, triggerfish_tsCGBPal5c1, triggerfish_tsCGBPal5c2, triggerfish_tsCGBPal5c3,
    triggerfish_tsCGBPal6c0, triggerfish_tsCGBPal6c1, triggerfish_tsCGBPal6c2, triggerfish_tsCGBPal6c3,
    triggerfish_tsCGBPal7c0, triggerfish_tsCGBPal7c1, triggerfish_tsCGBPal7c2, triggerfish_tsCGBPal7c3,
};

const UWORD surgeonfish_palettes[] = {
    surgeonfish_tsCGBPal0c0, surgeonfish_tsCGBPal0c1, surgeonfish_tsCGBPal0c2, surgeonfish_tsCGBPal0c3,
    surgeonfish_tsCGBPal1c0, surgeonfish_tsCGBPal1c1, surgeonfish_tsCGBPal1c2, surgeonfish_tsCGBPal1c3,
    surgeonfish_tsCGBPal2c0, surgeonfish_tsCGBPal2c1, surgeonfish_tsCGBPal2c2, surgeonfish_tsCGBPal2c3,
    surgeonfish_tsCGBPal3c0, surgeonfish_tsCGBPal3c1, surgeonfish_tsCGBPal3c2, surgeonfish_tsCGBPal3c3,
    surgeonfish_tsCGBPal4c0, surgeonfish_tsCGBPal4c1, surgeonfish_tsCGBPal4c2, surgeonfish_tsCGBPal4c3,
    surgeonfish_tsCGBPal5c0, surgeonfish_tsCGBPal5c1, surgeonfish_tsCGBPal5c2, surgeonfish_tsCGBPal5c3,
    surgeonfish_tsCGBPal6c0, surgeonfish_tsCGBPal6c1, surgeonfish_tsCGBPal6c2, surgeonfish_tsCGBPal6c3,
    surgeonfish_tsCGBPal7c0, surgeonfish_tsCGBPal7c1, surgeonfish_tsCGBPal7c2, surgeonfish_tsCGBPal7c3,
};

const UWORD gammaloreto_palettes[] = {
    gammaloreto_tsCGBPal0c0, gammaloreto_tsCGBPal0c1, gammaloreto_tsCGBPal0c2, gammaloreto_tsCGBPal0c3,
    gammaloreto_tsCGBPal1c0, gammaloreto_tsCGBPal1c1, gammaloreto_tsCGBPal1c2, gammaloreto_tsCGBPal1c3,
    gammaloreto_tsCGBPal2c0, gammaloreto_tsCGBPal2c1, gammaloreto_tsCGBPal2c2, gammaloreto_tsCGBPal2c3,
    gammaloreto_tsCGBPal3c0, gammaloreto_tsCGBPal3c1, gammaloreto_tsCGBPal3c2, gammaloreto_tsCGBPal3c3,
    gammaloreto_tsCGBPal4c0, gammaloreto_tsCGBPal4c1, gammaloreto_tsCGBPal4c2, gammaloreto_tsCGBPal4c3,
    gammaloreto_tsCGBPal5c0, gammaloreto_tsCGBPal5c1, gammaloreto_tsCGBPal5c2, gammaloreto_tsCGBPal5c3,
    gammaloreto_tsCGBPal6c0, gammaloreto_tsCGBPal6c1, gammaloreto_tsCGBPal6c2, gammaloreto_tsCGBPal6c3,
    gammaloreto_tsCGBPal7c0, gammaloreto_tsCGBPal7c1, gammaloreto_tsCGBPal7c2, gammaloreto_tsCGBPal7c3,
};

const UWORD lionfish_palettes[] = {
    lionfish_tsCGBPal0c0, lionfish_tsCGBPal0c1, lionfish_tsCGBPal0c2, lionfish_tsCGBPal0c3,
    lionfish_tsCGBPal1c0, lionfish_tsCGBPal1c1, lionfish_tsCGBPal1c2, lionfish_tsCGBPal1c3,
    lionfish_tsCGBPal2c0, lionfish_tsCGBPal2c1, lionfish_tsCGBPal2c2, lionfish_tsCGBPal2c3,
    lionfish_tsCGBPal3c0, lionfish_tsCGBPal3c1, lionfish_tsCGBPal3c2, lionfish_tsCGBPal3c3,
    lionfish_tsCGBPal4c0, lionfish_tsCGBPal4c1, lionfish_tsCGBPal4c2, lionfish_tsCGBPal4c3,
    lionfish_tsCGBPal5c0, lionfish_tsCGBPal5c1, lionfish_tsCGBPal5c2, lionfish_tsCGBPal5c3,
    lionfish_tsCGBPal6c0, lionfish_tsCGBPal6c1, lionfish_tsCGBPal6c2, lionfish_tsCGBPal6c3,
    lionfish_tsCGBPal7c0, lionfish_tsCGBPal7c1, lionfish_tsCGBPal7c2, lionfish_tsCGBPal7c3,
};


// --- RNG ---
void srand_gb(unsigned int seed) {
    rng_seed = seed;
}

int rand_gb(void) {
    rng_seed = rng_seed * 75 + 1; // simple LCG
    return rng_seed & 0x7FFF;     // return 0..32767
}


//--- SISTEMA DI SALVATAGGIO ---
void save_game(void) {
    ENABLE_RAM;
    SaveData* sd = (SaveData*)SAVE_ADDR;

    sd->magic = SAVE_MAGIC;
    sd->hunger = hunger;
    sd->algae = algae;
    sd->food = food;
    sd->medicine = medicine;
    sd->bubbles_currency = bubbles_currency;

    for (UINT8 i = 0; i < MAX_FISHES; i++) {
        sd->fishes[i] = fishes[i];
    }

    DISABLE_RAM;
}

void load_game(void) {
    ENABLE_RAM;
    SaveData* sd = (SaveData*)SAVE_ADDR;

    if (sd->magic == SAVE_MAGIC) {
        // dati validi → carico
        hunger = sd->hunger;
        algae = sd->algae;
        food = sd->food;
        medicine = sd->medicine;
        bubbles_currency = sd->bubbles_currency;

        for (UINT8 i = 0; i < MAX_FISHES; i++) {
            fishes[i] = sd->fishes[i];
        }
    } else {
        hunger = 100;
		last_hunger = 9999;

		algae = 0;
		last_algae = 9999;

		food = 10;
		last_food = 9999;

		medicine = 3;
		last_medicine = 9999;

		bubbles_currency = 0;
		last_bubbles_currency = 9999;
    }

    DISABLE_RAM;
}


// --- GESTIONE PRESSIONE TASTI ---
void wait_release(void) {
    // aspetta finché non vengono rilasciati tutti i tasti
    while (joypad() != 0) {
        wait_vbl_done();
    }
}

// --- GESTIONE VISIBILITA' SPRITES
void hide_all_sprites(void) {
    // Nasconde bolle
    for(UINT8 i = 0; i < MAX_BUBBLES; i++) {
        if(bubbles[i].active) {
            bubbles[i].active = 0;
            move_sprite(bubbles[i].sprite_id, 0, 0);
        }
    }

    // Nasconde pesci
	for(UINT8 i = 0; i < MAX_FISHES; i++) {
        UINT8 oam_base_index = FISH_SPRITE_ID_BASE + (i * 4);
        for(UINT8 j = 0; j < 4; j++) {
            move_sprite(oam_base_index + j, 0, 0);
        }
    }

	// Nasconde il cursore del menu
	cursor.active = 0;
	for(UINT8 i = 0; i < 4; i++) {
		move_sprite(CURSOR_SPRITE_ID_BASE + i, 0, 0);
	}
		
	// Nasconde anche il cursore del negozio
	move_sprite(6, 0, 0);
}

// --- GRAFICA PRINCIPALE ---
void init_gfx(void) {
    set_bkg_palette(0, 8, &aquarium_palettes[0]);
    set_bkg_data(38, 24, aquarium_ts);

    set_bkg_attributes(0, 0, aquariumWidth, aquariumHeight, aquariumPLN1);
    set_bkg_tiles(0, 0, aquariumWidth, aquariumHeight, aquariumPLN0);
	
    SHOW_BKG;
    DISPLAY_ON;
}


// --- AGGIORNAMENTO COLORE ACQUA ---
void update_water_color(void) {
    UINT8 water_pal = 0;    // palette per acqua
    UINT8 special_pal = 0;  // palette per scrigno/roccia

    // --- Aggiorna la palette dell'acqua e dei tile speciali ---
    if (algae > 50 && algae <= 90) {
        water_pal = 4;
        special_pal = 6;  // palette per scrigno/roccia
        set_bkg_palette(4, 1, &aquarium_palettes[16]);
        set_sprite_palette(0, 1, &bubbles_palettes[4]);
    }
    else if (algae > 90) {
        water_pal = 5;
        special_pal = 7;  // palette diversa per scrigno/roccia
        set_bkg_palette(5, 1, &aquarium_palettes[20]);
        set_sprite_palette(0, 1, &bubbles_palettes[8]);
    }
    else {
        water_pal = 0;
        special_pal = 0; // palette originale per scrigno/roccia
        set_bkg_palette(0, 1, &aquarium_palettes[0]);
        set_sprite_palette(0, 1, &bubbles_palettes[0]);
    }

    // --- Aggiorna tutte le tile con la palette acqua di base ---
    UINT8 attrs[aquariumWidth * aquariumHeight];
    for (UINT16 i = 0; i < aquariumWidth * aquariumHeight; i++) {
        attrs[i] = water_pal;
    }

    // --- Imposta palette speciale per scrigno e roccia ---
    // Scrigno 2x2
    for (UINT8 y = 13; y <= 14; y++) {
        for (UINT8 x = 3; x <= 4; x++) {
            attrs[y * aquariumWidth + x] = special_pal;
        }
    }

    // Roccia 2x2
    for (UINT8 y = 13; y <= 14; y++) {
        for (UINT8 x = 9; x <= 10; x++) {
            attrs[y * aquariumWidth + x] = special_pal;
        }
    }

    // --- Eventuali altre tile particolari nell'else ---
    if (algae <= 50) {
        // Sabbia
        for (UINT8 y = 15; y <= 16; y++) {
            for (UINT8 x = 1; x <= 13; x++) {
                attrs[y * aquariumWidth + x] = 1;
            }
        }

        // Scrigno e roccia tornano alle palette originali
        for (UINT8 y = 13; y <= 14; y++) {
            for (UINT8 x = 3; x <= 4; x++) attrs[y * aquariumWidth + x] = 3;
            for (UINT8 x = 9; x <= 10; x++) attrs[y * aquariumWidth + x] = 2;
        }
    }

    // --- Aggiorna la tilemap ---
    set_bkg_attributes(0, 0, aquariumWidth, aquariumHeight, attrs);
}

// --- UI ---
void print_window(UINT16 x, UINT16 y, const char* str) {
    UINT8 i = 0;
    UINT8 tile;
    
    while(str[i] != '\0') {
        char c = str[i];

        if(c >= '0' && c <= '9') {
            tile = 1 + (c - '0'); // 0 -> tile 1, 1 -> tile 2, ..., 9 -> tile 10 (0A)
        }
        else if(c >= 'A' && c <= 'Z') {
            tile = 11 + (c - 'A'); // 'A' -> tile 11 (0B), 'B' -> 12, ...
        }
        else if(c >= 'a' && c <= 'z') {
            tile = 11 + (c - 'a'); // minuscole mappate come maiuscole
        }
        else if(c == ' ') {
            tile = 0; // tile vuoto
        }
        else {
            tile = 0; // caratteri non gestiti come vuoto
        }

        set_win_tiles(x + i, y, 1, 1, &tile);
        i++;
    }
}

void print_window_number(UINT16 x, UINT16 y, UINT16 value) {
    char buffer[6];

    print_window(x, y, "     ");  

    sprintf(buffer, "%u", value);
    print_window(x, y, buffer);
}


void init_ui(void) {
    font_t min_font;
    
    font_init();
    min_font = font_load(font_min);
    font_set(min_font);
    
    SHOW_WIN;
    move_win(125, 1);
    
	update_ui();
}

void update_ui(void) {
	if (hunger != last_hunger) {
		print_window(0, 1, "H");
		print_window_number(2, 1, hunger);
		last_hunger = hunger;
	}

	if (algae != last_algae) {
		print_window(0, 2, "A");
		print_window_number(2, 2, algae);
		last_algae = algae;
	}
	
	if (food != last_food) {
		print_window(0, 5, "F");
		print_window_number(2, 5, food);
		last_food = food;
	}
	
	if (medicine != last_medicine) {
		print_window(0, 6, "M");
		print_window_number(2, 6, medicine);
		last_medicine = medicine;
	}
	
	if (bubbles_currency != last_bubbles_currency) {
		print_window(0, 7, "B");
		print_window_number(2, 7, bubbles_currency);
		last_bubbles_currency = bubbles_currency;
	}
}

// --- MENU ---
void init_menu_map(void) {
    set_win_data(62, 16, menu_ts);
    set_win_tiles(0, 10, menuWidth, menuHeight, menuPLN0);

    SHOW_WIN;
    move_win(123, 0);
}

void update_cursor_sprites(void) {
	if (!cursor.active) return;
	move_sprite(CURSOR_SPRITE_ID_BASE + 0, cursor.x,     cursor.y);
    move_sprite(CURSOR_SPRITE_ID_BASE + 1, cursor.x + 8, cursor.y);
    move_sprite(CURSOR_SPRITE_ID_BASE + 2, cursor.x,     cursor.y + 8);
    move_sprite(CURSOR_SPRITE_ID_BASE + 3, cursor.x + 8, cursor.y + 8);
}

void init_cursor(void) {
    // Carica palette e tile
    set_sprite_palette(7, 1, &cursor_palettes[0]);
    set_sprite_data(2, 4, cursor_ts);

    // Assegna tile e palette agli sprite
    for(UINT8 i = 0; i < 4; i++) {
        set_sprite_tile(CURSOR_SPRITE_ID_BASE + i, 2 + i);
        set_sprite_prop(CURSOR_SPRITE_ID_BASE + i, 7);
    }

    // Posizione iniziale
	cursor.active = 1;
    cursor.x = 124;
    cursor.y = 96;
	cursor.row = cursor.col = 0;
    update_cursor_sprites();

    SHOW_SPRITES;
}



// --- ANIMAZIONE ACQUA ---
void animate_water(void) {
    unsigned char temp[1];
    for(UINT8 x = 0; x < aquariumWidth; x++) {
        UINT8 tile = get_bkg_tile_xy(x, 1);
        switch(tile) {
            case 42: temp[0] = 43; set_bkg_tiles(x, 1, 1, 1, temp); break;
            case 43: temp[0] = 42; set_bkg_tiles(x, 1, 1, 1, temp); break;
            case 44: temp[0] = 45; set_bkg_tiles(x, 1, 1, 1, temp); break;
            case 45: temp[0] = 44; set_bkg_tiles(x, 1, 1, 1, temp); break;
            case 46: temp[0] = 47; set_bkg_tiles(x, 1, 1, 1, temp); break;
            case 47: temp[0] = 46; set_bkg_tiles(x, 1, 1, 1, temp); break;
        }
    }
}


// --- ANIMAZIONE SCRIGNO ---
void animate_treasure(void) {
    unsigned char temp[1];
    for(UINT8 x = 0; x < aquariumWidth; x++) {
        UINT8 tile = get_bkg_tile_xy(x, 13);
        switch(tile) {
            case 56: temp[0] = 58; set_bkg_tiles(x, 13, 1, 1, temp); break;
            case 58: temp[0] = 56; set_bkg_tiles(x, 13, 1, 1, temp); break;
            case 57: temp[0] = 59; set_bkg_tiles(x, 13, 1, 1, temp); break;
            case 59: temp[0] = 57; set_bkg_tiles(x, 13, 1, 1, temp); break;
        }
    }
}


// --- SISTEMA BOLLE ---
void init_bubbles_system(void) {
    set_sprite_palette(0, 1, &bubbles_palettes[0]);
    set_sprite_data(0, 2, bubbles_ts);

    for(UINT8 i = 0; i < MAX_BUBBLES; i++) {
        bubbles[i].sprite_id = BUBBLE_SPRITE_ID_BASE + i; 
        bubbles[i].active = 0;
        move_sprite(bubbles[i].sprite_id, 0, 0);
    }
    SHOW_SPRITES;
}

void spawn_bubble(UINT8 x, UINT8 y) {
    for(UINT8 i = 0; i < MAX_BUBBLES; i++) {
        if(!bubbles[i].active) {
            bubbles[i].active = 1;
            bubbles[i].x = x;
            bubbles[i].y = y;
            bubbles[i].tile = 0;
            set_sprite_tile(bubbles[i].sprite_id, bubbles[i].tile);
            move_sprite(bubbles[i].sprite_id, x, y);
            break;
        }
    }
}

void update_bubbles(void) {
    for(UINT8 i = 0; i < MAX_BUBBLES; i++) {
        if(bubbles[i].active) {
            bubbles[i].y--;
            move_sprite(bubbles[i].sprite_id, bubbles[i].x, bubbles[i].y);

            bubbles[i].tile ^= 1;
            set_sprite_tile(bubbles[i].sprite_id, bubbles[i].tile);

            if(bubbles[i].y < 25) {
                bubbles[i].active = 0;
                move_sprite(bubbles[i].sprite_id, 0, 0);
            }
        }
    }
}


// --- SISTEMA PESCI ---
void init_fish_system(void) {
    // Clownfish
    set_sprite_palette(1, 1, &clownfish_palettes[0]);
    set_sprite_data(FISH_STAGE0_TILE, 6, clownfish_ts);

    // Triggerfish
    set_sprite_palette(2, 1, &triggerfish_palettes[0]);
    set_sprite_data(FISH_STAGE0_TILE + 6, 6, triggerfish_ts); // evita sovrapposizione tile
	
	// Surgeonfish
    set_sprite_palette(3, 1, &surgeonfish_palettes[0]);
    set_sprite_data(FISH_STAGE0_TILE + 12, 6, surgeonfish_ts); // offset 12 per evitare conflitti
	
	// Gammaloreto
    set_sprite_palette(4, 1, &gammaloreto_palettes[0]);
    set_sprite_data(FISH_STAGE0_TILE + 18, 6, gammaloreto_ts); // offset 18 per evitare conflitti
	
	// Lionfish
    set_sprite_palette(5, 1, &lionfish_palettes[0]);
    set_sprite_data(FISH_STAGE0_TILE + 24, 6, lionfish_ts); // offset 24 per evitare conflitti
	
    for(UINT8 i=0; i<MAX_FISHES; i++) {
        fishes[i].active = 0;
    }
}

void draw_fish(Fish* f, UINT8 oam_index) {
    if (game_state != STATE_AQUARIUM) return;

    UINT8 prop = 0;       
    UINT8 flip = 0;       
    UINT8 pal = 1;        

    if(f->dx < 0) {
        prop |= S_FLIPX;
        flip = 1;
    }

    // palette e tile offset
    UINT8 tile_offset = 0;
    switch(f->type) {
        case FISH_CLOWN: pal = 1; tile_offset = 0; break;
        case FISH_TRIGGER: pal = 2; tile_offset = 6; break;
        case FISH_SURGEON: pal = 3; tile_offset = 12; break;
		case FISH_GAMMA: pal = 4; tile_offset = 18; break;
        case FISH_LION: pal = 5; tile_offset = 24; break;
    }

    prop |= pal;

    switch(f->stage) {
        case 0:
        case 1:
            {
                UINT8 stage_tile = (f->stage == 0) ? FISH_STAGE0_TILE : FISH_STAGE1_TILE;
                set_sprite_tile(oam_index, stage_tile + tile_offset + f->frame);
                set_sprite_prop(oam_index, prop);
                move_sprite(oam_index, f->x, f->y);

                for(UINT8 k = 1; k < 4; k++)
                    move_sprite(oam_index + k, 0, 0);
            }
            break;

        case 2:
            for(UINT8 i = 0; i < 4; i++) {
                set_sprite_tile(oam_index + i, FISH_STAGE2_TILE + tile_offset + i);
                set_sprite_prop(oam_index + i, prop);

                UINT8 dx = metasprite_offsets[i][0];
                if(flip) dx = 8 - dx;

                move_sprite(oam_index + i, f->x + dx, f->y + metasprite_offsets[i][1]);
            }
            break;
    }
}


void spawn_fish(UINT8 x, UINT8 y, FishType type) {
    for(UINT8 i = 0; i < MAX_FISHES; i++) {
        if(!fishes[i].active) {
            fishes[i].active = 1;
            fishes[i].x = x;
            fishes[i].y = y;
            fishes[i].stage = 0;
            fishes[i].frame = 0;
            fishes[i].dx = 1;
            fishes[i].dy = 0;
            fishes[i].type = type;
            last_fish_index = i;
            return;
        }
    }

    last_fish_index = (last_fish_index + 1) % MAX_FISHES;
    UINT8 i = last_fish_index;
    fishes[i].active = 1;
    fishes[i].x = x; 
    fishes[i].y = y;
    fishes[i].stage = 0; 
    fishes[i].frame = 0;
    fishes[i].dx = 1;
    fishes[i].dy = 0;
    fishes[i].type = type;
}


void evolve_fishes(void) {
    for(UINT8 i=0; i<MAX_FISHES; i++) {
        if(fishes[i].active && counter % 300 == 0 && fishes[i].stage < 2) {
            fishes[i].stage++;
        }
    }
}

void update_fishes(void) {
	if (game_state == STATE_AQUARIUM){
		
		for(UINT8 i = 0; i < MAX_FISHES; i++) {
			if(fishes[i].active) {
				// Invertiamo la direzione se al prossimo passo supera i limiti
				if(fishes[i].x + fishes[i].dx < 20 || fishes[i].x + fishes[i].dx > 100)
					fishes[i].dx = -fishes[i].dx;
				if(fishes[i].y + fishes[i].dy < 30 || fishes[i].y + fishes[i].dy > 120)
					fishes[i].dy = -fishes[i].dy;

				// Aggiorniamo la posizione
				fishes[i].x += fishes[i].dx;
				fishes[i].y += fishes[i].dy;

				// Calcola l'indice OAM di base per questo pesce
				UINT8 oam_base_index = FISH_SPRITE_ID_BASE + (i * 4); // Ogni pesce usa 4 sprite OAM

				// Disegniamo il pesce
				draw_fish(&fishes[i], oam_base_index);
			}
		}
	}
}
void randomize_fish_direction(Fish* f) {
    // dx = -1 o 1
    f->dx = (rand_gb() % 2) ? 1 : -1;
    
    // dy = -1, 0 o 1
    UINT8 r = rand_gb() % 3;
    if(r == 0) f->dy = -1;
    else if(r == 1) f->dy = 0;
    else f->dy = 1;
}

UINT8 get_active_fish_count(void) {
    UINT8 count = 0;
    for (UINT8 i = 0; i < MAX_FISHES; i++) {
        if (fishes[i].active) count++;
    }
    return count;
}
void check_hunger_kill_fish(void) {
    if (hunger <= 0) {
        // Conta quanti pesci attivi ci sono
        UINT8 active_count = get_active_fish_count();
        if (active_count == 0) return;

        // Cerca un pesce a caso tra quelli attivi
        UINT8 index;
        do {
            index = rand_gb() % MAX_FISHES;
        } while (!fishes[index].active);

        // "uccidi" il pesce scelto
        fishes[index].active = 0;

        // Nascondi i suoi sprite
        UINT8 oam_base_index = FISH_SPRITE_ID_BASE + (index * 4);
        for (UINT8 j = 0; j < 4; j++) {
            move_sprite(oam_base_index + j, 0, 0);
        }
		
		hunger = 50;
    }
}

// --- SISTEMA CIBO/MEDICINE ---
void use_food(void){
	if (food > 0){
		hunger = 100;
		food--;
	}
}

void use_medicine(void){
	if (medicine > 0){
		algae = 0;
		medicine--;
		update_water_color();
	}
}

// --- SISTEMA SHOP ---
void hide_window_area_full(void) {
    for(UINT8 y = 0; y < 18; y++) {
        for(UINT8 x = 0; x < 20; x++) {
            set_win_tiles(x, y, 1, 1, &empty_tile);
        }
    }
}


void init_shop_cursor(void) {
	set_sprite_palette(0, 1, &shop_cursor_palettes[0]);
    set_sprite_data(6, 1, shop_cursor_ts);   // carico la tile in VRAM index 6

    set_sprite_tile(6, 6);  // sprite ID usa tile 6
    set_sprite_prop(6, 0);  // palette 0

    shop_cursor.active = 1;
    shop_cursor.x = 26;
    shop_cursor.y = 72;
    shop_cursor.row = 0;

    move_sprite(6, shop_cursor.x, shop_cursor.y);
    SHOW_SPRITES;
}

void init_shop(void){
	set_bkg_palette(0, 7, &shop_palettes[0]);
    set_win_data(79, 33, shop_ts);
    set_win_tiles(0, 0, shopWidth, shopHeight, shopPLN0);
	
	move_win(7, 0);
	
	print_window(3,7,"FOOD");			print_window(16,7,"3");
	print_window(3,8,"MEDICINE");		print_window(16,8,"5");
	
	print_window(3,10,"CLOWN");			print_window(15,10,"100");
	print_window(3,11,"TRIGGER");		print_window(15,11,"200");
	print_window(3,12,"SURGEON");		print_window(15,12,"500");
	print_window(3,13,"GAMMALORETO");	print_window(15,13,"750");
	print_window(3,14,"LION");			print_window(15,14,"999");
	
	DISPLAY_ON;
	SHOW_WIN;
	
	hide_all_sprites();
	game_state = STATE_SHOP;
	
	init_shop_cursor();
	
}

void exit_shop(void) {
	hide_window_area_full();
	
	shop_cursor.active = 0;
	move_sprite(6, 0, 0); 
	 
    // Ripristina grafica acquario
	last_hunger = last_algae = last_food = last_medicine = last_bubbles_currency = 9999;
	init_ui();
	init_menu_map();
    init_gfx();
	init_bubbles_system();
	
    // Riattiva cursore del menu
    cursor.active = 1;
    update_cursor_sprites();
	
    // Riattiva bolle e pesci (non per forza spawn immediato, ma sblocchiamo la logica)
    game_state = STATE_AQUARIUM;
}


// --- MAIN LOOP ---
void main(void) {


    srand_gb(rng_seed); 
    
    init_ui();
    init_menu_map();
    init_gfx();
    init_cursor();
    
    init_bubbles_system();
    init_fish_system();
    
	load_game();

    while(1) {
        counter++;
                        
        UINT8 keys = joypad();
        UINT8 pressed = keys & ~prev_keys; 

        if (game_state == STATE_AQUARIUM) {
            update_ui();
			
			if (joypad() & J_START) {
				save_game();
				gotoxy(0,0);
				printf("SAVED");

				// Attendi ~3 secondi (180 frame a 60Hz)
				for (UINT8 i = 0; i < 180; i++) {
					wait_vbl_done();
				}

				// Cancella la scritta (sovrascrivendo con spazi bianchi)
				gotoxy(0,0);
				printf("     "); 
			}

		
            // --- MOVIMENTO MENU CURSOR ---
            if(cursor_move_delay == 0) {
                if(keys & J_LEFT)  if(cursor.x > 128){   cursor.x -= 24;    cursor.col--;}
                if(keys & J_RIGHT) if(cursor.x < 144){   cursor.x += 24;    cursor.col++;}
                if(keys & J_UP)    if(cursor.y > 104){   cursor.y -= 24;    cursor.row--;}
                if(keys & J_DOWN)  if(cursor.y < 110){   cursor.y += 24;    cursor.row++;}

                if(keys) cursor_move_delay = 24;
            }
            if(cursor_move_delay > 0) cursor_move_delay--;

            update_cursor_sprites(); 

            // --- SELEZIONE VOCI MENU ---
            if (pressed & J_A && action_cooldown == 0){
                if (cursor.row == 0 && cursor.col == 0){ 	hide_all_sprites(); init_shop();}
                if (cursor.row == 0 && cursor.col == 1) use_food();
                if (cursor.row == 1 && cursor.col == 0) use_medicine();
            }
            if (action_cooldown > 0) action_cooldown--;

            // --- GESTIONE RESET (tenere premuto 3s = 180 frame) ---
            if ((joypad() & J_A) && cursor.row == 1 && cursor.col == 1) {
                reset_hold_counter++;

                // ogni 60 frame = 1 secondo → stampa un punto
                if (reset_hold_counter % 60 == 0) {
                    gotoxy((reset_hold_counter / 60) - 1, 0); // posizione puntini in riga 0
                    printf("o");
                }

                if (reset_hold_counter >= 180) {
                    wait_release();
                    reset();
                }
            } else {
                if (reset_hold_counter > 0) {
                    // se molli A: cancella i puntini
                    gotoxy(0,0);
                    printf("   "); // sovrascrive i 3 puntini
                }
                reset_hold_counter = 0;
            }


            // --- LOGICA DI GIOCO ACQUARIO ---
			
			// --- AUMENTO VALUTA (PRESSIONE B) ---
			
			if (pressed & J_B && bubbles_currency < 999){
				bubbles_currency++;
			}
			
			// --- AGGIORNAMENTO ALGHE ---
			if (counter % 1000 == 0) {
				UINT8 fish_count = get_active_fish_count();
				
				if (fish_count > 0) {
					algae += fish_count;  // più pesci = più alghe
					update_water_color();
				}
			}
			
			// --- AGGIORNAMENTO FAME ---
			if (counter % 250 == 0 && get_active_fish_count() >= 1) {
				hunger--;
			}
			
			// --- ANIMAZIONI ---
            if(counter % 10 == 0) update_bubbles();
            if(counter % 30 == 0) animate_water();
            if(counter % 200 == 0) {
                if(spawn_if_even % 2 == 0) spawn_bubble(40, 110);
                animate_treasure();
                spawn_if_even++;
            }

			// --- CRESCITA PESCE ---
            if(counter % 800 == 0) evolve_fishes();

			// --- MOVIMENTO PESCE ---
            fish_timer++;
            if(fish_timer % 3 == 0) update_fishes();
            if(fish_timer % 500 == 0) {
                for(UINT8 i = 0; i < MAX_FISHES; i++) {
                    if(fishes[i].active) randomize_fish_direction(&fishes[i]);
                }
            }
			
			// --- STATI "GAME OVER" (CIBO E ALGHE) ---
			check_hunger_kill_fish();
			if (algae >= 100) reset();
			
        }else if (game_state == STATE_SHOP) {
	
            // --- MOVIMENTO SHOP CURSOR ---
            if(shop_cursor_move_delay == 0) {
                if(keys & J_UP) {
                    if(shop_cursor.row > 0) {
                        shop_cursor.row--;
                        if(shop_cursor.row != 1) {
                            shop_cursor.y -= 8; 
                        }else{
                            shop_cursor.y -= 16;    
                        }
                          
                        move_sprite(6, shop_cursor.x, shop_cursor.y);
                    }
                }
                if(keys & J_DOWN) {
                    if(shop_cursor.row < 6) { // adatta al numero di opzioni disponibili
                        shop_cursor.row++;
                        if(shop_cursor.row != 2) {
                            shop_cursor.y += 8; 
                        }else{
                            shop_cursor.y += 16;    
                        }
                        move_sprite(6, shop_cursor.x, shop_cursor.y);
                    }
                }

                if(keys) shop_cursor_move_delay = 12; // piccolo delay come nel menu
            }
            if(shop_cursor_move_delay > 0) shop_cursor_move_delay--;

            // --- SELEZIONE ITEM SHOP ---
            if(pressed & J_A) {
				// nella selezione item shop
				switch(shop_cursor.row) {
					case 0: if (bubbles_currency >= 3) {food++; bubbles_currency = bubbles_currency - 3; break; }
					case 1: if (bubbles_currency >= 5) {medicine++; bubbles_currency = bubbles_currency - 5; break; }
					case 2: if (bubbles_currency >=100) {spawn_fish(80, 80, FISH_CLOWN); bubbles_currency = bubbles_currency - 100; break; }
					case 3: if (bubbles_currency >=200) {spawn_fish(80, 80, FISH_TRIGGER); bubbles_currency = bubbles_currency - 200; break; }
					case 4: if (bubbles_currency >=500) {spawn_fish(80, 80, FISH_SURGEON); bubbles_currency = bubbles_currency - 500; break; }
					case 5: if (bubbles_currency >=750) {spawn_fish(80, 80, FISH_GAMMA); bubbles_currency = bubbles_currency - 750; break; }
					case 6: if (bubbles_currency >=999) {spawn_fish(80, 80, FISH_LION); bubbles_currency = bubbles_currency - 999; break; }
				}
				exit_shop();
			}

            // --- USCITA ---
            if (pressed & J_B) {
                exit_shop();
            }
        }

        prev_keys = keys;
        wait_vbl_done();
    }

}