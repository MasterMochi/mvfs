/******************************************************************************/
/* src/lib/libc/include/stdlib.h                                              */
/*                                                                 2019/03/26 */
/* Copyright (C) 2018-2019 Mochi.                                             */
/******************************************************************************/
#ifndef _STDLIB_H_
#define _STDLIB_H_
/******************************************************************************/
/* インクルード                                                               */
/******************************************************************************/
/* 共通ヘッダ */
#include <stddef.h>


/******************************************************************************/
/* グローバル関数プロトタイプ宣言                                             */
/******************************************************************************/
extern void *malloc( size_t size );
extern void free( void *ptr );


/******************************************************************************/
#endif
