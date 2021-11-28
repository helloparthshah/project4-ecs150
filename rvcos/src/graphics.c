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

volatile uint8_t *BackgroundDataBuffer[5];
volatile uint8_t *LargeSpriteDataBuffer[64];
volatile uint8_t *SmallSpriteDataBuffer[128];

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
    UpcallParam = (void*)param;
    return RVCOS_STATUS_SUCCESS;
}

int nBg = 0;
int nLs = 0;
int nSs = 0;

TStatus RVCGraphicCreate(TGraphicType type, TGraphicIDRef gidref) {
  if (type == RVCOS_GRAPHIC_TYPE_FULL) {
    BackgroundControls[nBg].DPalette = 0;
    BackgroundControls[nBg].DXOffset = 0;
    BackgroundControls[nBg].DYOffset = 0;
    BackgroundControls[nBg].DZ = 0;
    // BackgroundDataBuffer[nBg] = malloc(512 * 288);
    RVCMemoryAllocate(512*288*sizeof(uint8_t),(void**)&BackgroundDataBuffer[nBg]);
    *gidref = nBg++;
  } else if (type == RVCOS_GRAPHIC_TYPE_LARGE) {
    LargeSpriteControls[nLs].DPalette = 0;
    LargeSpriteControls[nLs].DXOffset = 0;
    LargeSpriteControls[nLs].DYOffset = 0;
    LargeSpriteControls[nLs].DWidth = 0;
    LargeSpriteControls[nLs].DHeight = 0;
    RVCMemoryAllocate(64*64*sizeof(uint8_t),(void**)&LargeSpriteDataBuffer[nLs]);
    *gidref = nLs++ + 4;
  } else if (type == RVCOS_GRAPHIC_TYPE_SMALL) {
    SmallSpriteControls[nSs].DPalette = 0;
    SmallSpriteControls[nSs].DXOffset = 0;
    SmallSpriteControls[nSs].DYOffset = 0;
    SmallSpriteControls[nSs].DZ = 7;
    SmallSpriteControls[nSs].DWidth = 0;
    SmallSpriteControls[nSs].DHeight = 0;
    RVCMemoryAllocate(16*16*sizeof(uint8_t),(void**)&SmallSpriteDataBuffer[nSs]);
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
    BackgroundControls[gid].DPalette = pid;
    memcpy((void*)BackgroundData[gid], (void*)BackgroundDataBuffer[gid], 512*288);
  } else if (gid < 64 + 4) {
    LargeSpriteControls[gid - 4].DXOffset = 64 + pos->DXPosition;
    LargeSpriteControls[gid - 4].DYOffset = 64 + pos->DYPosition;
    LargeSpriteControls[gid - 4].DWidth = dim->DWidth-33;
    LargeSpriteControls[gid - 4].DHeight = dim->DHeight-33;
    LargeSpriteControls[gid - 4].DPalette = pid;
    memcpy((void*)LargeSpriteData[gid - 4], (void*)LargeSpriteDataBuffer[gid - 4], dim->DWidth*dim->DHeight);
  } else if (gid < 128 + 64 + 4) {
    SmallSpriteControls[gid - 68].DXOffset = 16 + pos->DXPosition;
    SmallSpriteControls[gid - 68].DYOffset = 16 + pos->DYPosition;
    SmallSpriteControls[gid - 68].DWidth = dim->DWidth-1;
    SmallSpriteControls[gid - 68].DHeight = dim->DHeight-1;
    SmallSpriteControls[gid - 68].DPalette = pid;
    SmallSpriteControls[gid - 68].DZ = pos->DZPosition;
    memcpy((void*)SmallSpriteData[gid - 68], (void*)SmallSpriteDataBuffer[gid - 68], dim->DWidth*dim->DHeight);
  }
  return RVCOS_STATUS_SUCCESS;
}

TStatus RVCGraphicDeactivate(TGraphicID gid) { //
  if (gid < 4) {
    BackgroundControls[gid].DXOffset = 0;
    BackgroundControls[gid].DYOffset = 0;
    BackgroundControls[gid].DZ = 0;
  } else if (gid < 64 + 4) {
    LargeSpriteControls[gid - 4].DXOffset = 0;
    LargeSpriteControls[gid - 4].DYOffset = 0;
  } else if (gid < 128 + 64 + 4) {
    SmallSpriteControls[gid - 68].DXOffset = 0;
    SmallSpriteControls[gid - 68].DYOffset = 0;
    SmallSpriteControls[gid - 68].DZ = 7;
  }
  return RVCOS_STATUS_SUCCESS;
}

void overlap(SGraphicPositionRef pos, SGraphicDimensionsRef dim, uint32_t gid) {
  int w=0,h=0;
  // check for overlap with the graphics
  if(gid<4){
    w=512;
    h=288;
  }else if(gid<64+4){
    w=64;
    h=64;
  }else if(gid<128+64+4){
    w=16;
    h=16;
  }

    if(pos->DXPosition+dim->DWidth>w){
      pos->DXPosition=w-dim->DWidth;
    }
    if(pos->DYPosition+dim->DHeight>h){
      pos->DYPosition=h-dim->DHeight;
    }
}

TStatus RVCGraphicDraw(TGraphicID gid, SGraphicPositionRef pos,
                       SGraphicDimensionsRef dim, TPaletteIndexRef src,
                       uint32_t srcwidth) {
  overlap(pos, dim, gid);
  if (gid < 4) {
    for(int i=0;i<dim->DHeight;i++){
      memcpy((void*)BackgroundDataBuffer[gid]+pos->DXPosition+pos->DYPosition*512 + i*512, src+i*srcwidth, dim->DWidth);
    }
  } else if (gid < 68) {
    for(int i=0;i<dim->DHeight;i++){
      memcpy((void*)LargeSpriteDataBuffer[gid - 4]+pos->DXPosition+(pos->DYPosition+i)*64,
             src+srcwidth*i, dim->DWidth);
    }
  } else if(gid < 128+64 + 4) {
    for(int i=0;i<dim->DWidth;i++){
      memcpy((void*)SmallSpriteDataBuffer[gid - 68] +pos->DXPosition+pos->DXPosition+(pos->DYPosition+i)*16, src+srcwidth*i, dim->DWidth);
    }
  }
  return RVCOS_STATUS_SUCCESS;
}

TStatus RVCPaletteCreate(TPaletteIDRef pidref) {
  *pidref = SpritePalettes.used;
  push_palette(&SpritePalettes, RVCOSPaletteDefaultColors);
  push_palette(&BackgroundPalettes, RVCOSPaletteDefaultColors);
  return RVCOS_STATUS_SUCCESS;
}

TStatus RVCPaletteDelete(TPaletteID pid) { //
  return RVCOS_STATUS_SUCCESS;
}

TStatus RVCPaletteUpdate(TPaletteID pid, SColorRef cols, TPaletteIndex offset,
                         uint32_t count) {
  memcpy((SColor *)SpritePalettes.Palettes[pid]+offset, cols,
         count * sizeof(SColor));
  memcpy((SColor *)BackgroundPalettes.Palettes[pid]+offset, cols,
         count * sizeof(SColor));
  return RVCOS_STATUS_SUCCESS;
}