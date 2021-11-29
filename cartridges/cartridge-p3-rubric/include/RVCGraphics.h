#ifndef __RVCGRAPHICS_H__
#define __RVCGRAPHICS_H__

#define MODE_GRAPHICS       1
#define MODE_TEXT           0

#define BG_WDTH             512
#define BG_HGHT             288
#define BG_SIZE             BG_WDTH * BG_HGHT

#define BG_MAX              5

#define FONT_DIM            8
#define NUM_FONT_DATA       256

#define TXT_WDTH            64
#define TXT_HGHT            36
#define TXT_SIZE            TXT_WDTH * TXT_HGHT

#define PALETTE_LENGTH      256

#define SPR_DIM_LG          64
#define SPR_NUM_LG          64
#define SPR_DIM_SM          16
#define SPR_NUM_SM          128

#define NUM_BG_PALETTES     4
#define NUM_SPR_PALETTES    4

typedef struct {
    unsigned        palette :2;
    unsigned        X_512   :10;
    unsigned        Y_288   :10;
    unsigned        Z       :3;
    unsigned        res     :7;
} background_controls_t;

typedef struct {
    unsigned        palette :2;
    unsigned        X_64    :10;
    unsigned        Y_64    :9;
    unsigned        W_33    :5;
    unsigned        H_33    :5;
    unsigned        res     :1;
} large_sprite_controls_t;

typedef struct {
    unsigned        palette :2;
    unsigned        X_16    :10;
    unsigned        Y_16    :9;
    unsigned        W_1     :4;
    unsigned        H_1     :4;
    unsigned        Z       :3;
} small_sprite_controls_t;

typedef struct {
    unsigned        mode    :1;
    unsigned        refresh :7;
    unsigned        res     :24;
} mode_controls_t;

typedef struct {
    uint8_t         rows[8];
} font_char_t;

typedef struct {
    uint8_t                     background_data[BG_MAX][BG_SIZE];
    uint8_t                     large_sprites[SPR_NUM_LG][SPR_DIM_LG * SPR_DIM_LG];
    uint8_t                     small_sprites[SPR_NUM_SM][SPR_DIM_SM * SPR_DIM_SM];
    uint32_t                    background_palettes[NUM_BG_PALETTES][PALETTE_LENGTH];
    uint32_t                    sprite_palettes[NUM_SPR_PALETTES][PALETTE_LENGTH];
    font_char_t                 font_data[NUM_FONT_DATA];
    uint8_t                     text_data[TXT_HGHT][TXT_WDTH];
    background_controls_t       background_controls[BG_MAX];
    large_sprite_controls_t     large_sprite_controls[SPR_NUM_LG];
    small_sprite_controls_t     small_sprite_controls[SPR_NUM_SM];
    mode_controls_t             mode_controls;
} fw_graphics_t;

#endif
