/******************************************************************************/
/*                                                                            */
/* src/libmvfs/Write.c                                                        */
/*                                                                 2019/07/11 */
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
/* write応答受信 */
static LibMvfsRet_t ReceiveWriteResp( MkTaskId_t         taskId,
                                      MvfsMsgWriteResp_t *pMsg,
                                      uint32_t           *pErrNo );
/* write要求送信 */
static LibMvfsRet_t SendWriteReq( MkTaskId_t taskId,
                                  uint32_t   globalFd,
                                  uint64_t   writeIdx,
                                  void       *pBuffer,
                                  size_t     size,
                                  uint32_t   *pErrNo   );


/******************************************************************************/
/* ライブラリ関数定義                                                         */
/******************************************************************************/
/******************************************************************************/
/**
 * @brief       ファイル書き込み
 * @details     仮想ファイルサーバにwrite要求メッセージを送信して、ファイルの
 *              書き込みを要求し、その応答を待ち合わせる。
 *
 * @param[in]   fd          ファイルディスクリプタ
 * @param[in]   *pBuffer    書込みバッファ
 * @param[in]   bufferSize  書込みバッファサイズ
 * @param[out]  *pWriteSize 書込み実施サイズ
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
LibMvfsRet_t LibMvfsWrite( uint32_t  fd,
                           void      *pBuffer,
                           size_t    bufferSize,
                           size_t    *pWriteSize,
                           uint32_t  *pErrNo      )
{
    size_t             size;     /* 書込みサイズ   */
    FdInfo_t           *pFdInfo; /* ローカルFD情報 */
    MkTaskId_t         taskId;   /* タスクID       */
    LibMvfsRet_t       ret;      /* 戻り値         */
    MvfsMsgWriteResp_t respMsg;  /* 応答メッセージ */

    /* 初期化 */
    size    = 0;
    pFdInfo = NULL;
    taskId  = MK_TASKID_NULL;
    ret     = LIBMVFS_RET_FAILURE;
    memset( &respMsg, 0, sizeof ( respMsg ) );

    /* パラメータチェック */
    if ( pBuffer == NULL ) {
        /* 不正 */

        /* エラー番号設定 */
        MLIB_SET_IFNOT_NULL( pErrNo, LIBMVFS_ERR_PARAM );

        return LIBMVFS_RET_FAILURE;
    }

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

    /* 書込み実施サイズ初期化 */
    MLIB_SET_IFNOT_NULL( pWriteSize, 0 );

    /* 1度の書込み最大サイズ毎に繰り返し */
    while ( bufferSize != 0 ) {
        /* 書込み最大サイズ判定 */
        if ( bufferSize < MVFS_BUFFER_SIZE_MAX ) {
            /* 以下 */
            size = bufferSize;
        } else {
            /* 超過 */
            size = MVFS_BUFFER_SIZE_MAX;
        }

        /* write要求メッセージ送信 */
        ret = SendWriteReq( taskId,
                            pFdInfo->globalFd,
                            pFdInfo->writeIdx,
                            pBuffer,
                            size,
                            pErrNo             );

        /* 送信結果判定 */
        if ( ret != LIBMVFS_RET_SUCCESS ) {
            /* 失敗 */

            return LIBMVFS_RET_FAILURE;
        }

        /* write応答メッセージ受信 */
        ret = ReceiveWriteResp( taskId, &respMsg, pErrNo );

        /* 受信結果判定 */
        if ( ret != LIBMVFS_RET_SUCCESS ) {
            /* 失敗 */

            return LIBMVFS_RET_FAILURE;
        }

        /* write処理結果判定 */
        if ( respMsg.result != MVFS_RESULT_SUCCESS ) {
            /* 失敗 */

            /* エラー番号設定 */
            MLIB_SET_IFNOT_NULL( pErrNo, LIBMVFS_ERR_SERVER );

            return LIBMVFS_RET_FAILURE;
        }

        /* 書込みインデックス更新 */
        pFdInfo->writeIdx += respMsg.size;
        pBuffer = ( char * ) pBuffer + respMsg.size;

        /* 書込み実施サイズ設定 */
        MLIB_SET_IFNOT_NULL( pWriteSize, *pWriteSize + respMsg.size );
        bufferSize -= respMsg.size;
    }

    return LIBMVFS_RET_SUCCESS;
}


/******************************************************************************/
/* ローカル関数定義                                                           */
/******************************************************************************/
/******************************************************************************/
/**
 * @brief       write応答受信
 * @details     仮想ファイルサーバからwrite応答メッセージの受信を待ち合わせる。
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
static LibMvfsRet_t ReceiveWriteResp( MkTaskId_t         taskId,
                                      MvfsMsgWriteResp_t *pMsg,
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
                         sizeof ( MvfsMsgWriteResp_t ),     /* バッファサイズ */
                         NULL,                              /* 送信元タスクID */
                         &errNo                        );   /* エラー番号     */

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

    /* 受信サイズチェック */
    if ( size != sizeof ( MvfsMsgWriteResp_t ) ) {
        /* 不正 */

        /* エラー番号設定 */
        MLIB_SET_IFNOT_NULL( pErrNo, LIBMVFS_ERR_NOT_RESP );

        return LIBMVFS_RET_FAILURE;
    }

    /* メッセージチェック */
    if ( ( pMsg->header.funcId != MVFS_FUNCID_WRITE ) &&
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
 * @brief       write要求送信
 * @details     仮想ファイルサーバにwrite要求メッセージを送信する。
 *
 * @param[in]   taskId   仮想ファイルサーバのタスクID
 * @param[in]   globalFd グローバルファイルディスクリプタ
 * @param[in]   writeIdx 書込みインデックス
 * @param[in]   *pBuffer 書込みデータ
 * @param[in]   size     書込みサイズ
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
static LibMvfsRet_t SendWriteReq( MkTaskId_t taskId,
                                  uint32_t   globalFd,
                                  uint64_t   writeIdx,
                                  void       *pBuffer,
                                  size_t     size,
                                  uint32_t   *pErrNo   )
{
    char              buffer[ MK_MSG_SIZE_MAX ];    /* メッセージバッファ */
    size_t            msgSize;                      /* メッセージサイズ   */
    int32_t           ret;                          /* カーネル戻り値     */
    uint32_t          errNo;                        /* カーネルエラー番号 */
    MvfsMsgWriteReq_t *pMsg;                        /* 要求メッセージ     */

    /* 初期化 */
    msgSize = sizeof ( MvfsMsgWriteReq_t ) + size;
    ret     = MK_MSG_RET_FAILURE;
    errNo   = MK_MSG_ERR_NONE;
    pMsg    = ( MvfsMsgWriteReq_t * ) &buffer;
    memset( &buffer, 0, sizeof ( buffer ) );

    /* メッセージ作成 */
    pMsg->header.funcId = MVFS_FUNCID_WRITE;
    pMsg->header.type   = MVFS_TYPE_REQ;
    pMsg->globalFd      = globalFd;
    pMsg->writeIdx      = writeIdx;
    pMsg->size          = size;
    memcpy( pMsg->pBuffer, pBuffer, size );

    /* メッセージ送信 */
    ret = MkMsgSend( taskId,        /* 送信先タスクID   */
                     pMsg,          /* 送信メッセージ   */
                     msgSize,       /* 送信メッセージ長 */
                     &errNo   );    /* エラー番号       */

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
