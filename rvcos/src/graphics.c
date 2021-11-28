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

extern volatile TUpcallPointer UpcallPointer;
extern volatile void *UpcallParam;

TStatus RVCSetVideoUpcall(TThreadEntry upcall, void *param) { 
  UpcallPointer = (TUpcallPointer)upcall;
  UpcallParam = (void *)param;
  return RVCOS_STATUS_SUCCESS;
}

int nBg = 0;
int nLs = 0;
int nSs = 0;

TStatus RVCGraphicCreate(TGraphicType type, TGraphicIDRef gidref) {
  if (type == RVCOS_GRAPHIC_TYPE_FULL) {
    BackgroundControls[nBg].DPalette = 0;
    BackgroundControls[nBg].DXOffset = 512;
    BackgroundControls[nBg].DYOffset = 288;
    BackgroundControls[nBg].DZ = 0;
    *gidref = nBg++;
  } else if (type == RVCOS_GRAPHIC_TYPE_LARGE) {
    LargeSpriteControls[nLs].DPalette = 0;
    LargeSpriteControls[nLs].DXOffset = 64;
    LargeSpriteControls[nLs].DYOffset = 64;
    LargeSpriteControls[nLs].DWidth = 31;
    LargeSpriteControls[nLs].DHeight = 31;
    *gidref = nLs++ + 4;
  } else if (type == RVCOS_GRAPHIC_TYPE_SMALL) {
    SmallSpriteControls[nSs].DPalette = 0;
    SmallSpriteControls[nSs].DXOffset = 16;
    SmallSpriteControls[nSs].DYOffset = 16;
    SmallSpriteControls[nSs].DZ = 7;
    SmallSpriteControls[nSs].DWidth = 15;
    SmallSpriteControls[nSs].DHeight = 15;
    *gidref = nSs++ + 64 + 4;
  }
  return RVCOS_STATUS_SUCCESS;
}

TStatus RVCGraphicDelete(TGraphicID gid) { //
  return RVCOS_STATUS_SUCCESS;
}

TStatus RVCGraphicActivate(TGraphicID gid, SGraphicPositionRef pos,
                           SGraphicDimensionsRef dim, TPaletteID pid) {
  if (gid < 4) {
    BackgroundControls[gid].DXOffset = 512 + pos->DXPosition;
    BackgroundControls[gid].DYOffset = 288 + pos->DYPosition;
    BackgroundControls[gid].DZ = pos->DZPosition;
  } else if (gid < 64 + 4) {
    LargeSpriteControls[gid - 4].DXOffset = 64 + pos->DXPosition;
    LargeSpriteControls[gid - 4].DYOffset = 64 + pos->DYPosition;
  } else {
    SmallSpriteControls[gid - 68].DXOffset = 16 + pos->DXPosition;
    SmallSpriteControls[gid - 68].DYOffset = 16 + pos->DYPosition;
    SmallSpriteControls[gid - 68].DZ = pos->DZPosition;
  }
  return RVCOS_STATUS_SUCCESS;
}

TStatus RVCGraphicDeactivate(TGraphicID gid) { //
  return RVCOS_STATUS_SUCCESS;
}

void overlap(SGraphicPositionRef pos, SGraphicDimensionsRef dim) {
  // if (pos->DXPosition + dim->DXDimension > pos2->DXPosition &&
  //     pos->DYPosition + dim->DYDimension > pos2->DYPosition &&
  //     pos->DXPosition < pos2->DXPosition + dim2->DXDimension &&
  //     pos->DYPosition < pos2->DYPosition + dim2->DYDimension) {
  //   pos->DXPosition = pos2->DXPosition + dim2->DXDimension;
  //   pos->DYPosition = pos2->DYPosition + dim2->DYDimension;
  // }
}

TStatus RVCGraphicDraw(TGraphicID gid, SGraphicPositionRef pos,
                       SGraphicDimensionsRef dim, TPaletteIndexRef src,
                       uint32_t srcwidth) {
  writei(pos->DXPosition, 20);
  if (gid < 4) {
    // for(int i=0;i<288;i++){
    //   memcpy(BackgroundData[gid] + pos->DXPosition + i*512,
    //          src+srcwidth*i, srcwidth);
    // }
    memcpy((void *)BackgroundData[gid]+srcwidth*pos->DYPosition+pos->DXPosition, src, srcwidth);
  } else if (gid < 68) {
    for(int i=0;i<dim->DHeight;i++){
      memcpy(LargeSpriteData[gid - 4] + pos->DXPosition + i*64,
             src+srcwidth*i, srcwidth);
    }
    // memcpy((void *)LargeSpriteData[gid - 4]+dim->DWidth*pos->DYPosition+ pos->DXPosition, src, dim->DWidth * dim->DHeight);
  } else {
    for(int i=0;i<dim->DHeight;i++){
      memcpy(SmallSpriteData[gid - 68] + pos->DXPosition + i*16,
             src+srcwidth*i, srcwidth);
    }
    // memcpy((void *)SmallSpriteData[gid - 68]+dim->DWidth*pos->DYPosition+ pos->DXPosition, src, dim->DWidth * dim->DHeight);
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