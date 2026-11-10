#ifndef ART_H
#define ART_H
#include <gb/cgb.h>
#include "bank.h"
extern const unsigned char farm_crop_attr[];
extern const unsigned char farm_crop_tile[];
extern const palette_color_t farm_spr_pal[];
extern const unsigned char font_tiles[];
extern const unsigned char music_notes[];
extern const unsigned char npc_base[];
extern const unsigned char npc_body_pal[];
extern const unsigned char npc_head_pal[];
extern const unsigned char spr_tiles[];
extern const unsigned char area_bank[];
extern const unsigned int area_addr[];
#define brawl_attr ((const unsigned char *)bk_get(3, 0x47F8, 512))
#define brawl_bkg_pal ((const palette_color_t *)bk_get(3, 0x49F8, 32))
#define brawl_fighter_pal ((const palette_color_t *)bk_get(3, 0x4A18, 64))
#define brawl_map ((const unsigned char *)bk_get(3, 0x4A58, 512))
#define brawl_spark_pal ((const palette_color_t *)bk_get(3, 0x4C58, 8))
#define brawl_tiles ((const unsigned char *)bk_get(3, 0x4C60, 912))
#define hockey_attr ((const unsigned char *)bk_get(3, 0x4FF0, 360))
#define hockey_bkg_pal ((const palette_color_t *)bk_get(3, 0x5158, 16))
#define hockey_map ((const unsigned char *)bk_get(3, 0x5168, 360))
#define hockey_spr_pal ((const palette_color_t *)bk_get(3, 0x52D0, 16))
#define hockey_tiles ((const unsigned char *)bk_get(3, 0x52E0, 1232))
#define SP_P1 0
#define SP_P2 6
#define SP_PIG_A 12
#define SP_PIG_B 14
#define SP_PIG_C 16
#define SP_CHICKEN 18
#define SP_CHICKEN_PECK 19
#define SP_CROW 20
#define SP_CROW_FLY 21
#define SP_DROP 22
#define SP_BUBBLE 23
#define SP_BOWL 24
#define SP_ICON_CUKE 25
#define SP_ICON_HAY 26
#define SP_ICON_PEPPER 27
#define SP_ICON_LETTUCE 28
#define SP_ICON_CORN 29
#define SP_ICON_PIG 30
#define SP_SUN 31
#define SP_MOON 32
#define SP_MALLET 33
#define SP_PUCK 37
#define SP_CURSOR 38
#define SP_SPARK 39
#define SP_HEAD0 40
#define SP_HEAD1 42
#define SP_HEAD2 44
#define SP_HEAD3 46
#define SP_TORSO 48
#define SP_TORSO_PUNCH 50
#define SP_LEGS 52
#define SP_LEGS_WALK 54
#define SP_LEGS_KICK 56
#define SP_FIST 58
#define SP_BOOT 59
#define SP_NPC_PEARL 60
#define SP_NPC_HOBBS 66
#define SP_NPC_DAISY 72
#define SP_NPC_WENDELL 78
#define SP_NPC_GUS 84
#define SP_COIN 90
#define SPR_TILE_COUNT 91
#define FONT_TILE_COUNT 74
#define HOCKEY_TILE_COUNT 77
#define BRAWL_T_BAR0 176
#define BRAWL_T_BAR1 177
#define BRAWL_T_BAR2 178
#define BRAWL_T_BAR3 179
#define BRAWL_T_BAR4 180
#define BRAWL_T_BAR5 181
#define BRAWL_T_BAR6 182
#define BRAWL_T_BAR7 183
#define BRAWL_T_BAR8 184
#define BRAWL_T_BLANK 128
#define BRAWL_TILE_COUNT 57
#define MUSIC_STEPS 64
#define MUSIC_STEP_FRAMES 9
#define AREA_FARM 0
#define AREA_TRAILER 1
#define AREA_VILLAGE 2
#define AREA_STORE 3
#define AREA_LAKE 4
#define AREA_COUNT 5
#define HS_TALK 1
#define HS_ARCADE 2
#define HS_HOCKEY 3
#define HS_BED 4
#define HS_SHOP 5
#define R_PEARL 1
#define R_SHOP 2
#define R_DAISY 3
#define R_WENDELL 4
#define R_GUS 5
#define FLAG_INDOOR 1
#define T_HUD 128
#define T_HEART 129
#define T_HEART_EMPTY 130
#define T_BOX 131
#define DLG_SIGN_FARM 0
#define DLG_BOOKS 1
#define DLG_LOCKED 2
#define DLG_BOARD 3
#define DLG_WELL 4
#define DLG_CLOSED 5
#define DLG_CABIN 6
#define DLG_SIGN_LAKE 7
#define DLG_POND 8
#define DLG_SLEEP 9
#define DLG_PEARL_INTRO 10
#define DLG_PEARL_T1 11
#define DLG_PEARL_T2 12
#define DLG_PEARL_T3 13
#define DLG_PEARL_T4 14
#define DLG_PEARL_T5 15
#define DLG_PEARL_T6 16
#define DLG_PEARL_T7 17
#define DLG_HOBBS_HELLO 18
#define DLG_DAISY_ASK 19
#define DLG_DAISY_WAIT 20
#define DLG_DAISY_DONE 21
#define DLG_DAISY_AFTER 22
#define DLG_WENDELL_ASK 23
#define DLG_WENDELL_WAIT 24
#define DLG_WENDELL_DONE 25
#define DLG_WENDELL_AFTER 26
#define DLG_GUS_T1 27
#define DLG_GUS_T2 28
#define DLG_GUS_T3 29
#define DLG_GUS_T4 30
#define DLG_GUS_T5 31
#define DLG_COUNT 32
#define COVER_BANK 2
#define COVER_ADDR 0x4000
#define DIALOG_BANK 3
#define DIALOG_ADDR 0x4000
#define COVER_TEXT_PAL 1
#define ROM_BANKS 4
#endif
