/******************************************************************************/
/*                                                                            */
/* src/mvfs/main.c                                                            */
/*                                                                 2019/10/13 */
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

/* 外部モジュールヘッダ */
#include <mvfs.h>
#include <Debug.h>
#include <Fd.h>
#include <Fn.h>
#include <Node.h>


/******************************************************************************/
/* 定義                                                                       */
/******************************************************************************/
/* 機能呼出し関数テーブル型 */
typedef void ( *Func_t )( MkTaskId_t taskId,
                          void       *pParam,
                          size_t     size     );


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
    { [ MVFS_FUNCID_MOUNT    ] = FnTaskRecvMountReq,
      [ MVFS_FUNCID_OPEN     ] = FnMainRecvOpenReq,
      [ MVFS_FUNCID_VFSOPEN  ] = FnMainRecvVfsOpenResp,
      [ MVFS_FUNCID_WRITE    ] = FnMainRecvWriteReq,
      [ MVFS_FUNCID_VFSWRITE ] = FnMainRecvVfsWriteResp,
      [ MVFS_FUNCID_READ     ] = FnMainRecvReadReq,
      [ MVFS_FUNCID_VFSREAD  ] = FnMainRecvVfsReadResp,
      [ MVFS_FUNCID_CLOSE    ] = FnMainRecvCloseReq,
      [ MVFS_FUNCID_VFSCLOSE ] = FnMainRecvVfsCloseResp,
      [ MVFS_FUNCID_SELECT   ] = FnTaskRecvSelectReq,
      [ MVFS_FUNCID_VFSREADY ] = FnTaskRecvVfsReadyNtc   };


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

    DEBUG_LOG_TRC( "start." );

    /* 初期化 */
    err = MK_ERR_NONE;
    ret = MK_RET_FAILURE;

    /* 各機能初期化 */
    FdInit();
    NodeInit();
    FnInit();

    /* タスク名登録 */
    ret = LibMkTaskNameRegister( "VFS", &err );

    /* 登録結果判定 */
    if ( ret != MK_RET_SUCCESS ) {
        /* 失敗 */

        DEBUG_LOG_ERR(
            "%s(): LibMkTaskNameRegister(): ret=%d, err=%#X",
            __func__,
            ret,
            err
        );
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

    /* 初期化 */
    ret       = MK_RET_FAILURE;
    err       = MK_ERR_NONE;
    srcTaskId = MK_TASKID_MAX;

    /* バッファメモリ割当て */
    pMsgHdr = ( MvfsMsgHdr_t * ) malloc( MK_MSG_SIZE_MAX );

    /* 割当て結果判定 */
    if ( pMsgHdr == NULL ) {
        /* 失敗 */
        DEBUG_LOG_ERR( "%s(): malloc(): size=%u", __func__, MK_MSG_SIZE_MAX );
        DEBUG_ABORT();
    }

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
            DEBUG_LOG_ERR(
                "%s(): LibMkMsgReceive(): ret=%d, err=%#X",
                __func__,
                ret,
                err
            );
            continue;
        }

        /* ヘッダサイズチェック */
        if ( size < sizeof ( MvfsMsgHdr_t ) ) {
            /* 不正 */
            DEBUG_LOG_ERR(
                "%s(): invalid size( %u < %u ).",
                __func__,
                size,
                sizeof ( MvfsMsgHdr_t )
            );
            continue;
        }

        /* 機能ID範囲チェック */
        if ( pMsgHdr->funcId > MVFS_FUNCID_MAX ) {
            /* 範囲外 */
            DEBUG_LOG_ERR(
                "%s(): invalid funcId( %#X > %#X ).",
                __func__,
                pMsgHdr->funcId,
                MVFS_FUNCID_MAX
            );
            continue;
        }

        /* 機能ID呼出し */
        ( gFuncTbl[ pMsgHdr->funcId ] )( srcTaskId, pMsgHdr, size );
    }
}


/******************************************************************************/
