/******************************************************************************/
/*                                                                            */
/* src/libmvfs/Open.c                                                         */
/*                                                                 2019/07/07 */
/* Copyright (C) 2019 Mochi.                                                  */
/*                                                                            */
/******************************************************************************/
/******************************************************************************/
/* インクルード                                                               */
/******************************************************************************/
/* 標準ヘッダ */
#include <stdbool.h>
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
/* open応答受信 */
static LibMvfsRet_t ReceiveOpenResp( MkTaskId_t        taskId,
                                     MvfsMsgOpenResp_t *pMsg,
                                     uint32_t          *pErrNo );
/* open要求送信 */
static LibMvfsRet_t SendOpenReq( MkTaskId_t taskId,
                                 const char *pPath,
                                 uint32_t   localFd,
                                 uint32_t   *pErrNo  );


/******************************************************************************/
/* ライブラリ関数定義                                                         */
/******************************************************************************/
/******************************************************************************/
/**
 * @brief       ファイルオープン
 * @details     仮想ファイルサーバにopen要求メッセージを送信して、ファイルの
 *              オープンを要求し、その応答を待ち合わせる。
 *
 * @param[out]  *pFd    ファイルディスクリプタ
 * @param[in]   *pPath  ファイルパス(絶対パス)
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
LibMvfsRet_t LibMvfsOpen( uint32_t   *pLocalFd,
                          const char *pPath,
                          uint32_t   *pErrNo    )
{
    FdInfo_t          *pFdInfo; /* ローカルFD情報 */
    MkTaskId_t        taskId;   /* タスクID       */
    LibMvfsRet_t      ret;      /* 戻り値         */
    MvfsMsgOpenResp_t respMsg;  /* 応答メッセージ */

    /* 初期化 */
    pFdInfo = NULL;
    taskId  = MK_TASKID_NULL;
    ret     = LIBMVFS_RET_FAILURE;
    memset( &respMsg, 0, sizeof ( respMsg ) );

    /* パラメータチェック */
    if ( pPath == NULL ) {
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

    /* ローカルFD割当 */
    pFdInfo = FdAlloc();

    /* 割当結果判定 */
    if ( pFdInfo == NULL ) {
        /* 失敗 */

        /* エラー番号設定 */
        MLIB_SET_IFNOT_NULL( pErrNo, LIBMVFS_ERR_NO_MEMORY );

        return LIBMVFS_RET_FAILURE;
    }

    /* open要求メッセージ送信 */
    ret = SendOpenReq( taskId, pPath, pFdInfo->localFd, pErrNo );

    /* 送信結果判定 */
    if ( ret != LIBMVFS_RET_SUCCESS ) {
        /* 失敗 */

        /* ローカルFD解放 */
        FdFree( pFdInfo );

        return LIBMVFS_RET_FAILURE;
    }

    /* open応答メッセージ受信 */
    ret = ReceiveOpenResp( taskId, &respMsg, pErrNo );

    /* 受信結果判定 */
    if ( ret != LIBMVFS_RET_SUCCESS ) {
        /* 失敗 */

        /* ローカルFD解放 */
        FdFree( pFdInfo );

        return LIBMVFS_RET_FAILURE;
    }

    /* open処理結果判定 */
    if ( respMsg.result != MVFS_RESULT_SUCCESS ) {
        /* 失敗 */

        /* ローカルFD解放 */
        FdFree( pFdInfo );

        /* エラー番号設定 */
        MLIB_SET_IFNOT_NULL( pErrNo, LIBMVFS_ERR_SERVER );

        return LIBMVFS_RET_FAILURE;
    }

    /* グローバルFD設定 */
    pFdInfo->globalFd = respMsg.globalFd;

    /* ローカルFD設定 */
    *pLocalFd = pFdInfo->localFd;

    return LIBMVFS_RET_SUCCESS;
}


/******************************************************************************/
/* ローカル関数定義                                                           */
/******************************************************************************/
/******************************************************************************/
/**
 * @brief       open応答受信
 * @details     仮想ファイルサーバからopen応答メッセージの受信を待ち合わせる。
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
static LibMvfsRet_t ReceiveOpenResp( MkTaskId_t        taskId,
                                     MvfsMsgOpenResp_t *pMsg,
                                     uint32_t          *pErrNo )
{
    int32_t  size;  /* 受信メッセージサイズ */
    uint32_t errNo; /* カーネルエラー番号   */

    /* 初期化 */
    size  = 0;
    errNo = MK_MSG_ERR_NONE;

    /* メッセージ受信 */
    size = MkMsgReceive( taskId,                            /* 受信タスクID   */
                         pMsg,                              /* バッファ       */
                         sizeof ( MvfsMsgOpenResp_t ),      /* バッファサイズ */
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
    if ( size != sizeof ( MvfsMsgOpenResp_t ) ) {
        /* 不正 */

        /* エラー番号設定 */
        MLIB_SET_IFNOT_NULL( pErrNo, LIBMVFS_ERR_NOT_RESP );

        return LIBMVFS_RET_FAILURE;
    }

    /* メッセージチェック */
    if ( ( pMsg->header.funcId != MVFS_FUNCID_OPEN ) &&
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
 * @brief       open要求送信
 * @details     仮想ファイルサーバにopen要求メッセージを送信する。
 *
 * @param[in]   taskId  仮想ファイルサーバのタスクID
 * @param[in]   *pPath  ファイルパス(絶対パス)
 * @param[in]   localFd ローカルファイルディスクリプタ
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
static LibMvfsRet_t SendOpenReq( MkTaskId_t taskId,
                                 const char *pPath,
                                 uint32_t   localFd,
                                 uint32_t   *pErrNo  )
{
    int32_t          ret;    /* カーネル戻り値     */
    uint32_t         errNo;  /* カーネルエラー番号 */
    MvfsMsgOpenReq_t msg;    /* 要求メッセージ     */

    /* 初期化 */
    ret   = MK_MSG_RET_FAILURE;
    errNo = MK_MSG_ERR_NONE;
    memset( &msg, 0, sizeof ( msg ) );

    /* メッセージ作成 */
    msg.header.funcId = MVFS_FUNCID_OPEN;
    msg.header.type   = MVFS_TYPE_REQ;
    msg.localFd       = localFd;
    strncpy( msg.path, pPath, MVFS_PATH_MAXLEN );

    /* メッセージ送信 */
    ret = MkMsgSend( taskId,                /* 送信先タスクID   */
                     &msg,                  /* 送信メッセージ   */
                     sizeof ( msg ),        /* 送信メッセージ長 */
                     &errNo          );     /* エラー番号       */

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
