/******************************************************************************/
/*                                                                            */
/* src/mvfs/Fd.h                                                              */
/*                                                                 2019/07/08 */
/* Copyright (C) 2019 Mochi.                                                  */
/*                                                                            */
/******************************************************************************/
#ifndef FD_H
#define FD_H
/******************************************************************************/
/* インクルード                                                               */
/******************************************************************************/
/* 標準ヘッダ */
#include <stdbool.h>

/* カーネルヘッダ */
#include <kernel/types.h>

/* モジュール内ヘッダ */
#include "Node.h"


/******************************************************************************/
/* 定義                                                                       */
/******************************************************************************/
/** ファイルディスクリプタ情報 */
typedef struct {
    bool       used;        /**< 使用済みフラグ */
    uint32_t   globalFd;    /**< グローバルFD   */
    uint32_t   localFd;     /**< ローカルFD     */
    MkPid_t    pid;         /**< PID            */
    NodeInfo_t *pNode;      /**< ノード         */
} FdInfo_t;


/******************************************************************************/
/* グローバル関数宣言                                                         */
/******************************************************************************/
/* FD割当 */
extern uint32_t FdAlloc( uint32_t   localFd,
                         MkPid_t    pid,
                         NodeInfo_t *pNode   );
/* FD情報取得 */
extern FdInfo_t *FdGet( uint32_t globalFd );
/* FD管理初期化 */
extern void FdInit( void );


/******************************************************************************/
#endif
