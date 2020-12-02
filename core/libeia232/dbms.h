/**
 *	Copyright 2000 Keisoku Giken CO.,LTD. VW Sect.
 *
 *	Basic Author: Seiichi Takeda  '2000-March-01 Active
 *		Last Alteration $Author: seiichi $
 */

/**
 * @file dbms.h
 * @brief デバッグメッセージ表示用定義
 */

#ifndef INC_DBMS_H
#define INC_DBMS_H

#define DBMS( f, ...) { if(debuglevel) { fprintf( stderr, "%s %u : ", __FILE__, __LINE__); fprintf( stderr,  __VA_ARGS__ ); }}
#define DBMS1( f, ...) { if(debuglevel>=1) { fprintf( stderr, "[DBMS1]%s %u : ", __FILE__, __LINE__); fprintf( stderr,  __VA_ARGS__ ); }}
#define DBMS2( f, ...) { if(debuglevel>=2) { fprintf( stderr, "[DBMS2]%s %u : ", __FILE__, __LINE__); fprintf( stderr,  __VA_ARGS__ ); }}
#define DBMS3( f, ...) { if(debuglevel>=3) { fprintf( stderr, "[DBMS3]%s %u : ", __FILE__, __LINE__); fprintf( stderr,  __VA_ARGS__ ); }}
#define DBMS4( f, ...) { if(debuglevel>=4) { fprintf( stderr, __VA_ARGS__ ); }}
#define DBMS5( f, ...) { if(debuglevel>=5) { fprintf( stderr, __VA_ARGS__ ); }}

#endif /* end of INC_DBMS_H */
