/**
 *	Copyright TSN・SINGENN
 *	Basic Author: Seiichi Takeda  '2000-March-01 Active
 *		Last Alteration $Author: takeda $
 */

/**
 * @file dbms.h
 * @brief デバッグメッセージ表示用定義
 */

#ifndef INC_DBMS_H
#define INC_DBMS_H

#if defined(MICROBLAZE) && defined(STANDALONE)
#define DMSG( f, ...) { xil_printf( __VA_ARGS__ ); }
#define DBMS( f, ...) { if(1) { xil_printf("%s %u : ", __FILE__, __LINE__); xil_printf( __VA_ARGS__ ); }}
#define DBMS1( f, ...) { if(debuglevel>=1) { xil_printf("[DBMS1]%s %u : ", __FILE__, __LINE__); xil_printf( __VA_ARGS__ ); }}
#define DBMS2( f, ...) { if(debuglevel>=2) { xil_printf("[DBMS2]%s %u : ", __FILE__, __LINE__); xil_printf( __VA_ARGS__ ); }}
#define DBMS3( f, ...) { if(debuglevel>=3) { xil_printf("[DBMS3]%s %u : ", __FILE__, __LINE__); xil_printf( __VA_ARGS__ ); }}
#define DBMS4( f, ...) { if(debuglevel>=4) { xil_printf(__VA_ARGS__ ); }}
#define DBMS5( f, ...) { if(debuglevel>=5) { xil_printf(__VA_ARGS__ ); }}
#else
#define DMSG( f, ...) { fprintf( stderr, __VA_ARGS__ ); }
#define DBMS( f, ...) { if(1) { fprintf( stderr, "%s %u : ", __FILE__, __LINE__); fprintf( stderr,  __VA_ARGS__ ); }}
#define DBMS1( f, ...) { if(debuglevel>=1) { fprintf( stderr, "[DBMS1]%s %u : ", __FILE__, __LINE__); fprintf( stderr,  __VA_ARGS__ ); }}
#define DBMS2( f, ...) { if(debuglevel>=2) { fprintf( stderr, "[DBMS2]%s %u : ", __FILE__, __LINE__); fprintf( stderr,  __VA_ARGS__ ); }}
#define DBMS3( f, ...) { if(debuglevel>=3) { fprintf( stderr, "[DBMS3]%s %u : ", __FILE__, __LINE__); fprintf( stderr,  __VA_ARGS__ ); }}
#define DBMS4( f, ...) { if(debuglevel>=4) { fprintf( stderr, __VA_ARGS__ ); }}
#define DBMS5( f, ...) { if(debuglevel>=5) { fprintf( stderr, __VA_ARGS__ ); }}
#endif

#define IFDBG1THEN if(debuglevel>=1)
#define IFDBG2THEN if(debuglevel>=2)
#define IFDBG3THEN if(debuglevel>=3)
#define IFDBG4THEN if(debuglevel>=4)
#define IFDBG5THEN if(debuglevel>=5)

#endif /* end of INC_DBMS_H */
