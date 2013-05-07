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
// Graphics initialization
//

#include "main/mainheader.h"
#include "gfx/ali3d.h"
#include "ac/common.h"
#include "ac/display.h"
#include "ac/draw.h"
#include "ac/walkbehind.h"
#include "debug/debug_log.h"
#include "debug/debugger.h"
#include "debug/out.h"
#include "font/fonts.h"
#include "game/game_objects.h"
#include "gui/guiinv.h"
#include "gui/guimain.h"
#include "main/graphics_mode.h"
#include "platform/base/agsplatformdriver.h"
#include "gfx/graphicsdriver.h"
#include "gfx/bitmap.h"

using AGS::Common::Bitmap;
namespace BitmapHelper = AGS::Common::BitmapHelper;
namespace Out = AGS::Common::Out;

extern int proper_exit;
extern GUIMain*guis;
extern int psp_gfx_renderer; // defined in ali3dogl
extern WalkBehindMethodEnum walkBehindMethod;
extern DynamicArray<GUIInv> guiinv;
extern int numguiinv;
extern int scrnwid,scrnhit;
extern int current_screen_resolution_multiplier;
extern char force_gfxfilter[50];
extern int force_letterbox;
extern AGSPlatformDriver *platform;
extern int force_16bit;
extern IGraphicsDriver *gfxDriver;
extern int final_scrn_wid,final_scrn_hit,final_col_dep;
extern volatile int timerloop;
extern IDriverDependantBitmap *blankImage;
extern IDriverDependantBitmap *blankSidebarImage;
extern Bitmap *_old_screen;
extern Bitmap *_sub_screen;
extern int _places_r, _places_g, _places_b;

int initasx,initasy;
int firstDepth, secondDepth;

// set to 0 once successful
int working_gfx_mode_status = -1;
int debug_15bit_mode = 0, debug_24bit_mode = 0;
int convert_16bit_bgr = 0;

int ff; // whatever!

int adjust_pixel_size_for_loaded_data(int size, int filever)
{
    if (filever < kGameVersion_300)
    {
        return multiply_up_coordinate(size);
    }
    return size;
}

void adjust_pixel_sizes_for_loaded_data(int *x, int *y, int filever)
{
    x[0] = adjust_pixel_size_for_loaded_data(x[0], filever);
    y[0] = adjust_pixel_size_for_loaded_data(y[0], filever);
}

void adjust_sizes_for_resolution(int filever)
{
    int ee;
    for (ee = 0; ee < game.MouseCursorCount; ee++) 
    {
        game.MouseCursors[ee].hotx = adjust_pixel_size_for_loaded_data(game.MouseCursors[ee].hotx, filever);
        game.MouseCursors[ee].hoty = adjust_pixel_size_for_loaded_data(game.MouseCursors[ee].hoty, filever);
    }

    for (ee = 0; ee < game.InvItemCount; ee++) 
    {
        adjust_pixel_sizes_for_loaded_data(&game.InventoryItems[ee].hotx, &game.InventoryItems[ee].hoty, filever);
    }

    for (ee = 0; ee < game.GuiCount; ee++) 
    {
        GUIMain*cgp=&guis[ee];
        adjust_pixel_sizes_for_loaded_data(&cgp->x, &cgp->y, filever);
        if (cgp->wid < 1)
            cgp->wid = 1;
        if (cgp->hit < 1)
            cgp->hit = 1;
        // Temp fix for older games
        if (cgp->wid == BASEWIDTH - 1)
            cgp->wid = BASEWIDTH;

        adjust_pixel_sizes_for_loaded_data(&cgp->wid, &cgp->hit, filever);

        cgp->popupyp = adjust_pixel_size_for_loaded_data(cgp->popupyp, filever);

        for (ff = 0; ff < cgp->numobjs; ff++) 
        {
            adjust_pixel_sizes_for_loaded_data(&cgp->objs[ff]->x, &cgp->objs[ff]->y, filever);
            adjust_pixel_sizes_for_loaded_data(&cgp->objs[ff]->wid, &cgp->objs[ff]->hit, filever);
            cgp->objs[ff]->activated=0;
        }
    }

    if ((filever >= 37) && (game.Options[OPT_NATIVECOORDINATES] == 0) &&
        (game.DefaultResolution > 2))
    {
        // New 3.1 format game file, but with Use Native Coordinates off

        for (ee = 0; ee < game.CharacterCount; ee++) 
        {
            game.Characters[ee].x /= 2;
            game.Characters[ee].y /= 2;
        }

        for (ee = 0; ee < numguiinv; ee++)
        {
            guiinv[ee].itemWidth /= 2;
            guiinv[ee].itemHeight /= 2;
        }
    }

}

void engine_init_screen_settings()
{
    Out::FPrint("Initializing screen settings");

    // default shifts for how we store the sprite data

#if defined(PSP_VERSION)
    // PSP: Switch b<>r for 15/16 bit.
    _rgb_r_shift_32 = 16;
    _rgb_g_shift_32 = 8;
    _rgb_b_shift_32 = 0;
    _rgb_b_shift_16 = 11;
    _rgb_g_shift_16 = 5;
    _rgb_r_shift_16 = 0;
    _rgb_b_shift_15 = 10;
    _rgb_g_shift_15 = 5;
    _rgb_r_shift_15 = 0;
#else
    _rgb_r_shift_32 = 16;
    _rgb_g_shift_32 = 8;
    _rgb_b_shift_32 = 0;
    _rgb_r_shift_16 = 11;
    _rgb_g_shift_16 = 5;
    _rgb_b_shift_16 = 0;
    _rgb_r_shift_15 = 10;
    _rgb_g_shift_15 = 5;
    _rgb_b_shift_15 = 0;
#endif

    usetup.BaseWidth = 320;
    usetup.BaseHeight = 200;

    if (game.DefaultResolution >= 5)
    {
        if (game.DefaultResolution >= 6)
        {
            // 1024x768
            usetup.BaseWidth = 512;
            usetup.BaseHeight = 384;
        }
        else
        {
            // 800x600
            usetup.BaseWidth = 400;
            usetup.BaseHeight = 300;
        }
        // don't allow letterbox mode
        game.Options[OPT_LETTERBOX] = 0;
        force_letterbox = 0;
        scrnwid = usetup.BaseWidth * 2;
        scrnhit = usetup.BaseHeight * 2;
        wtext_multiply = 2;
    }
    else if ((game.DefaultResolution == 4) ||
        (game.DefaultResolution == 3))
    {
        scrnwid = 640;
        scrnhit = 400;
        wtext_multiply = 2;
    }
    else if ((game.DefaultResolution == 2) ||
        (game.DefaultResolution == 1))
    {
        scrnwid = 320;
        scrnhit = 200;
        wtext_multiply = 1;
    }
    else
    {
        scrnwid = usetup.BaseWidth;
        scrnhit = usetup.BaseHeight;
        wtext_multiply = 1;
    }

    usetup.TextHeight = wgetfontheight(0) + 1;

    vesa_xres=scrnwid; vesa_yres=scrnhit;
    //scrnwto=scrnwid-1; scrnhto=scrnhit-1;
    current_screen_resolution_multiplier = scrnwid * 1.0f / BASEWIDTH;

	if (game.Options[OPT_NATIVECOORDINATES] && (game.DefaultResolution == 7) && usetup.CustomHeight>0 && usetup.CustomWidth>0)
	{
		current_screen_resolution_multiplier = 2;
	}

	if ((game.DefaultResolution > 2) && (game.DefaultResolution!=7) &&
        (game.Options[OPT_NATIVECOORDINATES]))
    {
        usetup.BaseWidth *= 2;
        usetup.BaseHeight *= 2;
    }

    initasx=scrnwid,initasy=scrnhit;
    if (scrnwid==960) { initasx=1024; initasy=768; }

    // save this setting so we only do 640x480 full-screen if they want it
    usetup.WantLetterbox = game.Options[OPT_LETTERBOX] != 0;

    if (force_letterbox > 0)
        game.Options[OPT_LETTERBOX] = 1;

    // don't allow them to force a 256-col game to hi-color
    if (game.ColorDepth < 2)
        usetup.ForceHicolorMode = false;

    firstDepth = 8, secondDepth = 8;
    if ((game.ColorDepth == 2) || (force_16bit) || (usetup.ForceHicolorMode)) {
        firstDepth = 16;
        secondDepth = 15;
    }
    else if (game.ColorDepth > 2) {
        firstDepth = 32;
        secondDepth = 24;
    }

    adjust_sizes_for_resolution(loaded_game_file_version);
}

int initialize_graphics_filter(const char *filterID, int width, int height, int colDepth, bool stretch_to_desktop_resolution)
{
	if(game.DefaultResolution==7 && usetup.CustomWidth>0 && usetup.CustomHeight>0) {
		if (stricmp(usetup.GfxDriverID, "D3D9") == 0)
		{
		  filter = get_d3d_custom_resolution_filter(usetup.CustomWidth, usetup.CustomHeight, stretch_to_desktop_resolution);
		}
		else
		{
		  filter = get_allegro_custom_resolution_filter(usetup.CustomWidth, usetup.CustomHeight);
		}

		filter->Initialize(width, height, colDepth);

		return 0;
	  }

    int idx = 0;
    GFXFilter **filterList;

    if (stricmp(usetup.GfxDriverID, "D3D9") == 0)
    {
        filterList = get_d3d_gfx_filter_list(false);
    }
    else
    {
        filterList = get_allegro_gfx_filter_list(false);
    }

    // by default, select No Filter
    filter = filterList[0];

    GFXFilter *thisFilter = filterList[idx];
    while (thisFilter != NULL) {

        if ((filterID != NULL) &&
            (strcmp(thisFilter->GetFilterID(), filterID) == 0))
            filter = thisFilter;
        else if (idx > 0)
            delete thisFilter;

        idx++;
        thisFilter = filterList[idx];
    }

    const char *filterError = filter->Initialize(width, height, colDepth);
    if (filterError != NULL) {
        proper_exit = 1;
        platform->DisplayAlert("Unable to initialize the graphics filter. It returned the following error:\n'%s'\n\nTry running Setup and selecting a different graphics filter.", filterError);
        return -1;
    }

    return 0;
}

int engine_init_gfx_filters()
{
    Out::FPrint("Init gfx filters");

    const char *gfxfilter;

    if (force_gfxfilter[0]) {
        gfxfilter = force_gfxfilter;
    }
    else if (!usetup.GfxFilterID.IsEmpty()) {
        gfxfilter = usetup.GfxFilterID;
    }
#if defined (WINDOWS_VERSION) || defined (LINUX_VERSION)
    else {
        int desktopWidth, desktopHeight;
        if (get_desktop_resolution(&desktopWidth, &desktopHeight) == 0)
        {
            if (usetup.Windowed > 0)
                desktopHeight -= 100;

            // calculate the correct game height when in letterbox mode
            int gameHeight = initasy;
            if (game.Options[OPT_LETTERBOX])
                gameHeight = (gameHeight * 12) / 10;

            int xratio = desktopWidth / initasx;
            int yratio = desktopHeight / gameHeight;
            int min_ratio = xratio < yratio ? xratio : yratio;

            if (min_ratio > 1)
            {
                if (min_ratio > 8)
                    min_ratio = 8;
                char filterID[12];
                sprintf(filterID, "StdScale%d", min_ratio);
                gfxfilter = filterID;
            }
        }
        else
        {
            Out::FPrint("Automatic scaling: disabled (unable to obtain desktop resolution)");
        }
    }
#endif

    if (initialize_graphics_filter(gfxfilter, initasx, initasy, firstDepth, false))
    {
        return EXIT_NORMAL;
    }

    return RETURN_CONTINUE;
}

void create_gfx_driver() 
{
#ifdef WINDOWS_VERSION
    if (stricmp(usetup.GfxDriverID, "D3D9") == 0)
        gfxDriver = GetD3DGraphicsDriver(filter);
    else
#endif
    {
#if defined(IOS_VERSION) || defined(ANDROID_VERSION) || defined(WINDOWS_VERSION)
        if ((psp_gfx_renderer > 0) && (game.ColorDepth != 1))
            gfxDriver = GetOGLGraphicsDriver(filter);
        else
#endif
            gfxDriver = GetSoftwareGraphicsDriver(filter);
    }

    gfxDriver->SetCallbackOnInit(GfxDriverOnInitCallback);
    gfxDriver->SetTintMethod(TintReColourise);
}

int init_gfx_mode(int wid,int hit,int cdep) {

    // a mode has already been initialized, so abort
    if (working_gfx_mode_status == 0) return 0;

    if (debug_15bit_mode)
        cdep = 15;
    else if (debug_24bit_mode)
        cdep = 24;

    Out::FPrint("Attempt to switch gfx mode to %d x %d (%d-bit)", wid, hit, cdep);

    if (usetup.RefreshRate >= 50)
        request_refresh_rate(usetup.RefreshRate);

    final_scrn_wid = wid;
    final_scrn_hit = hit;
    final_col_dep = cdep;

    if (game.ColorDepth == 1) {
        final_col_dep = 8;
    }
    else {
        set_color_depth(cdep);
    }

    working_gfx_mode_status = (gfxDriver->Init(wid, hit, final_col_dep, usetup.Windowed > 0, &timerloop) ? 0 : -1);

    if (working_gfx_mode_status == 0) 
        Out::FPrint("Succeeded. Using gfx mode %d x %d (%d-bit)", wid, hit, final_col_dep);
    else
        Out::FPrint("Failed, resolution not supported");

    if ((working_gfx_mode_status < 0) && (usetup.Windowed > 0) && (editor_debugging_enabled == 0)) {
        usetup.Windowed ++;
        if (usetup.Windowed > 2) usetup.Windowed = 0;
        return init_gfx_mode(wid,hit,cdep);
    }
    return working_gfx_mode_status;    
}

int try_widescreen_bordered_graphics_mode_if_appropriate(int initasx, int initasy, int firstDepth)
{
    if (working_gfx_mode_status == 0) return 0;
    if (usetup.EnableSideBorders == 0)
    {
        Out::FPrint("Widescreen side borders: disabled in Setup");
        return 1;
    }
    if (usetup.Windowed > 0)
    {
        Out::FPrint("Widescreen side borders: disabled (windowed mode)");
        return 1;
    }

    int failed = 1;
    int desktopWidth, desktopHeight;
    if (get_desktop_resolution(&desktopWidth, &desktopHeight) == 0)
    {
        int gameHeight = initasy;

        int screenRatio = (desktopWidth * 1000) / desktopHeight;
        int gameRatio = (initasx * 1000) / gameHeight;
        // 1250 = 1280x1024 
        // 1333 = 640x480, 800x600, 1024x768, 1152x864, 1280x960
        // 1600 = 640x400, 960x600, 1280x800, 1680x1050
        // 1666 = 1280x768

        Out::FPrint("Widescreen side borders: game resolution: %d x %d; desktop resolution: %d x %d", initasx, gameHeight, desktopWidth, desktopHeight);

        if ((screenRatio > 1500) && (gameRatio < 1500))
        {
            int tryWidth = (initasx * screenRatio) / gameRatio;
            int supportedRes = gfxDriver->FindSupportedResolutionWidth(tryWidth, gameHeight, firstDepth, 110);
            if (supportedRes > 0)
            {
                tryWidth = supportedRes;
                Out::FPrint("Widescreen side borders: enabled, attempting resolution %d x %d", tryWidth, gameHeight);
            }
            else
            {
                Out::FPrint("Widescreen side borders: gfx card does not support suitable resolution. will attempt %d x %d anyway", tryWidth, gameHeight);
            }
            failed = init_gfx_mode(tryWidth, gameHeight, firstDepth);
        }
        else
        {
            Out::FPrint("Widescreen side borders: disabled (not necessary, game and desktop aspect ratios match)", initasx, gameHeight, desktopWidth, desktopHeight);
        }
    }
    else 
    {
        Out::FPrint("Widescreen side borders: disabled (unable to obtain desktop resolution)");
    }
    return failed;
}

int switch_to_graphics_mode(int initasx, int initasy, int scrnwid, int scrnhit, int firstDepth, int secondDepth) 
{
    int failed;
    int initasyLetterbox = (initasy * 12) / 10;

    // first of all, try 16-bit normal then letterboxed
    if (game.Options[OPT_LETTERBOX] == 0) 
    {
        failed = try_widescreen_bordered_graphics_mode_if_appropriate(initasx, initasy, firstDepth);
        failed = init_gfx_mode(initasx,initasy, firstDepth);
    }
    failed = try_widescreen_bordered_graphics_mode_if_appropriate(initasx, initasyLetterbox, firstDepth);
    failed = init_gfx_mode(initasx, initasyLetterbox, firstDepth);

    if (secondDepth != firstDepth) {
        // now, try 15-bit normal then letterboxed
        if (game.Options[OPT_LETTERBOX] == 0) 
        {
            failed = try_widescreen_bordered_graphics_mode_if_appropriate(initasx, initasy, secondDepth);
            failed = init_gfx_mode(initasx,initasy, secondDepth);
        }
        failed = try_widescreen_bordered_graphics_mode_if_appropriate(initasx, initasyLetterbox, secondDepth);
        failed = init_gfx_mode(initasx, initasyLetterbox, secondDepth);
    }

    if ((scrnwid != initasx) || (scrnhit != initasy))
    {
        // now, try the original resolution at 16 then 15 bit
        failed = init_gfx_mode(scrnwid,scrnhit,firstDepth);
        failed = init_gfx_mode(scrnwid,scrnhit, secondDepth);
    }

    if (failed)
        return -1;

    return 0;
}

void engine_init_gfx_driver()
{
    Out::FPrint("Init gfx driver");

    create_gfx_driver();
}

int engine_init_graphics_mode()
{
    Out::FPrint("Switching to graphics mode");

	bool errorAndExit = false;
    if (switch_to_graphics_mode(initasx, initasy, scrnwid, scrnhit, firstDepth, secondDepth))
    {
		delete filter;
		if (initialize_graphics_filter(usetup.GfxFilterID, initasx, initasy, firstDepth, true))
		{
		  errorAndExit = true;
		}
		else if (switch_to_graphics_mode(initasx, initasy, scrnwid, scrnhit, firstDepth, secondDepth))
		{
		  errorAndExit = true;

        if (((usetup.GfxFilterID.IsEmpty()) || 
            (stricmp(usetup.GfxFilterID, "none") == 0)) &&
            (scrnwid == 320))
        {
            // If the game is 320x200 and no filter is being used, try using a 2x
            // filter automatically since many gfx drivers don't suport 320x200.
            Out::FPrint("320x200 not supported, trying with 2x filter");
            delete filter;

			if (!initialize_graphics_filter("StdScale2", initasx, initasy, firstDepth, false)) 
            {

	            create_gfx_driver();

	            if (!switch_to_graphics_mode(initasx, initasy, scrnwid, scrnhit, firstDepth, secondDepth))
	            {
	                errorAndExit = false;
	            }
			}
		}
	}

        if (errorAndExit)
        {
            proper_exit=1;
            platform->FinishedUsingGraphicsMode();

            // make sure the error message displays the true resolution
            if (game.Options[OPT_LETTERBOX])
                initasy = (initasy * 12) / 10;

            if (filter != NULL)
                filter->GetRealResolution(&initasx, &initasy);

            platform->DisplayAlert("There was a problem initializing graphics mode %d x %d (%d-bit).\n"
                "(Problem: '%s')\n"
                "Try to correct the problem, or seek help from the AGS homepage.\n"
                "\nPossible causes:\n* your graphics card drivers do not support this resolution. "
                "Run the game setup program and try the other resolution.\n"
                "* the graphics driver you have selected does not work. Try switching between Direct3D and DirectDraw.\n"
                "* the graphics filter you have selected does not work. Try another filter.",
                initasx, initasy, firstDepth, allegro_error);
            return EXIT_NORMAL;
        }
    }

    return RETURN_CONTINUE;
}

void CreateBlankImage()
{
    // this is the first time that we try to use the graphics driver,
    // so it's the most likey place for a crash
    try
    {
        Bitmap *blank = BitmapHelper::CreateBitmap(16, 16, final_col_dep);
        blank = gfxDriver->ConvertBitmapToSupportedColourDepth(blank);
        blank->Clear();
        blankImage = gfxDriver->CreateDDBFromBitmap(blank, false, true);
        blankSidebarImage = gfxDriver->CreateDDBFromBitmap(blank, false, true);
        delete blank;
    }
    catch (Ali3DException gfxException)
    {
        quit((char*)gfxException._message);
    }

}

void engine_post_init_gfx_driver()
{
    //screen = _filter->ScreenInitialized(screen, final_scrn_wid, final_scrn_hit);
	_old_screen = BitmapHelper::GetScreenBitmap();

    if (gfxDriver->HasAcceleratedStretchAndFlip()) 
    {
        walkBehindMethod = DrawAsSeparateSprite;

        CreateBlankImage();
    }
}

void engine_prepare_screen()
{
    Out::FPrint("Preparing graphics mode screen");

    if ((final_scrn_hit != scrnhit) || (final_scrn_wid != scrnwid)) {
        initasx = final_scrn_wid;
        initasy = final_scrn_hit;
        _old_screen->Clear();
		BitmapHelper::SetScreenBitmap(
			BitmapHelper::CreateSubBitmap(_old_screen, RectWH(initasx / 2 - scrnwid / 2, initasy/2-scrnhit/2, scrnwid, scrnhit))
			);
		Bitmap *screen_bmp = BitmapHelper::GetScreenBitmap();
        _sub_screen=screen_bmp;

        scrnhit = screen_bmp->GetHeight();
        vesa_yres = screen_bmp->GetHeight();
        scrnwid = screen_bmp->GetWidth();
        vesa_xres = screen_bmp->GetWidth();
		gfxDriver->SetMemoryBackBuffer(screen_bmp);

        Out::FPrint("Screen resolution: %d x %d; game resolution %d x %d", _old_screen->GetWidth(), _old_screen->GetHeight(), scrnwid, scrnhit);
    }


    // Most cards do 5-6-5 RGB, which is the format the files are saved in
    // Some do 5-6-5 BGR, or  6-5-5 RGB, in which case convert the gfx
    if ((final_col_dep == 16) && ((_rgb_b_shift_16 != 0) || (_rgb_r_shift_16 != 11))) {
        convert_16bit_bgr = 1;
        if (_rgb_r_shift_16 == 10) {
            // some very old graphics cards lie about being 16-bit when they
            // are in fact 15-bit ... get around this
            _places_r = 3;
            _places_g = 3;
        }
    }
    if (final_col_dep > 16) {
        // when we're using 32-bit colour, it converts hi-color images
        // the wrong way round - so fix that

#if defined(IOS_VERSION) || defined(ANDROID_VERSION) || defined(PSP_VERSION)
        _rgb_b_shift_16 = 0;
        _rgb_g_shift_16 = 5;
        _rgb_r_shift_16 = 11;

        _rgb_b_shift_15 = 0;
        _rgb_g_shift_15 = 5;
        _rgb_r_shift_15 = 10;

        _rgb_r_shift_32 = 0;
        _rgb_g_shift_32 = 8;
        _rgb_b_shift_32 = 16;
#else
        _rgb_r_shift_16 = 11;
        _rgb_g_shift_16 = 5;
        _rgb_b_shift_16 = 0;
#endif
    }
    else if (final_col_dep == 16) {
        // ensure that any 32-bit graphics displayed are converted
        // properly to the current depth
#if defined(PSP_VERSION)
        _rgb_r_shift_32 = 0;
        _rgb_g_shift_32 = 8;
        _rgb_b_shift_32 = 16;

        _rgb_b_shift_15 = 0;
        _rgb_g_shift_15 = 5;
        _rgb_r_shift_15 = 10;
#else
        _rgb_r_shift_32 = 16;
        _rgb_g_shift_32 = 8;
        _rgb_b_shift_32 = 0;
#endif
    }
    else if (final_col_dep < 16) {
        // ensure that any 32-bit graphics displayed are converted
        // properly to the current depth
#if defined (WINDOWS_VERSION)
        _rgb_r_shift_32 = 16;
        _rgb_g_shift_32 = 8;
        _rgb_b_shift_32 = 0;
#else
        _rgb_r_shift_32 = 0;
        _rgb_g_shift_32 = 8;
        _rgb_b_shift_32 = 16;

        _rgb_b_shift_15 = 0;
        _rgb_g_shift_15 = 5;
        _rgb_r_shift_15 = 10;
#endif
    }
}

void engine_set_gfx_driver_callbacks()
{
    gfxDriver->SetCallbackForPolling(update_polled_stuff_if_runtime);
    gfxDriver->SetCallbackToDrawScreen(draw_screen_callback);
    gfxDriver->SetCallbackForNullSprite(GfxDriverNullSpriteCallback);
}

void engine_set_color_conversions()
{
    Out::FPrint("Initializing colour conversion");

    set_color_conversion(COLORCONV_MOST | COLORCONV_EXPAND_256 | COLORCONV_REDUCE_16_TO_15);
}
