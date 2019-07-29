/******************************************************************************/
/*                                                                            */
/* src/libmvfs/Send.c                                                         */
/*                                                                 2019/07/28 */
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
/* vfsClose応答送信 */
static LibMvfsRet_t SendVfsCloseResp( MkTaskId_t taskId,
                                      uint32_t   globalFd,
                                      uint32_t   result,
                                      uint32_t   *pErrNo   );
/* vfsOpen応答送信 */
static LibMvfsRet_t SendVfsOpenResp( MkTaskId_t taskId,
                                     uint32_t   result,
                                     uint32_t   *pErrNo );
/* vfsWrite応答送信 */
static LibMvfsRet_t SendVfsWriteResp( MkTaskId_t taskId,
                                      uint32_t   globalFd,
                                      uint32_t   result,
                                      size_t     size,
                                      uint32_t   *pErrNo   );
/* vfsRead応答送信 */
static LibMvfsRet_t SendVfsReadResp( MkTaskId_t taskId,
                                     uint32_t   globalFd,
                                     uint32_t   result,
                                     void       *pBuffer,
                                     size_t     size,
                                     uint32_t   *pErrNo   );


/******************************************************************************/
/* ライブラリ関数定義                                                         */
/******************************************************************************/
/******************************************************************************/
/**
 * @brief       vfsClose応答送信
 * @details     仮想ファイルサーバにvfsClose応答メッセージを送信する。
 *
 * @param[in]   globalFd グローバルファイルディスクリプタ
 * @param[in]   result   処理結果
 *                  - MVFS_RESULT_SUCCESS 処理成功
 *                  - MVFS_RESULT_FAILURE 処理失敗
 * @param[out]  *pErrNo  エラー番号
 *                  - LIBMVFS_ERR_NONE      エラー無し
 *                  - LIBMVFS_ERR_NOT_FOUND 仮想ファイルサーバが不明
 *                  -
 *
 * @return      送信結果を返す。
 * @retval      LIBMVFS_RET_SUCCESS 正常終了
 * @retval      LIBMVFS_RET_FAILURE 異常終了
 */
/******************************************************************************/
LibMvfsRet_t LibMvfsSendVfsCloseResp( uint32_t globalFd,
                                      uint32_t result,
                                      uint32_t *pErrNo   )
{
    int32_t    ret;     /* 関数戻り値 */
    MkTaskId_t taskId;  /* タスクID   */

    /* 初期化 */
    ret    = LIBMVFS_RET_FAILURE;
    taskId = MK_TASKID_NULL;

    /* パラメータチェック */
    if ( ( result != MVFS_RESULT_SUCCESS ) &&
         ( result != MVFS_RESULT_FAILURE )    ) {
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

    /* vfsClose応答メッセージ送信 */
    ret = SendVfsCloseResp( taskId, globalFd, result, pErrNo );

    return ret;
}


/******************************************************************************/
/**
 * @brief       vfsOpen応答送信
 * @details     仮想ファイルサーバにvfsOpen応答メッセージを送信する。
 *
 * @param[in]   result  処理結果
 *                  - MVFS_RESULT_SUCCESS 処理成功
 *                  - MVFS_RESULT_FAILURE 処理失敗
 * @param[out]  *pErrNo エラー番号
 *                  - LIBMVFS_ERR_NONE      エラー無し
 *                  - LIBMVFS_ERR_NOT_FOUND 仮想ファイルサーバが不明
 *                  -
 *
 * @return      送信結果を返す。
 * @retval      LIBMVFS_RET_SUCCESS 正常終了
 * @retval      LIBMVFS_RET_FAILURE 異常終了
 */
/******************************************************************************/
LibMvfsRet_t LibMvfsSendVfsOpenResp( uint32_t result,
                                     uint32_t *pErrNo )
{
    int32_t    ret;     /* 関数戻り値 */
    MkTaskId_t taskId;  /* タスクID   */

    /* 初期化 */
    ret    = LIBMVFS_RET_FAILURE;
    taskId = MK_TASKID_NULL;

    /* パラメータチェック */
    if ( ( result != MVFS_RESULT_SUCCESS ) &&
         ( result != MVFS_RESULT_FAILURE )    ) {
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

    /* vfsOpen応答メッセージ送信 */
    ret = SendVfsOpenResp( taskId, result, pErrNo );

    return ret;
}


/******************************************************************************/
/**
 * @brief       vfsWrite応答送信
 * @details     仮想ファイルサーバにvfsWrite応答メッセージを送信する。
 *
 * @param[in]   globalFd グローバルファイルディスクリプタ
 * @param[in]   result   処理結果
 *                  - MVFS_RESULT_SUCCESS 処理成功
 *                  - MVFS_RESULT_FAILURE 処理失敗
 * @param[in]   size     書込み実施サイズ
 * @param[out]  *pErrNo  エラー番号
 *                  - LIBMVFS_ERR_NONE      エラー無し
 *                  - LIBMVFS_ERR_PARAM     パラメータ不正
 *                  - LIBMVFS_ERR_NOT_FOUND 仮想ファイルサーバが不明
 *
 * @return      送信結果を返す。
 * @retval      LIBMVFS_RET_SUCCESS 正常終了
 * @retval      LIBMVFS_RET_FAILURE 異常終了
 */
/******************************************************************************/
LibMvfsRet_t LibMvfsSendVfsWriteResp( uint32_t globalFd,
                                      uint32_t result,
                                      size_t   size,
                                      uint32_t *pErrNo )
{
    int32_t    ret;     /* 関数戻り値 */
    MkTaskId_t taskId;  /* タスクID   */

    /* 初期化 */
    ret    = LIBMVFS_RET_FAILURE;
    taskId = MK_TASKID_NULL;

    /* パラメータチェック */
    if ( ( result != MVFS_RESULT_SUCCESS ) &&
         ( result != MVFS_RESULT_FAILURE )    ) {
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

    /* vfsWrite応答メッセージ送信 */
    ret = SendVfsWriteResp( taskId, globalFd, result, size, pErrNo );

    return ret;
}


/******************************************************************************/
/**
 * @brief       vfsRead応答送信
 * @details     仮想ファイルサーバにvfsRead応答メッセージを送信する。
 *
 * @param[in]   globalFd グローバルファイルディスクリプタ
 * @param[in]   result   処理結果
 *                  - MVFS_RESULT_SUCCESS 処理成功
 *                  - MVFS_RESULT_FAILURE 処理失敗
 * @param[in]   *pBuffer 読込みバッファ
 * @param[in]   size     読込み実施サイズ
 * @param[out]  *pErrNo  エラー番号
 *                  - LIBMVFS_ERR_NONE      エラー無し
 *                  - LIBMVFS_ERR_PARAM     パラメータ不正
 *                  - LIBMVFS_ERR_NOT_FOUND 仮想ファイルサーバが不明
 *
 * @return      送信結果を返す。
 * @retval      LIBMVFS_RET_SUCCESS 正常終了
 * @retval      LIBMVFS_RET_FAILURE 異常終了
 */
/******************************************************************************/
LibMvfsRet_t LibMvfsSendVfsReadResp( uint32_t globalFd,
                                     uint32_t result,
                                     void     *pBuffer,
                                     size_t   size,
                                     uint32_t *pErrNo   )
{
    int32_t    ret;     /* 関数戻り値 */
    MkTaskId_t taskId;  /* タスクID   */

    /* 初期化 */
    ret    = LIBMVFS_RET_FAILURE;
    taskId = MK_TASKID_NULL;

    /* パラメータチェック */
    if ( ( result != MVFS_RESULT_SUCCESS ) &&
         ( result != MVFS_RESULT_FAILURE )    ) {
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

    /* vfsRead応答メッセージ送信 */
    ret = SendVfsReadResp( taskId, globalFd, result, pBuffer, size, pErrNo );

    return ret;
}


/******************************************************************************/
/* ローカル関数定義                                                           */
/******************************************************************************/
/******************************************************************************/
/**
 * @brief       vfsClose応答送信
 * @details     仮想ファイルサーバにvfsClose応答メッセージを送信する。
 *
 * @param[in]   taskId   仮想ファイルサーバのタスクID
 * @param[in]   globalFd グローバルファイルディスクリプタ
 * @param[in]   result   処理結果
 *                  - MVFS_RESULT_SUCCESS 処理成功
 *                  - MVFS_RESULT_FAILURE 処理失敗
 * @param[in]   *pErrNo  エラー番号
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
static LibMvfsRet_t SendVfsCloseResp( MkTaskId_t taskId,
                                      uint32_t   globalFd,
                                      uint32_t   result,
                                      uint32_t   *pErrNo   )
{
    MkRet_t               ret;      /* カーネル戻り値     */
    MkErr_t               err;      /* カーネルエラー内容 */
    MvfsMsgVfsCloseResp_t msg;      /* 応答メッセージ     */

    /* 初期化 */
    ret = MK_RET_FAILURE;
    err = MK_ERR_NONE;
    memset( &msg, 0, sizeof ( msg ) );

    /* メッセージ作成 */
    msg.header.funcId = MVFS_FUNCID_VFSCLOSE;
    msg.header.type   = MVFS_TYPE_RESP;
    msg.globalFd      = globalFd;
    msg.result        = result;

    /* メッセージ送信 */
    ret = LibMkMsgSend( taskId,            /* 送信先タスクID   */
                        &msg,              /* 送信メッセージ   */
                        sizeof ( msg ),    /* 送信メッセージ長 */
                        &err            ); /* エラー内容       */

    /* 送信結果判定 */
    if ( ret != MK_RET_SUCCESS ) {
        /* 失敗 */

        /* エラー判定 */
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
/**
 * @brief       vfsOpen応答送信
 * @details     仮想ファイルサーバにvfsOpen応答メッセージを送信する。
 *
 * @param[in]   taskId  仮想ファイルサーバのタスクID
 * @param[in]   result  処理結果
 *                  - MVFS_RESULT_SUCCESS 処理成功
 *                  - MVFS_RESULT_FAILURE 処理失敗
 * @param[in]   *pErrNo エラー番号
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
static LibMvfsRet_t SendVfsOpenResp( MkTaskId_t taskId,
                                     uint32_t   result,
                                     uint32_t   *pErrNo )
{
    MkRet_t              ret;   /* カーネル戻り値     */
    MkErr_t              err;   /* カーネルエラー内容 */
    MvfsMsgVfsOpenResp_t msg;   /* 応答メッセージ     */

    /* 初期化 */
    ret = MK_RET_FAILURE;
    err = MK_ERR_NONE;
    memset( &msg, 0, sizeof ( msg ) );

    /* メッセージ作成 */
    msg.header.funcId = MVFS_FUNCID_VFSOPEN;
    msg.header.type   = MVFS_TYPE_RESP;
    msg.result        = result;

    /* メッセージ送信 */
    ret = LibMkMsgSend( taskId,            /* 送信先タスクID   */
                        &msg,              /* 送信メッセージ   */
                        sizeof ( msg ),    /* 送信メッセージ長 */
                        &err            ); /* エラー内容       */

    /* 送信結果判定 */
    if ( ret != MK_RET_SUCCESS ) {
        /* 失敗 */

        /* エラー判定 */
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
/**
 * @brief       vfsWrite応答送信
 * @details     仮想ファイルサーバにvfsWrite応答メッセージを送信する。
 *
 * @param[in]   taskId   仮想ファイルサーバのタスクID
 * @param[in]   globalFd グローバルファイルディスクリプタ
 * @param[in]   result   処理結果
 *                  - MVFS_RESULT_SUCCESS 処理成功
 *                  - MVFS_RESULT_FAILURE 処理失敗
 * @param[in]   size     書込み実施サイズ
 * @param[in]   *pErrNo  エラー番号
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
static LibMvfsRet_t SendVfsWriteResp( MkTaskId_t taskId,
                                      uint32_t   globalFd,
                                      uint32_t   result,
                                      size_t     size,
                                      uint32_t   *pErrNo   )
{
    MkRet_t               ret;      /* カーネル戻り値     */
    MkErr_t               err;      /* カーネルエラー内容 */
    MvfsMsgVfsWriteResp_t msg;      /* 応答メッセージ     */

    /* 初期化 */
    ret = MK_RET_FAILURE;
    err = MK_ERR_NONE;
    memset( &msg, 0, sizeof ( msg ) );

    /* メッセージ作成 */
    msg.header.funcId = MVFS_FUNCID_VFSWRITE;
    msg.header.type   = MVFS_TYPE_RESP;
    msg.globalFd      = globalFd;
    msg.result        = result;
    msg.size          = size;

    /* メッセージ送信 */
    ret = LibMkMsgSend( taskId,            /* 送信先タスクID   */
                        &msg,              /* 送信メッセージ   */
                        sizeof ( msg ),    /* 送信メッセージ長 */
                        &err            ); /* エラー内容       */

    /* 送信結果判定 */
    if ( ret != MK_RET_SUCCESS ) {
        /* 失敗 */

        /* エラー判定 */
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
/**
 * @brief       vfsRead応答送信
 * @details     仮想ファイルサーバにvfsRead応答メッセージを送信する。
 *
 * @param[in]   taskId   仮想ファイルサーバのタスクID
 * @param[in]   globalFd グローバルファイルディスクリプタ
 * @param[in]   result   処理結果
 *                  - MVFS_RESULT_SUCCESS 処理成功
 *                  - MVFS_RESULT_FAILURE 処理失敗
 * @param[in]   *pBuffer 読込みバッファ
 * @param[in]   size     読込み実施サイズ
 * @param[in]   *pErrNo  エラー番号
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
static LibMvfsRet_t SendVfsReadResp( MkTaskId_t taskId,
                                     uint32_t   globalFd,
                                     uint32_t   result,
                                     void       *pBuffer,
                                     size_t     size,
                                     uint32_t   *pErrNo   )
{
    MkRet_t              ret;   /* カーネル戻り値     */
    MkErr_t              err;   /* カーネルエラー内容 */
    MvfsMsgVfsReadResp_t *pMsg; /* 応答メッセージ     */

    /* 初期化 */
    ret = MK_RET_FAILURE;
    err = MK_ERR_NONE;

    /* バッファ確保 */
    pMsg = malloc( sizeof ( MvfsMsgVfsReadResp_t ) + size );

    /* 確保結果判定 */
    if ( pMsg == NULL ) {
        /* 失敗 */

        /* エラー番号設定 */
        MLIB_SET_IFNOT_NULL( pErrNo, LIBMVFS_ERR_NO_MEMORY );

        return LIBMVFS_RET_FAILURE;
    }

    /* バッファ初期化 */
    memset( pMsg, 0, sizeof ( MvfsMsgVfsReadResp_t ) + size );

    /* メッセージ作成 */
    pMsg->header.funcId = MVFS_FUNCID_VFSREAD;
    pMsg->header.type   = MVFS_TYPE_RESP;
    pMsg->globalFd      = globalFd;
    pMsg->result        = result;
    pMsg->size          = size;

    /* 読込みバッファ有効チェック */
    if ( ( pBuffer != NULL ) && ( size != 0 ) ) {
        /* 有効 */

        /* 読込みバッファコピー */
        memcpy( pMsg->pBuffer, pBuffer, size );
    }

    /* メッセージ送信 */
    ret = LibMkMsgSend(
              taskId,                                   /* 送信先タスクID   */
              pMsg,                                     /* 送信メッセージ   */
              sizeof ( MvfsMsgVfsReadResp_t ) + size,   /* 送信メッセージ長 */
              &err                                    );/* エラー内容       */

    /* 送信結果判定 */
    if ( ret != MK_RET_SUCCESS ) {
        /* 失敗 */

        /* エラー判定 */
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

        /* バッファ解放 */
        free( pMsg );

        return LIBMVFS_RET_FAILURE;
    }

    /* バッファ解放 */
    free( pMsg );

    return LIBMVFS_RET_SUCCESS;
}


/******************************************************************************/
