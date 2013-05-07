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

#include <stdio.h>
#include "gfx/gfxfilter_d3d_customres.h"
#include "util/wgt2allg.h"
#include "device/mousew32.h"
#include "gfx/gfxfilterhelpers.h"
#ifdef WINDOWS_VERSION
#include <d3d9.h>
#endif

D3DGFXFilterCustomResolution::D3DGFXFilterCustomResolution(int custom_width, int custom_height, bool stretch_to_desktop_resolution) : GFXFilter() {
  this->custom_width = custom_width;
  this->custom_height = custom_height;
  this->stretch_to_desktop_resolution = stretch_to_desktop_resolution;
}

const char* D3DGFXFilterCustomResolution::Initialize(int width, int height, int colDepth) {
  mouseCallbackPtr = new CustomResolutionMouseGetPosCallbackImpl(this);
  msetcallback((CustomResolutionMouseGetPosCallbackImpl*)mouseCallbackPtr);
  if(stretch_to_desktop_resolution) {
    get_desktop_resolution(&screen_width, &screen_height);
    multiplierX = screen_width*1.0f / custom_width;
    multiplierY = screen_height*1.0f / custom_height;
    top = 0;
    left = 0;
  } else {
    screen_width = width;
    screen_height = height;
    multiplierX = 1;
    multiplierY = 1;
    top = 0;
    left = 0;
  }

  return NULL;
}

void D3DGFXFilterCustomResolution::UnInitialize() {
  msetcallback(NULL);
}

void D3DGFXFilterCustomResolution::AdjustPosition(int *x, int *y) {
  *x = (*x - left) / multiplierX;
  *y = (*y - top) / multiplierY;
}

void D3DGFXFilterCustomResolution::GetRealResolution(int *wid, int *hit) {
  *wid = screen_width;
  *hit = screen_height;
}

void D3DGFXFilterCustomResolution::SetMouseArea(int x1, int y1, int x2, int y2) {
  x1 *= multiplierX + left;
  y1 *= multiplierY + top;
  x2 *= multiplierX + left;
  y2 *= multiplierY + top;
  mgraphconfine(x1, y1, x2, y2);
}

void D3DGFXFilterCustomResolution::SetMousePosition(int x, int y) {
  msetgraphpos(x * multiplierX + left, y * multiplierY + top);
}

void D3DGFXFilterCustomResolution::SetMouseLimit(int x1, int y1, int x2, int y2) {
  x1 = x1 * multiplierX + (multiplierX - 1) + left;
  y1 = y1 * multiplierY + (multiplierY - 1) + top;
  x2 = x2 * multiplierX + (multiplierX - 1) + left;
  y2 = y2 * multiplierY + (multiplierY - 1) + top;
  msetcursorlimit(x1, y1, x2, y2);
}

const char* D3DGFXFilterCustomResolution::GetVersionBoxText() {
  return "D3DGFXFilterCustomResolution 1.0";
}

const char* D3DGFXFilterCustomResolution::GetFilterID() {
  return "D3DGFXFilterCustomResolution";
}

void D3DGFXFilterCustomResolution::SetSamplerStateForStandardSprite(void *direct3ddevice9)
{
#ifdef WINDOWS_VERSION
  IDirect3DDevice9* d3d9 = ((IDirect3DDevice9*)direct3ddevice9);
  d3d9->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
  d3d9->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
#endif
}

bool D3DGFXFilterCustomResolution::NeedToColourEdgeLines()
{
  return false;
}
