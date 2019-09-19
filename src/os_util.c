/* 
 * Tux Racer 
 * Copyright (C) 1999-2001 Jasmin F. Patry
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#pragma warning(disable: 4996) //---- kong ---- for GetVersionEx
#include "tuxracer.h"

#ifdef _MSC_VER
#  define snprintf _snprintf
#endif

/* This function only used in Win32; ifdef'd to eliminate "unused" warnings */
#if defined(WIN32)

static bool_t append_to_buff( char **buff, int *size, char *string )
{
    int len;

    if ( *size < 0 ) {
        return False;
    }

    len = snprintf( *buff, *size, "%s", string );

    check_assertion( len >= 0, "buff too small" );
    if ( len < 0 ) {
        return False;
    }
    
    *buff += len;
    *size -= len;

    return True;
}

#endif /* WIN32 */

int get_os_version( char *buff, int size )
{

#ifdef WIN32
    /* Win32 Version */
    /* See http://www.mvps.org/vb/index2.html?tips/getversionex.htm for  a table mapping OSVERSIONINFOEX entries to Windows version */

    char tmp_buff[BUFF_LEN];
    int tmp_buff_size = BUFF_LEN;
    char *ptr = tmp_buff;
    int len;
    
    OSVERSIONINFO osvi;
    
    ZeroMemory(&osvi, sizeof(OSVERSIONINFO));
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    
    if ( !GetVersionEx( (OSVERSIONINFO *) &osvi) ) { //---- kong ---- ????
        return -1;
    }
    
    switch (osvi.dwPlatformId)
    {
    case VER_PLATFORM_WIN32_NT:

        /* Check for NT versus 2000 */
        if ( osvi.dwMajorVersion <= 4 ) {
            if ( !append_to_buff( &ptr, &tmp_buff_size, "Microsoft Windows NT" ) )
            {
                return -1;
            }
        }

        if ( osvi.dwMajorVersion == 5 ) {
            if ( !append_to_buff( &ptr, &tmp_buff_size, "Microsoft Windows 2000" ) )
            {
                return -1;
            }
        }
        /* Display version, service pack (if any), and build number. */
        //---- kong ---- %s to %S
        len = snprintf(ptr, tmp_buff_size, " version %d.%d %S (Build %d)", osvi.dwMajorVersion, osvi.dwMinorVersion, osvi.szCSDVersion, osvi.dwBuildNumber & 0xFFFF);
        check_assertion( len >= 0, "tmp_buff too small" );
        if ( len < 0 ) {
            return -1;
        }
        ptr += len;
        tmp_buff_size -= len;
        break;

    case VER_PLATFORM_WIN32_WINDOWS:
        if ((osvi.dwMajorVersion > 4) || 
            ((osvi.dwMajorVersion == 4) && (osvi.dwMinorVersion > 0)))
        {
            if ( osvi.dwMinorVersion <= 10 ) {
                if ( strcmp( (char *)osvi.szCSDVersion, "A" ) == 0 ) { //---- kong ---- add (char *)
                    if ( !append_to_buff( &ptr, &tmp_buff_size, "Microsoft Windows 98 SE") )
                    {
                        return -1;
                    }
                } else {
                    if ( !append_to_buff( &ptr, &tmp_buff_size, "Microsoft Windows 98") )
                    {
                        return -1;
                    }
                }
            } else {
                if ( !append_to_buff( &ptr, &tmp_buff_size, "Microsoft Windows ME") )
                {
                    return -1;
                }
            }
        } else {
            if ( strcmp( (char *)osvi.szCSDVersion, "B" ) == 0 ) { //---- kong ---- add (char *)
                if ( !append_to_buff( &ptr, &tmp_buff_size, "Microsoft Windows 95 OSR2") )
                {
                    return -1;
                }
            } else {
                if ( !append_to_buff( &ptr, &tmp_buff_size, "Microsoft Windows 95") )
                {
                    return -1;
                }
            }
        }

        /* Append Build */
        len = snprintf(ptr, tmp_buff_size, " (Build %d)", osvi.dwBuildNumber & 0xFFFF);

        check_assertion( len >= 0, "tmp_buff too small" );
        if ( len < 0 ) {
            return -1;
        }

        ptr += len;
        tmp_buff_size -= len;
        break;

    case VER_PLATFORM_WIN32s:
        if ( !append_to_buff( &ptr, &tmp_buff_size, "Microsoft Win32s") ) {
            return -1;
        }
        break;
    }

    len = snprintf( buff, size, "%s", tmp_buff );
    if ( len < 0 ) {
        /* buffer too small */
        buff[size-1] = (char)0;
        return 1;
    }
    return 0;

#endif /* WIN32 */
}

/* EOF */