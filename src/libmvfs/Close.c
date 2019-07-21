/******************************************************************************/
/*                                                                            */
/* src/libmvfs/Close.c                                                        */
/*                                                                 2019/07/20 */
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

/* ライブラリヘッダ */
#include <libMk.h>
#include <libmvfs.h>
#include <MLib/MLib.h>

/* 共通ヘッダ */
#include <mvfs.h>

/* モジュール内ヘッダ */
#include "Fd.h"


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
    pFdInfo = FdGet( fd );

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
    FdFree( pFdInfo );

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
    int32_t  size;  /* 受信メッセージサイズ */
    uint32_t errNo; /* カーネルエラー番号   */

    /* 初期化 */
    size  = 0;
    errNo = MK_MSG_ERR_NONE;

    /* メッセージ受信 */
    size = MkMsgReceive( taskId,                            /* 受信タスクID   */
                         pMsg,                              /* バッファ       */
                         sizeof ( MvfsMsgCloseResp_t ),     /* バッファサイズ */
                         NULL,                              /* 送信元タスクID */
                         &errNo                         );  /* エラー番号     */

    /* 受信結果判定 */
    if ( size == MK_MSG_RET_FAILURE ) {
        /* 失敗 */

        /* エラー番号判定 */
        if ( errNo == MK_MSG_ERR_NO_EXIST ) {
            /* 存在しないタスクID */

            /* エラー番号設定 */
            MLIB_SET_IFNOT_NULL( pErrNo, LIBMVFS_ERR_NOT_FOUND );

        } else if ( errNo == MK_MSG_ERR_NO_MEMORY ) {
            /* メモリ不足 */

            /* エラー番号設定 */
            MLIB_SET_IFNOT_NULL( pErrNo, LIBMVFS_ERR_NO_MEMORY );

        } else {
            /* その他エラー */

            /* エラー番号設定 */
            MLIB_SET_IFNOT_NULL( pErrNo, LIBMVFS_ERR_OTHER );
        }

        return LIBMVFS_RET_FAILURE;
    }

    /* メッセージチェック */
    if ( ( pMsg->header.funcId != MVFS_FUNCID_CLOSE ) &&
         ( pMsg->header.type   != MVFS_TYPE_RESP    )    ) {
        /* メッセージ不正 */

        /* エラー番号設定 */
        MLIB_SET_IFNOT_NULL( pErrNo, LIBMVFS_ERR_NOT_RESP );

        return LIBMVFS_RET_FAILURE;
    }

    return LIBMVFS_RET_SUCCESS;
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
    int32_t           ret;      /* カーネル戻り値     */
    uint32_t          errNo;    /* カーネルエラー番号 */
    MvfsMsgCloseReq_t msg;      /* 要求メッセージ     */

    /* 初期化 */
    ret     = MK_MSG_RET_FAILURE;
    errNo   = MK_MSG_ERR_NONE;
    memset( &msg, 0, sizeof ( msg ) );

    /* メッセージ作成 */
    msg.header.funcId = MVFS_FUNCID_CLOSE;
    msg.header.type   = MVFS_TYPE_REQ;
    msg.globalFd      = globalFd;

    /* メッセージ送信 */
    ret = MkMsgSend( taskId,            /* 送信先タスクID   */
                     &msg,              /* 送信メッセージ   */
                     sizeof ( msg ),    /* 送信メッセージ長 */
                     &errNo          ); /* エラー番号       */

    /* 送信結果判定 */
    if ( ret != MK_MSG_RET_SUCCESS ) {
        /* 失敗 */

        /* エラー番号判定 */
        if ( errNo == MK_MSG_ERR_NO_EXIST ) {
            /* 送信先不明 */

            /* エラー番号設定 */
            MLIB_SET_IFNOT_NULL( pErrNo, LIBMVFS_ERR_NOT_FOUND );

        } else if ( errNo == MK_MSG_ERR_NO_MEMORY ) {
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
