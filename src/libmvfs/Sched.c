/******************************************************************************/
/*                                                                            */
/* src/libmvfs/Sched.c                                                        */
/*                                                                 2019/10/01 */
/* Copyright (C) 2019 Mochi.                                                  */
/*                                                                            */
/******************************************************************************/
/******************************************************************************/
/* インクルード                                                               */
/******************************************************************************/
/* 標準ヘッダ */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* カーネルヘッダ */
#include <kernel/types.h>

/* ライブラリヘッダ */
#include <libmk.h>
#include <libmvfs.h>
#include <MLib/MLib.h>

/* 共通ヘッダ */
#include <mvfs.h>


/******************************************************************************/
/* ローカル関数宣言                                                           */
/******************************************************************************/
/* 仮想ファイルサーバメッセージ処理 */
static void Proc( LibMvfsSchedInfo_t *pInfo,
                  MvfsMsgHdr_t       *pMsg   );
/* vfsClose要求メッセージ処理 */
static void ProcVfsCloseReq( LibMvfsSchedInfo_t   *pInfo,
                             MvfsMsgVfsCloseReq_t *pMsg   );
/* その他メッセージ処理 */
static void ProcOther( LibMvfsSchedInfo_t *pInfo,
                       MkTaskId_t         src,
                       void               *pMsg,
                       size_t             size    );
/* vfsOpen要求メッセージ処理 */
static void ProcVfsOpenReq( LibMvfsSchedInfo_t  *pInfo,
                            MvfsMsgVfsOpenReq_t *pMsg   );
/* vfsWrite要求メッセージ処理 */
static void ProcVfsWriteReq( LibMvfsSchedInfo_t   *pInfo,
                             MvfsMsgVfsWriteReq_t *pMsg   );
/* vfsRead要求メッセージ処理 */
static void ProcVfsReadReq( LibMvfsSchedInfo_t  *pInfo,
                            MvfsMsgVfsReadReq_t *pMsg   );


/******************************************************************************/
/* ライブラリ関数定義                                                         */
/******************************************************************************/
/******************************************************************************/
/**
 * @brief       スケジュール開始
 * @details     メッセージ受信を繰り返し行い、メッセージに応じたコールバック関
 *              数を呼び出す。
 *
 * @param[in]   *pInfo  スケジューラ情報
 * @param[out]  *pErrNo エラー番号
 *                  - LIBMVFS_ERR_NONE      エラー無し
 *                  - LIBMVFS_ERR_PARAM     パラメータ不正
 *                  - LIBMVFS_ERR_NOT_FOUND 仮想ファイルサーバが不明
 *                  - LIBMVFS_ERR_NO_MEMORY メモリ不足
 *
 * @return      処理結果を返す。
 * @retval      LIBMVFS_RET_SUCCESS 正常終了
 * @retval      LIBMVFS_RET_FAILURE 異常終了
 */
/******************************************************************************/
LibMvfsRet_t LibMvfsSchedStart( LibMvfsSchedInfo_t *pInfo,
                                uint32_t           *pErrNo )
{
    char         *pBuffer;  /* 受信バッファ               */
    size_t       size;      /* 受信サイズ                 */
    MkRet_t      retLibMk;  /* カーネル戻り値             */
    MkErr_t      err;       /* エラー内容                 */
    MkTaskId_t   src;       /* 送信元タスクID             */
    MkTaskId_t   mvfs;      /* 仮想ファイルサーバタスクID */
    LibMvfsRet_t ret;       /* 戻り値                     */

    /* 初期化 */
    pBuffer  = NULL;
    size     = 0;
    retLibMk = MK_RET_FAILURE;
    err      = MK_ERR_NONE;
    src      = MK_TASKID_NULL;
    mvfs     = MK_TASKID_NULL;

    /* エラー番号初期化 */
    MLIB_SET_IFNOT_NULL( pErrNo, LIBMVFS_ERR_NONE );

    /* パラメータチェック */
    if ( pInfo == NULL ) {
        /* 不正 */

        /* エラー番号設定 */
        MLIB_SET_IFNOT_NULL( pErrNo, LIBMVFS_ERR_PARAM );

        return LIBMVFS_RET_FAILURE;
    }

    /* 受信バッファ割当 */
    pBuffer = malloc( MK_MSG_SIZE_MAX );

    /* 割当結果判定 */
    if ( pBuffer == NULL ) {
        /* 失敗 */

        /* エラー番号設定 */
        MLIB_SET_IFNOT_NULL( pErrNo, LIBMVFS_ERR_NO_MEMORY );

        return LIBMVFS_RET_FAILURE;
    }

    /* ループ */
    while ( true ) {
        /* 初期化 */
        memset( pBuffer, 0, MK_MSG_SIZE_MAX );

        /* 仮想ファイルサーバタスクID取得 */
        ret = LibMvfsGetTaskId( &mvfs, pErrNo );

        /* 取得結果判定 */
        if ( ret != LIBMVFS_RET_SUCCESS ) {
            /* 失敗 */

            break;
        }

        /* メッセージ受信 */
        retLibMk = LibMkMsgReceive( MK_TASKID_NULL,        /* 受信タスクID   */
                                    pBuffer,               /* バッファ       */
                                    MK_MSG_SIZE_MAX,       /* バッファサイズ */
                                    &src,                  /* 送信元タスクID */
                                    &size,                 /* 受信サイズ     */
                                    &err              );

        /* 受信結果判定 */
        if ( retLibMk != MK_RET_SUCCESS ) {
            /* 失敗 */

            continue;
        }

        /* 送信元タスクID判定 */
        if ( src == mvfs ) {
            /* 仮想ファイルサーバ */

            Proc( pInfo, ( MvfsMsgHdr_t * ) pBuffer );

        } else {
            /* その他 */

            ProcOther( pInfo, src, pBuffer, size );
        }
    }

    /* バッファ解放 */
    free( pBuffer );

    return ret;
}


/******************************************************************************/
/* ローカル関数定義                                                           */
/******************************************************************************/
/******************************************************************************/
/**
 * @brief       仮想ファイルサーバメッセージ処理
 * @details     仮想ファイルサーバから受信したメッセージを判定し、そのメッセー
 *              ジに対応した関数を呼び出す。
 *
 * @param[in]   *pInfo スケジューラ情報
 * @param[in]   *pMsg  受信メッセージ
 */
/******************************************************************************/
static void Proc( LibMvfsSchedInfo_t *pInfo,
                  MvfsMsgHdr_t       *pMsg   )
{
    /* メッセージ判定 */
    if ( ( pMsg->funcId == MVFS_FUNCID_VFSOPEN ) &&
         ( pMsg->type   == MVFS_TYPE_REQ       )    ) {
        /* vfsOpen要求 */

        /* vfsOpen要求処理 */
        ProcVfsOpenReq( pInfo, ( MvfsMsgVfsOpenReq_t * ) pMsg );

    } else if ( ( pMsg->funcId == MVFS_FUNCID_VFSWRITE ) &&
                ( pMsg->type   == MVFS_TYPE_REQ        )    ) {
        /* vfsWrite要求 */

        /* vfsWrite要求処理 */
        ProcVfsWriteReq( pInfo, ( MvfsMsgVfsWriteReq_t * ) pMsg );

    } else if ( ( pMsg->funcId == MVFS_FUNCID_VFSREAD ) &&
                ( pMsg->type   == MVFS_TYPE_REQ       )    ) {
        /* vfsRead要求 */

        /* vfsRead要求処理 */
        ProcVfsReadReq( pInfo, ( MvfsMsgVfsReadReq_t * ) pMsg );

    } else if ( ( pMsg->funcId == MVFS_FUNCID_VFSCLOSE ) &&
                ( pMsg->type   == MVFS_TYPE_REQ        )    ) {
        /* vfsClose要求 */

        /* vfsClose要求処理 */
        ProcVfsCloseReq( pInfo, ( MvfsMsgVfsCloseReq_t * ) pMsg );
    }

    return;
}


/******************************************************************************/
/**
 * @brief       vfsClose要求メッセージ処理
 * @details     vfsClose要求コールバック関数を呼び出す。コールバック関数が設定
 *              されていない場合は、vfsClose応答を行う。
 *
 * @param[in]   *pInfo スケジューラ情報
 * @param[in]   *pMsg  受信メッセージ
 */
/******************************************************************************/
static void ProcVfsCloseReq( LibMvfsSchedInfo_t   *pInfo,
                             MvfsMsgVfsCloseReq_t *pMsg   )
{
    /* コールバック関数設定判定 */
    if ( pInfo->callBack.pVfsClose != NULL ) {
        /* 設定有り */

        /* コールバック */
        ( pInfo->callBack.pVfsClose )( pMsg->globalFd );

    } else {
        /* 設定無し */

        /* vfsClose応答送信 */
        LibMvfsSendVfsCloseResp( pMsg->globalFd, MVFS_RESULT_SUCCESS, NULL );
    }

    return;
}


/******************************************************************************/
/**
 * @brief       その他メッセージ処理
 * @details     仮想ファイルサーバからでないメッセージ用のコールバック関数を呼
 *              び出す。
 *
 * @param[in]   *pInfo スケジューラ情報
 * @param[in]   src    送信元タスクID
 * @param[in]   *pMsg  受信メッセージ
 * @param[in]   size   受信メッセージ長
 */
/******************************************************************************/
static void ProcOther( LibMvfsSchedInfo_t  *pInfo,
                       MkTaskId_t          src,
                       void                *pMsg,
                       size_t              size    )
{
    /* コールバック関数設定判定 */
    if ( pInfo->callBack.pOther != NULL ) {
        /* 設定有り */

        /* コールバック */
        ( pInfo->callBack.pOther )( src, pMsg, size );
    }

    return;
}


/******************************************************************************/
/**
 * @brief       vfsOpen要求メッセージ処理
 * @details     vfsOpen要求コールバック関数を呼び出す。コールバック関数が設定さ
 *              れていない場合は、vfsOpen応答を行う。
 *
 * @param[in]   *pInfo スケジューラ情報
 * @param[in]   *pMsg  受信メッセージ
 */
/******************************************************************************/
static void ProcVfsOpenReq( LibMvfsSchedInfo_t  *pInfo,
                            MvfsMsgVfsOpenReq_t *pMsg   )
{
    /* コールバック関数設定判定 */
    if ( pInfo->callBack.pVfsOpen != NULL ) {
        /* 設定有り */

        /* コールバック */
        ( pInfo->callBack.pVfsOpen )( pMsg->pid,
                                      pMsg->globalFd,
                                      pMsg->path      );

    } else {
        /* 設定無し */

        /* vfsOpen応答送信 */
        LibMvfsSendVfsOpenResp( pMsg->globalFd, MVFS_RESULT_SUCCESS, NULL );
    }

    return;
}


/******************************************************************************/
/**
 * @brief       vfsWrite要求メッセージ処理
 * @details     vfsWrite要求コールバック関数を呼び出す。コールバック関数が設定さ
 *              れていない場合は、vfsWrite応答を行う。
 *
 * @param[in]   *pInfo スケジューラ情報
 * @param[in]   *pMsg  受信メッセージ
 */
/******************************************************************************/
static void ProcVfsWriteReq( LibMvfsSchedInfo_t   *pInfo,
                             MvfsMsgVfsWriteReq_t *pMsg   )
{
    /* コールバック関数設定判定 */
    if ( pInfo->callBack.pVfsWrite != NULL ) {
        /* 設定有り */

        /* コールバック */
        ( pInfo->callBack.pVfsWrite )( pMsg->globalFd,
                                       pMsg->writeIdx,
                                       pMsg->pBuffer,
                                       pMsg->size      );

    } else {
        /* 設定無し */

        /* vfsWrite応答送信 */
        LibMvfsSendVfsWriteResp( pMsg->globalFd,
                                 MVFS_RESULT_FAILURE,
                                 0,
                                 0,
                                 NULL                 );
    }

    return;
}


/******************************************************************************/
/**
 * @brief       vfsRead要求メッセージ処理
 * @details     vfsRead要求コールバック関数を呼び出す。コールバック関数が設定さ
 *              れていない場合は、vfsRead応答を行う。
 *
 * @param[in]   *pInfo スケジューラ情報
 * @param[in]   *pMsg  受信メッセージ
 */
/******************************************************************************/
static void ProcVfsReadReq( LibMvfsSchedInfo_t  *pInfo,
                            MvfsMsgVfsReadReq_t *pMsg   )
{
    /* コールバック関数設定判定 */
    if ( pInfo->callBack.pVfsRead != NULL ) {
        /* 設定有り */

        /* コールバック */
        ( pInfo->callBack.pVfsRead )( pMsg->globalFd,
                                      pMsg->readIdx,
                                      pMsg->size      );

    } else {
        /* 設定無し */

        /* vfsRead応答送信 */
        LibMvfsSendVfsReadResp( pMsg->globalFd,
                                MVFS_RESULT_FAILURE,
                                0,
                                NULL,
                                0,
                                NULL                 );
    }

    return;
}


/******************************************************************************/
