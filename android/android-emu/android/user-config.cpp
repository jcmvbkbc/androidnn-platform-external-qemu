/* Copyright (C) 2015 The Android Open Source Project
**
** This software is licensed under the terms of the GNU General Public
** License version 2, as published by the Free Software Foundation, and
** may be copied, distributed, and modified under those terms.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
*/
#include "android/user-config.h"

#include "android/base/memory/ScopedPtr.h"

#include "android/emulation/bufprint_config_dirs.h"
#include "android/utils/bufprint.h"
#include "android/utils/debug.h"
#include "android/utils/system.h"
#include "android/utils/path.h"
#include <stdlib.h>
#include <errno.h>
#ifdef _MSC_VER
#include "msvc-posix.h"
#else
#include <sys/time.h>
#endif

#define  D(...)   VERBOSE_PRINT(init,__VA_ARGS__)

#if 0 /* set to 1 for more debugging */
#  define  DD(...)  D(__VA_ARGS__)
#else
#  define  DD(...)  ((void)0)
#endif

struct AUserConfig {
    ABool        changed;
    int          windowX;
    int          windowY;
    int          windowW;
    int          windowH;
    int          frameX;
    int          frameY;
    int          frameW;
    int          frameH;
    uint64_t     uuid;
    char*        iniPath;
};

/* Name of the user-config file */
#define  USER_CONFIG_FILE  "emulator-user.ini"

#define  KEY_WINDOW_X  "window.x"
#define  KEY_WINDOW_Y  "window.y"
#define  KEY_WINDOW_W  "window.w"
#define  KEY_WINDOW_H  "window.h"
#define  KEY_FRAME_X   "frame.x"
#define  KEY_FRAME_Y   "frame.y"
#define  KEY_FRAME_W   "frame.w"
#define  KEY_FRAME_H   "frame.h"
#define  KEY_UUID      "uuid"

#define  DEFAULT_X 100
#define  DEFAULT_Y 100

void auserConfig_free( AUserConfig* uconfig) {
    if (uconfig->iniPath) {
        free(uconfig->iniPath);
    }
    delete uconfig;
}

/* Create a new AUserConfig object from a given AvdInfo */
AUserConfig*
auserConfig_new( AvdInfo* info, SkinRect* monitorRect, int screenWidth, int screenHeight )
{
    char          inAndroidBuild = avdInfo_inAndroidBuild(info);
    char          needUUID = 1;
    char          temp[PATH_MAX], *p=temp, *end=p+sizeof(temp);
    CIniFile*     ini = NULL;

    std::unique_ptr<AUserConfig> uc(new AUserConfig());
    memset(uc.get(), 0, sizeof(*uc));

    /* If we are in the Android build system, store the configuration
     * in ~/.android/emulator-user.ini. otherwise, store it in the file
     * emulator-user.ini in the AVD's content directory.
     */
    if (inAndroidBuild) {
        p = bufprint_config_file(temp, end, USER_CONFIG_FILE);
    } else {
        p = bufprint(temp, end, "%s" PATH_SEP "%s", avdInfo_getContentPath(info),
                     USER_CONFIG_FILE);
    }

    /* handle the unexpected */
    if (p >= end) {
        /* Hmmm, something is weird, let's use a temporary file instead */
        p = bufprint_temp_file(temp, end, USER_CONFIG_FILE);
        if (p >= end) {
            derror("Weird: Cannot create temporary user-config file?");
            return NULL;
        }
        dwarning("Weird: Content path too long, using temporary user-config.");
    }

    uc->iniPath = ASTRDUP(temp);
    DD("looking user-config in: %s", uc->iniPath);

    {
        /* ensure that the parent directory exists */
        android::base::ScopedCPtr<char> parentPath(path_parent(uc->iniPath, 1));
        if (parentPath == NULL) {
            derror("Weird: Can't find parent of user-config file: %s",
                   uc->iniPath);
            return NULL;
        }

        if (!path_exists(parentPath.get())) {
            if (!inAndroidBuild) {
                derror("Weird: No content path for this AVD: %s",
                       parentPath.get());
                return NULL;
            }
            DD("creating missing directory: %s", parentPath.get());
            if (path_mkdir_if_needed_no_cow(parentPath.get(), 0755) < 0) {
                derror("Using empty user-config, can't create %s: %s",
                       parentPath.get(), strerror(errno));
                return NULL;
            }
        }
    }

    if (path_exists(uc->iniPath)) {
        DD("reading user-config file");
        ini = iniFile_newFromFile(uc->iniPath);
        if (ini == NULL) {
            dwarning("Can't read user-config file: %s\nUsing default values",
                     uc->iniPath);
        }
    }
    // Set the default width and height to half the monitor size
    int default_w = monitorRect->size.w / 2;
    int default_h = monitorRect->size.h / 2;
    if (default_w < 100) default_w = 100;
    if (default_h < 100) default_h = 100;
    if (screenWidth > 0 && screenHeight > 0) {
        // Reduce one side of the default size to make it match the device's aspect ratio
        double defaultSizeAspectRatio = ((double)default_w)   / default_h;
        double deviceAspectRatio      = ((double)screenWidth) / screenHeight;
        if (defaultSizeAspectRatio < deviceAspectRatio) {
            // Reduce the height
            default_h = default_w / deviceAspectRatio;
        } else {
            // Reduce the width
            default_w = default_h * deviceAspectRatio;
        }
    }
    if (ini != NULL) {
        uc->windowX = iniFile_getInteger(ini, KEY_WINDOW_X, DEFAULT_X);
        DD("    found %s = %d", KEY_WINDOW_X, uc->windowX);

        uc->windowY = iniFile_getInteger(ini, KEY_WINDOW_Y, DEFAULT_Y);
        DD("    found %s = %d", KEY_WINDOW_Y, uc->windowY);

        uc->windowW = iniFile_getInteger(ini, KEY_WINDOW_W, default_w);
        DD("    found %s = %d", KEY_WINDOW_W, uc->windowW);

        uc->windowH = iniFile_getInteger(ini, KEY_WINDOW_H, default_h);
        DD("    found %s = %d", KEY_WINDOW_H, uc->windowH);

        uc->frameX = iniFile_getInteger(ini, KEY_FRAME_X, DEFAULT_X);
        DD("    found %s = %d", KEY_FRAME_X, uc->frameX);

        uc->frameY = iniFile_getInteger(ini, KEY_FRAME_Y, DEFAULT_Y);
        DD("    found %s = %d", KEY_FRAME_Y, uc->frameY);

        uc->frameW = iniFile_getInteger(ini, KEY_FRAME_W, default_w);
        DD("    found %s = %d", KEY_FRAME_W, uc->frameW);

        uc->frameH = iniFile_getInteger(ini, KEY_FRAME_H, default_h);
        DD("    found %s = %d", KEY_FRAME_H, uc->frameH);

        if (iniFile_hasKey(ini, KEY_UUID)) {
            uc->uuid = (uint64_t) iniFile_getInt64(ini, KEY_UUID, 0LL);
            needUUID = 0;
            DD("    found %s = %lld", KEY_UUID, uc->uuid);
        }

        iniFile_free(ini);
    }
    else {
        uc->windowX  = DEFAULT_X;
        uc->windowY  = DEFAULT_Y;
        uc->windowW  = default_w;
        uc->windowH  = default_h;
        uc->frameX   = DEFAULT_X;
        uc->frameY   = DEFAULT_Y;
        uc->frameW   = default_w;
        uc->frameH   = default_h;
        uc->changed  = 1;
    }

    /* Generate a 64-bit UUID if necessary. We simply take the
     * current time, which avoids any privacy-related value.
     */
    if (needUUID) {
        struct timeval  tm;

        gettimeofday( &tm, NULL );
        uc->uuid    = (uint64_t)tm.tv_sec*1000 + tm.tv_usec/1000;
        uc->changed = 1;
        DD("    Generated UUID = %lld", uc->uuid);
    }

    return uc.release();
}


uint64_t
auserConfig_getUUID( AUserConfig*  uconfig )
{
    return uconfig->uuid;
}

void
auserConfig_getWindowGeo( AUserConfig* uconfig,
                          int* pX, int* pY, int* pW, int* pH,
                          int* pFrameX, int* pFrameY, int* pFrameW, int* pFrameH )
{
    *pX = uconfig->windowX;
    *pY = uconfig->windowY;
    *pW = uconfig->windowW;
    *pH = uconfig->windowH;

    *pFrameX = uconfig->frameX;
    *pFrameY = uconfig->frameY;
    *pFrameW = uconfig->frameW;
    *pFrameH = uconfig->frameH;
}


void
auserConfig_setWindowGeo( AUserConfig* uconfig,
                          int x, int y, int w, int h,
                          int frameX, int frameY, int frameW, int frameH )
{
    if (x      != uconfig->windowX || y      != uconfig->windowY ||
        w      != uconfig->windowW || h      != uconfig->windowH ||
        frameX != uconfig->frameX  || frameY != uconfig->frameY  ||
        frameW != uconfig->frameW  || frameH != uconfig->frameH    )
    {
        uconfig->windowX = x;
        uconfig->windowY = y;
        uconfig->windowW = w;
        uconfig->windowH = h;
        uconfig->frameX  = frameX;
        uconfig->frameY  = frameY;
        uconfig->frameW  = frameW;
        uconfig->frameH  = frameH;
        uconfig->changed = 1;
    }
}

/* Save the user configuration back to the content directory.
 * Should be used in an atexit() handler */
void
auserConfig_save( AUserConfig*  uconfig )
{
    CIniFile* ini;

    if (uconfig->changed == 0) {
        D("User-config was not changed.");
        return;
    }

    ini = iniFile_newEmpty(uconfig->iniPath);
    if (ini == NULL) {
        D("Weird: can't allocate user-config iniFile?");
        return;
    }

    iniFile_setInteger(ini, KEY_WINDOW_X, uconfig->windowX);
    iniFile_setInteger(ini, KEY_WINDOW_Y, uconfig->windowY);
    iniFile_setInteger(ini, KEY_WINDOW_W, uconfig->windowW);
    iniFile_setInteger(ini, KEY_WINDOW_H, uconfig->windowH);
    iniFile_setInteger(ini, KEY_FRAME_X,  uconfig->frameX);
    iniFile_setInteger(ini, KEY_FRAME_Y,  uconfig->frameY);
    iniFile_setInteger(ini, KEY_FRAME_W,  uconfig->frameW);
    iniFile_setInteger(ini, KEY_FRAME_H,  uconfig->frameH);
    iniFile_setInt64(ini, KEY_UUID, uconfig->uuid);
    if (iniFile_saveToFile(ini, uconfig->iniPath) < 0) {
        dwarning("could not save user configuration: %s: %s",
                 uconfig->iniPath, strerror(errno));
    } else {
        D("User configuration saved to %s", uconfig->iniPath);
    }
    iniFile_free(ini);
}
