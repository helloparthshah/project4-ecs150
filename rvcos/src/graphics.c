#include "Deque.h"
#include "RVCOS.h"
#include <stdint.h>
#include <string.h>

typedef struct {
  uint32_t DPalette : 2;
  uint32_t DXOffset : 10;
  uint32_t DYOffset : 9;
  uint32_t DWidth : 5;
  uint32_t DHeight : 5;
  uint32_t DReserved : 1;
} SLargeSpriteControl, *SLargeSpriteControlRef;

typedef struct {
  uint32_t DPalette : 2;
  uint32_t DXOffset : 10;
  uint32_t DYOffset : 9;
  uint32_t DWidth : 4;
  uint32_t DHeight : 4;
  uint32_t DZ : 3;
} SSmallSpriteControl, *SSmallSpriteControlRef;

typedef struct {
  uint32_t DPalette : 2;
  uint32_t DXOffset : 10;
  uint32_t DYOffset : 10;
  uint32_t DZ : 3;
  uint32_t DReserved : 7;
} SBackgroundControl, *SBackgroundControlRef;

typedef struct {
  uint32_t DMode : 1;
  uint32_t DRefresh : 7;
  uint32_t DReserved : 24;
} SVideoControllerMode, *SVideoControllerModeRef;

volatile uint8_t *BackgroundData[5];
volatile uint8_t *LargeSpriteData[64];
volatile uint8_t *SmallSpriteData[128];

typedef struct {
  volatile SColor *Palettes[4];
  volatile uint32_t used;
} PaletteArray;

void push_palette(volatile PaletteArray *p, SColor *c) {
  memcpy((SColor *)p->Palettes[p->used], c, 256 * sizeof(SColor));
  p->used++;
}

volatile PaletteArray BackgroundPalettes;
volatile PaletteArray SpritePalettes;

volatile SBackgroundControl *BackgroundControls =
    (volatile SBackgroundControl *)0x500FF100;
volatile SLargeSpriteControl *LargeSpriteControls =
    (volatile SLargeSpriteControl *)0x500FF114;
volatile SSmallSpriteControl *SmallSpriteControls =
    (volatile SSmallSpriteControl *)0x500FF214;
volatile SVideoControllerMode *ModeControl =
    (volatile SVideoControllerMode *)0x500FF414;

extern SColor RVCOSPaletteDefaultColors[];

extern void write(const TTextCharacter *, uint32_t);
extern void writei(uint32_t, uint32_t);

void InitGraphics(void) {
  for (int Index = 0; Index < 4; Index++) {
    BackgroundPalettes.Palettes[Index] =
        (volatile SColor *)(0x500FC000 + 256 * sizeof(SColor) * Index);
    SpritePalettes.Palettes[Index] =
        (volatile SColor *)(0x500FD000 + 256 * sizeof(SColor) * Index);
  }
  for (int Index = 0; Index < 5; Index++) {
    BackgroundData[Index] =
        (volatile uint8_t *)(0x50000000 + 512 * 288 * Index);
  }
  for (int Index = 0; Index < 64; Index++) {
    LargeSpriteData[Index] = (volatile uint8_t *)(0x500B4000 + 64 * 64 * Index);
  }
  for (int Index = 0; Index < 128; Index++) {
    SmallSpriteData[Index] = (volatile uint8_t *)(0x500F4000 + 16 * 16 * Index);
  }
  //  Loading the default palette into position 0
  BackgroundPalettes.used = 0;
  SpritePalettes.used = 0;
  push_palette(&BackgroundPalettes, RVCOSPaletteDefaultColors);
  push_palette(&SpritePalettes, RVCOSPaletteDefaultColors);
  writei(BackgroundPalettes.Palettes[0][1].DRed, 5);
}

volatile TVideoMode currentVideoMode = RVCOS_VIDEO_MODE_TEXT;

TStatus RVCChangeVideoMode(TVideoMode mode) {
  if (mode == currentVideoMode) {
    return RVCOS_STATUS_SUCCESS;
  }
  ModeControl->DMode ^= 1;
  currentVideoMode = mode;
  return RVCOS_STATUS_SUCCESS;
}

TStatus RVCSetVideoUpcall(TThreadEntry upcall, void *param) { //
  return RVCOS_STATUS_SUCCESS;
}

int nGraphics = 0;

TStatus RVCGraphicCreate(TGraphicType type, TGraphicIDRef gidref) {
  if (type == RVCOS_GRAPHIC_TYPE_FULL) {
    BackgroundControls[nGraphics].DPalette = 0;
    BackgroundControls[nGraphics].DXOffset = 512;
    BackgroundControls[nGraphics].DYOffset = 288;
    BackgroundControls[nGraphics].DZ = 0;
    *gidref = nGraphics++;
  } else if (type == RVCOS_GRAPHIC_TYPE_LARGE) {
    LargeSpriteControls[nGraphics].DPalette = 0;
    LargeSpriteControls[nGraphics].DXOffset = 64;
    LargeSpriteControls[nGraphics].DYOffset = 64;
    LargeSpriteControls[nGraphics].DWidth = 31;
    LargeSpriteControls[nGraphics].DHeight = 31;
    *gidref = nGraphics++ + 4;
  } else if (type == RVCOS_GRAPHIC_TYPE_SMALL) {
    SmallSpriteControls[nGraphics].DPalette = 0;
    SmallSpriteControls[nGraphics].DXOffset = 16;
    SmallSpriteControls[nGraphics].DYOffset = 16;
    SmallSpriteControls[nGraphics].DZ = 7;
    SmallSpriteControls[nGraphics].DWidth = 15;
    SmallSpriteControls[nGraphics].DHeight = 15;
    *gidref = nGraphics++ + 64 + 4;
  }
  return RVCOS_STATUS_SUCCESS;
}

TStatus RVCGraphicDelete(TGraphicID gid) { //
  return RVCOS_STATUS_SUCCESS;
}

TStatus RVCGraphicActivate(TGraphicID gid, SGraphicPositionRef pos,
                           SGraphicDimensionsRef dim, TPaletteID pid) { //
  if (gid < 4) {
    BackgroundControls[gid].DXOffset += pos->DXPosition;
    BackgroundControls[gid].DYOffset += pos->DYPosition;
    BackgroundControls[gid].DZ = pos->DZPosition;
  } else if (gid < 64 + 4) {
    LargeSpriteControls[gid - 4].DXOffset += pos->DXPosition;
    LargeSpriteControls[gid - 4].DYOffset += pos->DYPosition;
  } else {
    SmallSpriteControls[gid - 68].DXOffset += pos->DXPosition;
    SmallSpriteControls[gid - 68].DYOffset += pos->DYPosition;
    SmallSpriteControls[gid - 68].DZ = pos->DZPosition;
  }
  return RVCOS_STATUS_SUCCESS;
}

TStatus RVCGraphicDeactivate(TGraphicID gid) { //
  return RVCOS_STATUS_SUCCESS;
}

TStatus RVCGraphicDraw(TGraphicID gid, SGraphicPositionRef pos,
                       SGraphicDimensionsRef dim, TPaletteIndexRef src,
                       uint32_t srcwidth) {
  writei(srcwidth, 20);
  if (gid < 4) {
    BackgroundControls[gid].DXOffset = 512 + pos->DXPosition;
    BackgroundControls[gid].DYOffset = 288 + pos->DYPosition;
    BackgroundControls[gid].DZ = pos->DZPosition;
    // BackgroundControls->DPalette = *src;
    memcpy((void *)BackgroundData[gid], src, srcwidth);
  } else if (gid < 68) {
    LargeSpriteControls[gid - 4].DXOffset = dim->DWidth + pos->DXPosition;
    LargeSpriteControls[gid - 4].DYOffset += dim->DHeight + pos->DYPosition;
    memcpy((void *)LargeSpriteData[gid - 4], src, dim->DWidth * dim->DHeight);
  } else {
    SmallSpriteControls[gid - 68].DXOffset = dim->DWidth + pos->DXPosition;
    SmallSpriteControls[gid - 68].DYOffset = dim->DHeight + pos->DYPosition;
    SmallSpriteControls[gid - 68].DZ = pos->DZPosition;
    memcpy((void *)SmallSpriteData[gid - 68], src, dim->DWidth * dim->DHeight);
  }
  return RVCOS_STATUS_SUCCESS;
}

TStatus RVCPaletteCreate(TPaletteIDRef pidref) {
  *pidref = SpritePalettes.used;
  push_palette(&SpritePalettes, RVCOSPaletteDefaultColors);
  return RVCOS_STATUS_SUCCESS;
}

TStatus RVCPaletteDelete(TPaletteID pid) { //
  return RVCOS_STATUS_SUCCESS;
}

TStatus RVCPaletteUpdate(TPaletteID pid, SColorRef cols, TPaletteIndex offset,
                         uint32_t count) {
  memcpy((SColor *)SpritePalettes.Palettes[pid], cols + offset,
         count * sizeof(SColor));
  return RVCOS_STATUS_SUCCESS;
}