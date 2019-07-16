/******************************************************************************/
/*                                                                            */
/* src/mvfs/main.c                                                            */
/*                                                                 2019/07/15 */
/* Copyright (C) 2018-2019 Mochi.                                             */
/*                                                                            */
/******************************************************************************/
/******************************************************************************/
/* インクルード                                                               */
/******************************************************************************/
/* 標準ヘッダ */
#include <stdbool.h>
#include <stdlib.h>

/* カーネルヘッダ */
#include <kernel/types.h>

/* ライブラリヘッダ */
#include <libmlog.h>
#include <libMk.h>

/* モジュール共通ヘッダ */
#include <mvfs.h>

/* モジュールヘッダ */
#include "Mount.h"
#include "Node.h"
#include "Open.h"
#include "Read.h"
#include "Write.h"


/******************************************************************************/
/* 定義                                                                       */
/******************************************************************************/
/* 機能呼出し関数テーブル型 */
typedef void ( *Func_t )( MkTaskId_t taskId,
                          void       *pParam );


/******************************************************************************/
/* ローカル関数宣言                                                           */
/******************************************************************************/
/* メインループ */
static void Loop( void );


/******************************************************************************/
/* 静的グローバル変数定義                                                     */
/******************************************************************************/
/** 機能呼出し関数テーブル */
static Func_t gFuncTbl[ MVFS_FUNCID_NUM ] =
    { MountRcvMsgMountReq,          /* MVFS_FUNCID_MOUNT    */
      OpenRcvMsgOpenReq,            /* MVFS_FUNCID_OPEN     */
      OpenRcvMsgVfsOpenResp,        /* MVFS_FUNCID_VFSOPEN  */
      WriteRcvMsgWriteReq,          /* MVFS_FUNCID_WRITE    */
      WriteRcvMsgVfsWriteResp,      /* MVFS_FUNCID_VFSWRITE */
      ReadRcvMsgReadReq,            /* MVFS_FUNCID_READ     */
      ReadRcvMsgVfsReadResp    };   /* MVFS_FUNCID_VFSREAD  */


/******************************************************************************/
/* グローバル関数定義                                                         */
/******************************************************************************/
/******************************************************************************/
/**
 * @brief       mvfsメイン関数
 * @details     各内部モジュールの初期化を起動した後、メインループを起動する。
 */
/******************************************************************************/
void main( void )
{
    int32_t ret;    /* カーネル関数戻り値 */

    /* 各機能初期化 */
    NodeInit();
    OpenInit();
    WriteInit();
    ReadInit();

    /* タスク名登録 */
    ret = MkTaskNameRegister( "VFS", NULL );

    /* 登録結果判定 */
    if ( ret != MK_TASKNAME_RET_SUCCESS ) {
        /* 失敗 */

        /* TODO: アボート */
        LibMlogPut(
            "[mvfs][%s:%d] MkTaskNameRegister() error.",
            __FILE__,
            __LINE__
        );
    }

    /* メインループ */
    Loop();

    return;
}


/******************************************************************************/
/* ローカル関数定義                                                           */
/******************************************************************************/
/******************************************************************************/
/**
 * @brief       メインループ
 * @details     メッセージの受信と機能呼び出しを繰り返す。
 */
/******************************************************************************/
static void Loop( void )
{
    int32_t      size;          /* メッセージサイズ   */
    uint32_t     errNo;         /* エラー番号         */
    MkTaskId_t   srcTaskId;     /* 送信元タスクID     */
    MvfsMsgHdr_t *pMsgHdr;      /* メッセージバッファ */

    /* 初期化 */
    errNo     = MK_MSG_ERR_NONE;
    srcTaskId = MK_TASKID_MAX;

    /* バッファメモリ割当て */
    pMsgHdr = ( MvfsMsgHdr_t * ) malloc( MK_MSG_SIZE_MAX );

    /* 割当て結果判定 */
    if ( pMsgHdr == NULL ) {
        /* 失敗 */

        /* TODO: アボート */
    }

    LibMlogPut( "[mvfs][%s:%d] mvfs start!", __FILE__, __LINE__ );

    /* メインループ */
    while ( true ) {
        /* メッセージ受信 */
        size = MkMsgReceive( MK_TASKID_NULL,        /* 受信タスクID   */
                             pMsgHdr,               /* バッファ       */
                             MK_MSG_SIZE_MAX,       /* バッファサイズ */
                             &srcTaskId,            /* 送信元タスクID */
                             &errNo           );    /* エラー番号     */

        /* メッセージ受信結果判定 */
        if ( size == MK_MSG_RET_FAILURE ) {
            /* 失敗 */

            continue;
        }

        /* 機能ID範囲チェック */
        if ( pMsgHdr->funcId > MVFS_FUNCID_MAX ) {
            /* 範囲外 */

            continue;
        }

        /* 機能ID呼出し */
        ( gFuncTbl[ pMsgHdr->funcId ] )( srcTaskId, pMsgHdr );
    }

    /* バッファ解放(実行されない) */
    free( pMsgHdr );

    return;
}


/******************************************************************************/
