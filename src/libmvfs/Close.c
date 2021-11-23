/******************************************************************************/
/*                                                                            */
/* src/libmvfs/Close.c                                                        */
/*                                                                 2021/11/17 */
/* Copyright (C) 2019-2021 Mochi.                                             */
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

/* ライブラリヘッダ */
#include <libmk.h>
#include <libmvfs.h>
#include <MLib/MLib.h>

/* 共通ヘッダ */
#include <mvfs.h>

/* モジュール内ヘッダ */
#include "Fd.h"
#include "Sched.h"


/******************************************************************************/
/* ローカル関数定義                                                           */
/******************************************************************************/
/* close応答受信 */
static LibMvfsRet_t ReceiveCloseResp( MkTaskId_t         taskId,
                                      MvfsMsgCloseResp_t *pMsg,
                                      uint32_t           *pErrNo );
/* close要求送信 */
static LibMvfsRet_t SendCloseReq( MkTaskId_t taskId,
                                  uint32_t   globalFd,
                                  uint32_t   *pErrNo   );


/******************************************************************************/
/* ライブラリ関数定義                                                         */
/******************************************************************************/
/******************************************************************************/
/**
 * @brief       ファイルクローズ
 * @details     仮想ファイルサーバにclose要求メッセージを送信して、ファイルのク
 *              ローズを要求し、その応答を待ち合わせる。
 *
 * @param[in]   fd      ファイルディスクリプタ
 * @param[out]  *pErrNo エラー番号
 *                  - LIBMVFS_ERR_NONE      エラー無し
 *                  - LIBMVFS_ERR_PARAM     パラメータ不正
 *                  - LIBMVFS_ERR_NOT_FOUND 仮想ファイルサーバが不明
 *                  - LIBMVFS_ERR_NOT_RESP  応答無し
 *                  - LIBMVFS_ERR_OTHER     その他エラー
 *
 * @return      処理結果を返す。
 * @retval      LIBMVFS_RET_SUCCESS 正常終了
 * @retval      LIBMVFS_RET_FAILURE 異常終了
 */
/******************************************************************************/
LibMvfsRet_t LibMvfsClose( uint32_t fd,
                           uint32_t *pErrNo )
{
    FdInfo_t           *pFdInfo;    /* ローカルFD情報     */
    MkTaskId_t         taskId;      /* タスクID           */
    LibMvfsRet_t       ret;         /* 戻り値             */
    MvfsMsgCloseResp_t respMsg;     /* 応答メッセージ     */

    /* 初期化 */
    pFdInfo = NULL;
    taskId  = MK_TASKID_NULL;
    ret     = LIBMVFS_RET_FAILURE;
    memset( &respMsg, 0, sizeof ( respMsg ) );

    /* タスクID取得 */
    ret = LibMvfsGetTaskId( &taskId, pErrNo );

    /* 取得結果判定 */
    if ( ret != LIBMVFS_RET_SUCCESS ) {
        /* 失敗 */

        return LIBMVFS_RET_FAILURE;
    }

    /* ローカルFD情報取得 */
    pFdInfo = FdGetLocalFdInfo( fd );

    /* 取得結果判定 */
    if ( pFdInfo == NULL ) {
        /* 失敗 */

        /* エラー番号設定 */
        MLIB_SET_IFNOT_NULL( pErrNo, LIBMVFS_ERR_INVALID_FD );

        return LIBMVFS_RET_FAILURE;
    }

    /* close要求メッセージ送信 */
    ret = SendCloseReq( taskId,
                        pFdInfo->globalFd,
                        pErrNo             );

    /* 送信結果判定 */
    if ( ret != LIBMVFS_RET_SUCCESS ) {
        /* 失敗 */

        return LIBMVFS_RET_FAILURE;
    }

    /* close応答メッセージ受信 */
    ret = ReceiveCloseResp( taskId, &respMsg, pErrNo );

    /* 受信結果判定 */
    if ( ret != LIBMVFS_RET_SUCCESS ) {
        /* 失敗 */

        return LIBMVFS_RET_FAILURE;
    }

    /* close処理結果判定 */
    if ( respMsg.result != MVFS_RESULT_SUCCESS ) {
        /* 失敗 */

        /* エラー番号設定 */
        MLIB_SET_IFNOT_NULL( pErrNo, LIBMVFS_ERR_SERVER );

        return LIBMVFS_RET_FAILURE;
    }

    /* ファイルディスクリプタ解放 */
    FdFree( pFdInfo->localFd );

    return LIBMVFS_RET_SUCCESS;
}


/******************************************************************************/
/* ローカル関数定義                                                           */
/******************************************************************************/
/******************************************************************************/
/**
 * @brief       close応答受信
 * @details     仮想ファイルサーバからclose応答メッセージの受信を待ち合わせる。
 *
 * @param[in]   taskId  仮想ファイルサーバタスクID
 * @param[out]  *pMsg   受信メッセージバッファ
 * @param[out]  *pErrNo エラー番号
 *                  - LIBMVFS_ERR_NONE      エラー無し
 *                  - LIBMVFS_ERR_NO_MEMORY メモリ不足
 *                  - LIBMVFS_ERR_NOT_RESP  応答無し
 *                  - LIBMVFS_ERR_OTHER     その他エラー
 *
 * @return      処理結果を返す。
 * @retval      LIBMVFS_RET_SUCCESS 正常終了
 * @retval      LIBMVFS_RET_FAILURE 異常終了
 */
/******************************************************************************/
static LibMvfsRet_t ReceiveCloseResp( MkTaskId_t         taskId,
                                      MvfsMsgCloseResp_t *pMsg,
                                      uint32_t           *pErrNo )
{
    MkRet_t       retMk;    /* 戻り値               */
    MkErr_t       errMk;    /* カーネルエラー内容   */
    size_t        size;     /* 受信メッセージサイズ */
    LibMvfsRet_t  ret;      /* 戻り値               */
    MvfsMsgHdr_t  *pHeader; /* メッセージヘッダ     */
    SchedMsgBuf_t *pMsgBuf; /* メッセージバッファ   */

    /* 初期化 */
    retMk   = MK_RET_FAILURE;
    errMk   = MK_ERR_NONE;
    size    = 0;
    ret     = LIBMVFS_RET_FAILURE;
    pHeader = NULL;
    pMsgBuf = NULL;

    /* close応答受信まで繰り返し */
    while ( true ) {
        /* メッセージバッファ割当 */
        pMsgBuf = malloc( sizeof ( SchedMsgBuf_t ) );
        pHeader = ( MvfsMsgHdr_t * ) pMsgBuf->buffer;

        /* 割当結果判定 */
        if ( pMsgBuf == NULL ) {
            /* 失敗 */

            /* エラー番号設定 */
            MLIB_SET_IFNOT_NULL( pErrNo, LIBMVFS_ERR_NO_MEMORY );

            return LIBMVFS_RET_FAILURE;
        }

        /* メッセージ受信 */
        retMk = LibMkMsgReceive( taskId,                /* 受信タスクID   */
                                 pMsgBuf->buffer,       /* バッファ       */
                                 MK_MSG_SIZE_MAX,       /* バッファサイズ */
                                 NULL,                  /* 送信元タスクID */
                                 &size,                 /* 受信サイズ     */
                                 0,                     /* タイムアウト値 */
                                 &errMk           );    /* エラー番号     */

        /* 受信結果判定 */
        if ( retMk != MK_RET_SUCCESS ) {
            /* 失敗 */

            /* エラー番号判定 */
            if ( errMk == MK_ERR_NO_EXIST ) {
                /* 存在しないタスクID */

                /* エラー番号設定 */
                MLIB_SET_IFNOT_NULL( pErrNo, LIBMVFS_ERR_NOT_FOUND );

            } else if ( errMk == MK_ERR_NO_MEMORY ) {
                /* メモリ不足 */

                /* エラー番号設定 */
                MLIB_SET_IFNOT_NULL( pErrNo, LIBMVFS_ERR_NO_MEMORY );

            } else {
                /* その他エラー */

                /* エラー番号設定 */
                MLIB_SET_IFNOT_NULL( pErrNo, LIBMVFS_ERR_OTHER );
            }

            /* 戻り値設定 */
            ret = LIBMVFS_RET_FAILURE;
            break;
        }

        /* メッセージチェック */
        if ( ( pHeader->funcId != MVFS_FUNCID_CLOSE             ) ||
             ( pHeader->type   != MVFS_TYPE_RESP                ) ||
             ( size            != sizeof ( MvfsMsgCloseResp_t ) )    ) {
            /* 他メッセージ */

            /* メッセージバッファ追加 */
            SchedAddMsgBuffer( pMsgBuf );

            continue;
        }

        /* メッセージコピー */
        memcpy( pMsg, pMsgBuf->buffer, size );

        /* 戻り値設定 */
        ret = LIBMVFS_RET_SUCCESS;
        break;
    }

    /* メッセージバッファ解放 */
    free( pMsgBuf );

    return ret;
}


/******************************************************************************/
/**
 * @brief       close要求送信
 * @details     仮想ファイルサーバにclose要求メッセージを送信する。
 *
 * @param[in]   taskId   仮想ファイルサーバのタスクID
 * @param[in]   globalFd グローバルファイルディスクリプタ
 * @param[out]  *pErrNo エラー番号
 *                  - LIBMVFS_ERR_NONE      エラー無し
 *                  - LIBMVFS_ERR_NOT_FOUND 仮想ファイルサーバ不正
 *                  - LIBMVFS_ERR_NO_MEMORY メモリ不足
 *                  - LIBMVFS_ERR_OTHER     その他エラー
 *
 * @return      送信結果を返す。
 * @retval      LIBMVFS_RET_SUCCESS 正常終了
 * @retval      LIBMVFS_RET_FAILURE 異常終了
 */
/******************************************************************************/
static LibMvfsRet_t SendCloseReq( MkTaskId_t taskId,
                                  uint32_t   globalFd,
                                  uint32_t   *pErrNo   )
{
    MkRet_t           ret;  /* カーネル戻り値     */
    MkErr_t           err;  /* カーネルエラー内容 */
    MvfsMsgCloseReq_t msg;  /* 要求メッセージ     */

    /* 初期化 */
    ret = MK_RET_FAILURE;
    err = MK_ERR_NONE;
    memset( &msg, 0, sizeof ( msg ) );

    /* メッセージ作成 */
    msg.header.funcId = MVFS_FUNCID_CLOSE;
    msg.header.type   = MVFS_TYPE_REQ;
    msg.globalFd      = globalFd;

    /* メッセージ送信 */
    ret = LibMkMsgSend( taskId,             /* 送信先タスクID   */
                        &msg,               /* 送信メッセージ   */
                        sizeof ( msg ),     /* 送信メッセージ長 */
                        &err            );  /* エラー内容       */

    /* 送信結果判定 */
    if ( ret != MK_RET_SUCCESS ) {
        /* 失敗 */

        /* エラー番号判定 */
        if ( err == MK_ERR_NO_EXIST ) {
            /* 送信先不明 */

            /* エラー番号設定 */
            MLIB_SET_IFNOT_NULL( pErrNo, LIBMVFS_ERR_NOT_FOUND );

        } else if ( err == MK_ERR_NO_MEMORY ) {
            /* メモリ不足 */

            /* エラー番号設定 */
            MLIB_SET_IFNOT_NULL( pErrNo, LIBMVFS_ERR_NO_MEMORY );

        } else {
            /* その他 */

            /* エラー番号設定 */
            MLIB_SET_IFNOT_NULL( pErrNo, LIBMVFS_ERR_OTHER );
        }

        return LIBMVFS_RET_FAILURE;
    }

    return LIBMVFS_RET_SUCCESS;
}


/******************************************************************************/
