/******************************************************************************/
/*                                                                            */
/* src/libmvfs/Send.c                                                         */
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

/* カーネルヘッダ */
#include <kernel/types.h>

/* ライブラリヘッダ */
#include <libMk.h>
#include <libmvfs.h>
#include <MLib/MLib.h>

/* 共通ヘッダ */
#include <mvfs.h>


/******************************************************************************/
/* ローカル関数宣言                                                           */
/******************************************************************************/
/* vfsOpen応答送信 */
static LibMvfsRet_t SendVfsOpenResp( MkTaskId_t taskId,
                                     uint32_t   result,
                                     uint32_t   *pErrNo );


/******************************************************************************/
/* ライブラリ関数定義                                                         */
/******************************************************************************/
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
/* ローカル関数定義                                                           */
/******************************************************************************/
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
    int32_t              ret;   /* カーネル戻り値     */
    uint32_t             errNo; /* カーネルエラー番号 */
    MvfsMsgVfsOpenResp_t msg;   /* 応答メッセージ     */

    /* 初期化 */
    ret   = MK_MSG_RET_FAILURE;
    errNo = MK_MSG_ERR_NONE;
    memset( &msg, 0, sizeof ( msg ) );

    /* メッセージ作成 */
    msg.header.funcId = MVFS_FUNCID_VFSOPEN;
    msg.header.type   = MVFS_TYPE_RESP;
    msg.result        = result;

    /* メッセージ送信 */
    ret = MkMsgSend( taskId,            /* 送信先タスクID   */
                     &msg,              /* 送信メッセージ   */
                     sizeof ( msg ),    /* 送信メッセージ長 */
                     &errNo          ); /* エラー番号       */

    /* 送信結果判定 */
    if ( ret != MK_MSG_RET_FAILURE ) {
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
