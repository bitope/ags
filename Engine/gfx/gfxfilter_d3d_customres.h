//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-20xx others
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// http://www.opensource.org/licenses/artistic-license-2.0.php
//
//=============================================================================
//
// AGS specific color blending routines for transparency and tinting effects
//
//=============================================================================

#ifndef __AC_D3DGFXFILTERCUSTOMRESOLUTION_H
#define __AC_D3DGFXFILTERCUSTOMRESOLUTION_H

#include "gfx/gfxfilter.h"

struct CustomResolutionMouseGetPosCallbackImpl;

struct D3DGFXFilterCustomResolution : public GFXFilter {
protected:
  int custom_width;
  int custom_height;
  int screen_width;
  int screen_height;
  float top;
  float left;
  float multiplierX;
  float multiplierY;
  void *mouseCallbackPtr;

  char filterName[100];
  char filterID[15];
  bool stretch_to_desktop_resolution;

public:
  D3DGFXFilterCustomResolution(int custom_width, int custom_height, bool fullscreen);
  virtual const char* Initialize(int width, int height, int colDepth);
  virtual void UnInitialize();
  virtual void GetRealResolution(int *wid, int *hit);
  virtual void SetMouseArea(int x1, int y1, int x2, int y2);
  virtual void SetMouseLimit(int x1, int y1, int x2, int y2);
  virtual void SetMousePosition(int x, int y);
  virtual void AdjustPosition(int *x, int *y);
  virtual const char *GetVersionBoxText();
  virtual const char *GetFilterID();
  virtual void SetSamplerStateForStandardSprite(void *direct3ddevice9);
  virtual bool NeedToColourEdgeLines();
};


#endif // __AC_D3DGFXFILTERCUSTOMRESOLUTION_H
