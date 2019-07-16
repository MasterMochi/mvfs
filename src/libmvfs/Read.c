/******************************************************************************/
/*                                                                            */
/* src/libmvfs/Read.c                                                         */
/*                                                                 2019/07/15 */
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
/* read応答受信 */
static LibMvfsRet_t ReceiveReadResp( MkTaskId_t        taskId,
                                     MvfsMsgReadResp_t *pMsg,
                                     uint32_t          *pErrNo );
/* read要求送信 */
static LibMvfsRet_t SendReadReq( MkTaskId_t taskId,
                                 uint32_t   globalFd,
                                 uint64_t   readIdx,
                                 size_t     size,
                                 uint32_t   *pErrNo   );


/******************************************************************************/
/* ライブラリ関数定義                                                         */
/******************************************************************************/
/******************************************************************************/
/**
 * @brief       ファイル読込み
 * @details     仮想ファイルサーバにread要求メッセージを送信して、ファイルの読
 *              み込みを要求し、その応答を待ち合わせる。
 *
 * @param[in]   fd         ファイルディスクリプタ
 * @param[out]  *pBuffer   読込みバッファ
 * @param[in]   bufferSize 読込みバッファサイズ
 * @param[out]  *pReadSize 読込み実施サイズ
 * @param[out]  *pErrNo エラー番号
 *                  - LIBMVFS_ERR_NONE      エラー無し
 *                  - LIBMVFS_ERR_PARAM     パラメータ不正
 *                  - LIBMVFS_ERR_NOT_FOUND 仮想ファイルサーバが不明
 *                  - LIBMVFS_ERR_NOT_RESP  応答無し
 *                  - LIBMVFS_ERR_NO_MEMORY メモリ不足
 *                  - LIBMVFS_ERR_OTHER     その他エラー
 *
 * @return      処理結果を返す。
 * @retval      LIBMVFS_RET_SUCCESS 正常終了
 * @retval      LIBMVFS_RET_FAILURE 異常終了
 */
/******************************************************************************/
LibMvfsRet_t LibMvfsRead( uint32_t  fd,
                          void      *pBuffer,
                          size_t    bufferSize,
                          size_t    *pReadSize,
                          uint32_t  *pErrNo     )
{
    size_t            size;         /* 書込みサイズ       */
    FdInfo_t          *pFdInfo;     /* ローカルFD情報     */
    MkTaskId_t        taskId;       /* タスクID           */
    LibMvfsRet_t      ret;          /* 戻り値             */
    MvfsMsgReadResp_t *pRespMsg;    /* 応答メッセージ     */

    /* 初期化 */
    size    = 0;
    pFdInfo = NULL;
    taskId  = MK_TASKID_NULL;
    ret     = LIBMVFS_RET_FAILURE;

    /* パラメータチェック */
    if ( pBuffer == NULL ) {
        /* 不正 */

        /* エラー番号設定 */
        MLIB_SET_IFNOT_NULL( pErrNo, LIBMVFS_ERR_PARAM );

        return LIBMVFS_RET_FAILURE;
    }

    /* メッセージバッファ確保 */
    pRespMsg = malloc( sizeof ( MvfsMsgReadResp_t ) + bufferSize );

    /* 確保結果判定 */
    if ( pRespMsg == NULL ) {
        /* 失敗 */

        /* エラー番号設定 */
        MLIB_SET_IFNOT_NULL( pErrNo, LIBMVFS_ERR_NO_MEMORY );

        return LIBMVFS_RET_FAILURE;
    }

    /* メッセージバッファ初期化 */
    memset( pRespMsg, 0, sizeof ( MvfsMsgReadResp_t ) + bufferSize );

    /* タスクID取得 */
    ret = LibMvfsGetTaskId( &taskId, pErrNo );

    /* 取得結果判定 */
    if ( ret != LIBMVFS_RET_SUCCESS ) {
        /* 失敗 */

        /* メッセージバッファ解放 */
        free( pRespMsg );

        return LIBMVFS_RET_FAILURE;
    }

    /* ローカルFD情報取得 */
    pFdInfo = FdGet( fd );

    /* 取得結果判定 */
    if ( pFdInfo == NULL ) {
        /* 失敗 */

        /* メッセージバッファ解放 */
        free( pRespMsg );

        /* エラー番号設定 */
        MLIB_SET_IFNOT_NULL( pErrNo, LIBMVFS_ERR_INVALID_FD );

        return LIBMVFS_RET_FAILURE;
    }

    /* 読込み実施サイズ初期化 */
    MLIB_SET_IFNOT_NULL( pReadSize, 0 );

    /* 1度の読込み最大サイズ毎に繰り返し */
    while ( bufferSize != 0 ) {
        /* 読込み最大サイズ判定 */
        if ( bufferSize < MVFS_BUFFER_SIZE_MAX ) {
            /* 以下 */
            size = bufferSize;
        } else {
            /* 超過 */
            size = MVFS_BUFFER_SIZE_MAX;
        }

        /* read要求メッセージ送信 */
        ret = SendReadReq( taskId,
                           pFdInfo->globalFd,
                           pFdInfo->readIdx,
                           size,
                           pErrNo             );

        /* 送信結果判定 */
        if ( ret != LIBMVFS_RET_SUCCESS ) {
            /* 失敗 */

            /* メッセージバッファ解放 */
            free( pRespMsg );

            return LIBMVFS_RET_FAILURE;
        }

        /* read応答メッセージ受信 */
        ret = ReceiveReadResp( taskId, pRespMsg, pErrNo );

        /* 受信結果判定 */
        if ( ret != LIBMVFS_RET_SUCCESS ) {
            /* 失敗 */

            /* メッセージバッファ解放 */
            free( pRespMsg );
            return LIBMVFS_RET_FAILURE;
        }

        /* read処理結果判定 */
        if ( pRespMsg->result != MVFS_RESULT_SUCCESS ) {
            /* 失敗 */

            /* メッセージバッファ解放 */
            free( pRespMsg );

            /* エラー番号設定 */
            MLIB_SET_IFNOT_NULL( pErrNo, LIBMVFS_ERR_SERVER );

            return LIBMVFS_RET_FAILURE;
        }

        /* 読込みバッファコピー */
        memcpy( pBuffer, pRespMsg->pBuffer, pRespMsg->size );

        /* 読込みインデックス・サイズ更新 */
        pFdInfo->readIdx += pRespMsg->size;
        bufferSize       -= pRespMsg->size;
        pBuffer           = ( ( char * ) pBuffer ) + pRespMsg->size;

        /* 読込み実施サイズ設定 */
        MLIB_SET_IFNOT_NULL( pReadSize, *pReadSize + pRespMsg->size );
    }

    /* メッセージバッファ解放 */
    free( pRespMsg );

    return LIBMVFS_RET_SUCCESS;
}


/******************************************************************************/
/* ローカル関数定義                                                           */
/******************************************************************************/
/******************************************************************************/
/**
 * @brief       read応答受信
 * @details     仮想ファイルサーバからread応答メッセージの受信を待ち合わせる。
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
static LibMvfsRet_t ReceiveReadResp( MkTaskId_t        taskId,
                                     MvfsMsgReadResp_t *pMsg,
                                     uint32_t          *pErrNo )
{
    int32_t  size;  /* 受信メッセージサイズ */
    uint32_t errNo; /* カーネルエラー番号   */

    /* 初期化 */
    size  = 0;
    errNo = MK_MSG_ERR_NONE;

    /* メッセージ受信 */
    size = MkMsgReceive( taskId,                /* 受信タスクID   */
                         pMsg,                  /* バッファ       */
                         MK_MSG_SIZE_MAX,       /* バッファサイズ */
                         NULL,                  /* 送信元タスクID */
                         &errNo           );    /* エラー番号     */

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
    if ( ( pMsg->header.funcId != MVFS_FUNCID_READ ) &&
         ( pMsg->header.type   != MVFS_TYPE_RESP   )    ) {
        /* メッセージ不正 */

        /* エラー番号設定 */
        MLIB_SET_IFNOT_NULL( pErrNo, LIBMVFS_ERR_NOT_RESP );

        return LIBMVFS_RET_FAILURE;
    }

    return LIBMVFS_RET_SUCCESS;
}


/******************************************************************************/
/**
 * @brief       read要求送信
 * @details     仮想ファイルサーバにread要求メッセージを送信する。
 *
 * @param[in]   taskId   仮想ファイルサーバのタスクID
 * @param[in]   globalFd グローバルファイルディスクリプタ
 * @param[in]   readIdx  読込みインデックス
 * @param[in]   size     読込みサイズ
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
static LibMvfsRet_t SendReadReq( MkTaskId_t taskId,
                                 uint32_t   globalFd,
                                 uint64_t   readIdx,
                                 size_t     size,
                                 uint32_t   *pErrNo   )
{
    int32_t          ret;   /* カーネル戻り値     */
    uint32_t         errNo; /* カーネルエラー番号 */
    MvfsMsgReadReq_t msg;   /* 要求メッセージ     */

    /* 初期化 */
    ret     = MK_MSG_RET_FAILURE;
    errNo   = MK_MSG_ERR_NONE;
    memset( &msg, 0, sizeof ( msg ) );

    /* メッセージ作成 */
    msg.header.funcId = MVFS_FUNCID_READ;
    msg.header.type   = MVFS_TYPE_REQ;
    msg.globalFd      = globalFd;
    msg.readIdx       = readIdx;
    msg.size          = size;

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
