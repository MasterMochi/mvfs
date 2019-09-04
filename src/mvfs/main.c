/******************************************************************************/
/*                                                                            */
/* src/mvfs/main.c                                                            */
/*                                                                 2019/09/03 */
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
#include <libmk.h>

/* モジュール共通ヘッダ */
#include <mvfs.h>

/* モジュールヘッダ */
#include "Close.h"
#include "Debug.h"
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
      ReadRcvMsgVfsReadResp,        /* MVFS_FUNCID_VFSREAD  */
      CloseRcvMsgCloseReq,          /* MVFS_FUNCID_CLOSE    */
      CloseRcvMsgVfsCloseResp  };   /* MVFS_FUNCID_VFSCLOSE */


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
    MkErr_t err;    /* カーネルエラー     */
    MkRet_t ret;    /* カーネル関数戻り値 */

    DEBUG_LOG_TRC( "start" );

    /* 初期化 */
    err = MK_ERR_NONE;
    ret = MK_RET_FAILURE;

    /* 各機能初期化 */
    NodeInit();
    OpenInit();
    WriteInit();
    ReadInit();
    CloseInit();

    /* タスク名登録 */
    ret = LibMkTaskNameRegister( "VFS", &err );

    /* 登録結果判定 */
    if ( ret != MK_RET_SUCCESS ) {
        /* 失敗 */

        DEBUG_LOG_ERR( "LibMkTaskNameRegister(): ret=%d, err=0x%X", ret, err );
        DEBUG_ABORT();
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
    size_t       size;          /* メッセージサイズ   */
    MkRet_t      ret;           /* 戻り値             */
    MkErr_t      err;           /* エラー内容         */
    MkTaskId_t   srcTaskId;     /* 送信元タスクID     */
    MvfsMsgHdr_t *pMsgHdr;      /* メッセージバッファ */

    DEBUG_LOG_FNC( "%s(): start.", __func__ );

    /* 初期化 */
    ret       = MK_RET_FAILURE;
    err       = MK_ERR_NONE;
    srcTaskId = MK_TASKID_MAX;

    /* バッファメモリ割当て */
    pMsgHdr = ( MvfsMsgHdr_t * ) malloc( MK_MSG_SIZE_MAX );

    /* 割当て結果判定 */
    if ( pMsgHdr == NULL ) {
        /* 失敗 */
        DEBUG_LOG_ERR( "malloc()" );
        DEBUG_ABORT();
    }

    DEBUG_LOG_TRC( "loop start!" );

    /* メインループ */
    while ( true ) {
        /* メッセージ受信 */
        ret = LibMkMsgReceive( MK_TASKID_NULL,      /* 受信タスクID   */
                               pMsgHdr,             /* バッファ       */
                               MK_MSG_SIZE_MAX,     /* バッファサイズ */
                               &srcTaskId,          /* 送信元タスクID */
                               &size,               /* 受信サイズ     */
                               &err           );    /* エラー内容     */

        /* メッセージ受信結果判定 */
        if ( ret != MK_RET_SUCCESS ) {
            /* 失敗 */
            DEBUG_LOG_ERR( "LibMkMsgReceive(): ret=%d, err=0x%X", ret, err );
            continue;
        }

        /* 機能ID範囲チェック */
        if ( pMsgHdr->funcId > MVFS_FUNCID_MAX ) {
            /* 範囲外 */
            DEBUG_LOG_ERR( "invalid funcId: 0x%X", pMsgHdr->funcId );
            continue;
        }

        /* 機能ID呼出し */
        ( gFuncTbl[ pMsgHdr->funcId ] )( srcTaskId, pMsgHdr );
    }
}


/******************************************************************************/
