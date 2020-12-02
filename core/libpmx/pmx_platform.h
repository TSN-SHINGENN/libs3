#ifndef INC_PMX_PLATFORM_H
#define INC_PMX_PLATFORM_H

#pragma once


namespace libs3 {

/* Microsoft Windows系 バージョン定義 { MajerVer, MinerVer } */
#define PMX_PLATFORM_VER_WINDOWS_95    {  4, 0}
#define PMX_PLATFORM_VER_WINDOWS_98    {  4, 1}
#define PMX_PLATFORM_VER_WINDOWS_2k    {  5, 0}
#define PMX_PLATFORM_VER_WINDOWS_XP    {  5, 1}
#define PMX_PLATFORM_VER_WINDOWS_XP64  {  5, 2}
#define PMX_PLATFORM_VER_WINDOWS_SERVER2003 PMX_PLATFORM_VER_WINDOWS_XP64
#define PMX_PLATFORM_VER_WINDOWS_VISTA {  6, 0}
#define PMX_PLATFORM_VER_WINDOWS_HOMESERVER PMX_PLATFORM_VER_WINDOWS_VISTA
#define PMX_PLATFORM_VER_WINDOWS_SERVER2008 PMX_PLATFORM_VER_WINDOWS_VISTA
#define PMX_PLATFORM_VER_WINDOWS_7     {  6, 1}
#define PMX_PLATFORM_VER_WINDOWS_SERVER2008R2   PMX_PLATFORM_VER_WINDOWS_7
#define PMX_PLATFORM_VER_WINDOWS_HOMESERVER2011 PMX_PLATFORM_VER_WINDOWS_7
#define PMX_PLATFORM_VER_WINDOWS_8     {  6, 2}
#define PMX_PLATFORM_VER_WINDOWS_SERVER1012 PMX_PLATFORM_VER_WINDOWS_8
#define PMX_PLATFORM_VER_WINDOWS_8_1   {  6, 3}
#define PMX_PLATFORM_VER_WINDOWS_SERVER1012R2 PMX_PLATFORM_VER_WINDOWS_8_1
#define PMX_PLATFORM_VER_WINDOWS_10    { 10, 0}
#define PMX_PLATFORM_VER_WINDOWS_SERVER1016 PMX_PLATFORM_VER_WINDOWS_10

/* Apple MACOS系 バージョン定義(not Kernel Version) { MajerVer, MinerVer } */
#define PMX_PLATFORM_VER_MACOSX_CHEETAH      { 10, 0}
#define PMX_PLATFORM_VER_MACOSX_PUMA         { 10, 1}
#define PMX_PLATFORM_VER_MACOSX_JAGUAR       { 10, 2}
#define PMX_PLATFORM_VER_MACOSX_PANTHER      { 10, 3}
#define PMX_PLATFORM_VER_MACOSX_TIGER        { 10, 4}
#define PMX_PLATFORM_VER_MACOSX_LEOPARD      { 10, 5}
#define PMX_PLATFORM_VER_MACOSX_SNOWLEOPARD  { 10, 6}
#define PMX_PLATFORM_VER_MACOSX_LION         { 10, 7}
#define PMX_PLATFORM_VER_MACOSX_MOUNTAINLION { 10, 8}
#define PMX_PLATFORM_VER_MACOSX_MAVERICKS    { 10, 9}
#define PMX_PLATFORM_VER_MACOSX_YOSEMITE     { 10, 10}
#define PMX_PLATFORM_VER_MACOSX_EICAPTAN     { 10, 11}
#define PMX_PLATFOEM_VER_MACOS_SIERRA        { 10, 12}
#define PMX_PLATFOEM_VER_MACOS_HIGHSIERRA    { 10, 13}
#define PMX_PLATFOEM_VER_MACOS_MOJAVE        { 10, 14}

int pmx_platform_is_ms_windows(void);
int pmx_platform_is_ms_windows_10(void);
int pmx_platform_is_ms_windows_8_1(void);
int pmx_platform_is_ms_windows_8(void);
int pmx_platform_is_ms_windows_7(void);
int pmx_platform_is_ms_windows_vista(void);
int pmx_platform_is_ms_windows_xp(void);
int pmx_platform_is_ms_windows_2k(void);
int pmx_platform_is_ms_windows_2003Server(void);
int pmx_platform_is_ms_windows_9x(void);
int pmx_platform_is_ms_windows_nt(void);
int pmx_platform_get_ms_windows_nt_version(int *majver_p, int *minver_p);
int pmx_platform_get_ms_windows_10_build_version(void *);

int pmx_platform_is_linux(void);
int pmx_platform_get_linux_version(int *majver_p, int *minver_p, int *rev_p);

int pmx_platform_is_macosx(void);

int pmx_platform_is_macosx_snowleopard(void);
int pmx_platform_is_macosx_leopard(void);
int pmx_platform_is_macosx_lion(void);
int pmx_platform_is_macosx_moutainlion(void);
int pmx_platform_is_macosx_mavericks(void);
int pmx_platform_is_macosx_yosemite(void);
int pmx_platform_is_macosx_eicaptain(void);
int pmx_platform_is_macos_sierra(void);
int pmx_platform_is_macos_highsierra(void);
int pmx_platform_is_macos_mojave(void);

int pmx_platform_get_macosx_version( int *majver_p, int *minver_p, int *rev_p);

int pmx_platofrm_is_qnx(void);
int pmx_platform_get_qnx_version(int *majver_p, int *minver_p, int *rev_p);

int pmx_platform_is_sunos(void);
int pmx_platform_get_sunos_version(int *majver_p, int *minver_p);

int pmx_platform_is_xilinx_standalone(void);
int pmx_platform_is_xilinx_xilkernel(void);
int pmx_platform_is_xilinx_freertos(void);

int pmx_platform_is_emulate_32bit_on_64bit_kernel(void);

}

#endif /* end of INC_PMX_PLATFORM_H */
