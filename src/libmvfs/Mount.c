/******************************************************************************/
/*                                                                            */
/* src/libmvfs/Mount.c                                                        */
/*                                                                 2020/04/30 */
/* Copyright (C) 2019-2020 Mochi.                                             */
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
#include <libmk.h>
#include <libmvfs.h>
#include <MLib/MLib.h>

/* 共通ヘッダ */
#include <mvfs.h>


/******************************************************************************/
/* ローカル関数宣言                                                           */
/******************************************************************************/
/* mount応答受信 */
static LibMvfsRet_t ReceiveMountResp( MkTaskId_t         taskId,
                                      MvfsMsgMountResp_t *pMsg,
                                      uint32_t           *pErrNo );
/* mount要求送信 */
static LibMvfsRet_t SendMountReq( MkTaskId_t taskId,
                                  const char *pPath,
                                  uint32_t   *pErrNo );


/******************************************************************************/
/* ライブラリ関数定義                                                         */
/******************************************************************************/
/******************************************************************************/
/**
 * @brief       マウントファイル作成
 * @details     仮想ファイルサーバにmount要求メッセージを送信して、マウントファ
 *              イルの作成を要求し、その応答を待ち合わせる。
 *
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
LibMvfsRet_t LibMvfsMount( const char *pPath,
                           uint32_t   *pErrNo )
{
    MkTaskId_t         taskId;  /* タスクID       */
    LibMvfsRet_t       ret;     /* 戻り値         */
    MvfsMsgMountResp_t respMsg; /* 応答メッセージ */

    /* 初期化 */
    taskId = MK_TASKID_NULL;
    ret    = LIBMVFS_RET_FAILURE;
    memset( &respMsg, 0, sizeof ( MvfsMsgMountResp_t ) );

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

    /* mount要求メッセージ送信 */
    ret = SendMountReq( taskId, pPath, pErrNo );

    /* 送信結果判定 */
    if ( ret != LIBMVFS_RET_SUCCESS ) {
        /* 失敗 */

        return LIBMVFS_RET_FAILURE;
    }

    /* mount応答メッセージ受信 */
    ret = ReceiveMountResp( taskId,
                            &respMsg,
                            pErrNo    );

    /* 受信結果判定 */
    if ( ret != LIBMVFS_RET_SUCCESS ) {
        /* 失敗 */

        return LIBMVFS_RET_FAILURE;
    }

    /* open処理結果判定 */
    if ( respMsg.result != MVFS_RESULT_SUCCESS ) {
        /* 失敗 */

        /* エラー番号設定 */
        MLIB_SET_IFNOT_NULL( pErrNo, LIBMVFS_ERR_SERVER );

        return LIBMVFS_RET_FAILURE;
    }

    return LIBMVFS_RET_SUCCESS;
}


/******************************************************************************/
/* ローカル関数定義                                                           */
/******************************************************************************/
/******************************************************************************/
/**
 * @brief       mount応答受信
 * @details     仮想ファイルサーバからmount応答メッセージの受信を待ち合わせる。
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
static LibMvfsRet_t ReceiveMountResp( MkTaskId_t         taskId,
                                      MvfsMsgMountResp_t *pMsg,
                                      uint32_t           *pErrNo )
{
    MkRet_t ret;    /* カーネル戻り値       */
    MkErr_t err;    /* カーネルエラー内容   */
    size_t  size;   /* 受信メッセージサイズ */

    /* 初期化 */
    ret  = MK_RET_FAILURE;
    err  = MK_ERR_NONE;
    size  = 0;

    /* メッセージ受信 */
    ret = LibMkMsgReceive( taskId,                          /* 受信タスクID   */
                           pMsg,                            /* バッファ       */
                           sizeof ( MvfsMsgMountResp_t ),   /* バッファサイズ */
                           NULL,                            /* 送信元タスクID */
                           &size,                           /* 受信サイズ     */
                           0,                               /* タイムアウト値 */
                           &err                           );/* エラー内容     */

    /* 受信結果判定 */
    if ( ret != MK_RET_SUCCESS ) {
        /* 失敗 */

        /* エラー番号判定 */
        if ( err == MK_ERR_NO_EXIST ) {
            /* 存在しないタスクID */

            /* エラー番号設定 */
            MLIB_SET_IFNOT_NULL( pErrNo, LIBMVFS_ERR_NOT_FOUND );

        } else if ( err == MK_ERR_NO_MEMORY ) {
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
    if ( size != sizeof ( MvfsMsgMountResp_t ) ) {
        /* 不正 */

        /* エラー番号設定 */
        MLIB_SET_IFNOT_NULL( pErrNo, LIBMVFS_ERR_NOT_RESP );

        return LIBMVFS_RET_FAILURE;
    }

    /* メッセージチェック */
    if ( ( pMsg->header.funcId != MVFS_FUNCID_MOUNT ) &&
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
 * @brief       mount要求送信
 * @details     仮想ファイルサーバにmount要求メッセージを送信する。
 *
 * @param[in]   taskId  仮想ファイルサーバのタスクID
 * @param[in]   *pPath  ファイルパス(絶対パス)
 * @param[out]  *pErrNo エラー番号
 *                  - LIBMVFS_ERR_NONE      エラー無し
 *                  - LIBMVFS_ERR_NOT_FOUND 仮想ファイルサーバ不明
 *                  - LIBMVFS_ERR_NO_MEMORY メモリ不足
 *                  - LIBMVFS_ERR_OTHER     その他エラー
 *
 * @return      送信結果を返す。
 * @retval      LIBMVFS_RET_SUCCESS 正常終了
 * @retval      LIBMVFS_RET_FAILURE 異常終了
 */
/******************************************************************************/
static LibMvfsRet_t SendMountReq( MkTaskId_t taskId,
                                  const char *pPath,
                                  uint32_t   *pErrNo )
{
    MkRet_t           ret;      /* カーネル戻り値     */
    MkErr_t           err;      /* カーネルエラー内容 */
    MvfsMsgMountReq_t msg;      /* 要求メッセージ     */

    /* 初期化 */
    ret = MK_RET_FAILURE;
    err = MK_ERR_NONE;
    memset( &msg,  0, sizeof ( msg ) );

    /* メッセージ作成 */
    msg.header.funcId = MVFS_FUNCID_MOUNT;
    msg.header.type   = MVFS_TYPE_REQ;
    strncpy( msg.path, pPath, MVFS_PATH_MAXLEN );

    /* メッセージ送信 */
    ret = LibMkMsgSend( taskId,             /* 送信先タスクID   */
                        &msg,               /* 送信メッセージ   */
                        sizeof ( msg ),     /* 送信メッセージ長 */
                        &err            );  /* エラー番号       */

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
