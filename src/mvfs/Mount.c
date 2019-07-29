/******************************************************************************/
/*                                                                            */
/* src/mvfs/Mount.c                                                           */
/*                                                                 2019/07/28 */
/* Copyright (C) 2019 Mochi.                                                  */
/*                                                                            */
/******************************************************************************/
/******************************************************************************/
/* インクルード                                                               */
/******************************************************************************/
/* 標準ヘッダ */
#include <stdint.h>
#include <string.h>

/* カーネルヘッダ */
#include <kernel/types.h>

/* ライブラリヘッダ */
#include <libmk.h>
#include <libmlog.h>
#include <MLib/MLib.h>
#include <MLib/MLibList.h>

/* モジュール共通ヘッダ */
#include <mvfs.h>

/* モジュールヘッダ */
#include "Mount.h"
#include "Node.h"


/******************************************************************************/
/* 定義                                                                       */
/******************************************************************************/


/******************************************************************************/
/* ローカル関数宣言                                                           */
/******************************************************************************/
/* mount応答メッセージ送信 */
static void SendMsgMountResp( MkTaskId_t dst,
                              uint32_t   result );


/******************************************************************************/
/* 静的グローバル変数定義                                                     */
/******************************************************************************/


/******************************************************************************/
/* グローバル関数定義                                                         */
/******************************************************************************/
/******************************************************************************/
/**
 * @brief       mount要求メッセージ受信
 * @details     メッセージ送信元タスクID(taskId)からのmount要求メッセージを処理
 *              する。メッセージバッファ(*pBuffer)からマウント先パスを取得し、
 *              ノードを生成した後、処理結果をメッセージ送信元タスクIDへ返す。
 *
 * @param[in]   taskId   メッセージ送信元タスクID
 * @param[in]   *pBuffer メッセージバッファ
 */
/******************************************************************************/
void MountRcvMsgMountReq( MkTaskId_t taskId,
                          void       *pBuffer )
{
    int32_t           ret;          /* 関数戻り値     */
    MLibRet_t         retMLib;      /* MLib関数戻り値 */
    NodeInfo_t        *pRootNode;   /* ルートノード   */
    NodeInfo_t        *pNode;       /* ノード         */
    MvfsMsgMountReq_t *pMsg;        /* メッセージ     */

    /* 初期化 */
    pMsg = ( MvfsMsgMountReq_t * ) pBuffer;

    LibMlogPut(
        "[mvfs][%s:%d] %s() start. taskId=%d, path=%s.",
        __FILE__,
        __LINE__,
        __func__,
        taskId,
        pMsg->path
    );

    /* 絶対パス判定 */
    if ( pMsg->path[ 0 ] != '/' ) {
        /* 絶対パスでない */

        /* エラー応答 */
        LibMlogPut(
            "[mvfs][%s:%d] %s() error. taskId=%u, path=%s.",
            __FILE__,
            __LINE__,
            __func__,
            taskId,
            pMsg->path
        );

        /* mount応答メッセージ(失敗)送信 */
        SendMsgMountResp( taskId, MVFS_RESULT_FAILURE );

        return;
    }

    /* ノード存在判定 */
    /* TODO */

    /* ノード作成 */
    pNode = NodeCreate( &( pMsg->path[ 1 ] ),
                        pMsg->path,
                        NODE_TYPE_MOUNT_FILE,
                        taskId                );

    /* 作成結果判定 */
    if ( pNode == NULL ) {
        /* 失敗 */

        /* エラー応答 */
        LibMlogPut(
            "[mvfs][%s:%d] %s() error. taskId=%u, path=%s.",
            __FILE__,
            __LINE__,
            __func__,
            taskId,
            pMsg->path
        );

        /* mount応答メッセージ(失敗)送信 */
        SendMsgMountResp( taskId, MVFS_RESULT_FAILURE );

        return;
    }

    /* ルートノード取得 */
    pRootNode = NodeGetRoot();

    /* ノードエントリ追加 */
    ret = NodeAddEntry( pRootNode, pNode );

    /* 追加結果判定 */
    if ( ret != MVFS_OK ) {
        /* 失敗 */

        /* エラー応答 */
        LibMlogPut(
            "[mvfs][%s:%d] %s() error. taskId=%u, path=%s.",
            __FILE__,
            __LINE__,
            __func__,
            taskId,
            pMsg->path
        );

        /* mount応答メッセージ(失敗)送信 */
        SendMsgMountResp( taskId, MVFS_RESULT_FAILURE );

        /* ノード解放 */
        NodeDelete( pNode );

        return;
    }

    /* mount応答メッセージ(成功)送信 */
    SendMsgMountResp( taskId, MVFS_RESULT_SUCCESS );

    LibMlogPut(
        "[mvfs][%s:%d] %s() end. taskId=%u, path=%s.",
        __FILE__,
        __LINE__,
        __func__,
        taskId,
        pMsg->path
    );

    return;
}


/******************************************************************************/
/* ローカル関数定義                                                           */
/******************************************************************************/
/******************************************************************************/
/**
 * @brief       mount応答メッセージ送信
 * @details     メッセージ送信先のタスクID dstに処理結果resultでmount応答メッ
 *              セージを送信する。
 *
 * @param[in]   dst    メッセージ送信先
 * @param[in]   result 処理結果
 *                  - MVFS_RESULT_SUCCESS 成功
 *                  - MVFS_RESULT_FAILURE 失敗
 */
/******************************************************************************/
static void SendMsgMountResp( MkTaskId_t dst,
                              uint32_t   result )
{
    MkRet_t            ret;     /* 関数戻り値 */
    MkErr_t            err;     /* エラー内容 */
    MvfsMsgMountResp_t msg;     /* メッセージ */

    /* 初期化 */
    ret = MK_RET_FAILURE;
    err = MK_ERR_NONE;
    memset( &msg, 0, sizeof ( MvfsMsgMountResp_t ) );

    /* メッセージ設定 */
    msg.header.funcId = MVFS_FUNCID_MOUNT;
    msg.header.type   = MVFS_TYPE_RESP;
    msg.result        = result;

    LibMlogPut(
        "[mvfs][%s:%d] %s() dst=%u, result=%u.",
        __FILE__,
        __LINE__,
        __func__,
        dst,
        result
    );

    /* メッセージ送信 */
    ret = LibMkMsgSend( dst, &msg, sizeof ( MvfsMsgMountResp_t ), &err );

    /* 送信結果判定 */
    if ( ret != MK_RET_SUCCESS ) {
        /* 失敗 */

        LibMlogPut(
            "[mvfs][%s:%d] %s() error. dst=%u, err=%u.",
            __FILE__,
            __LINE__,
            __func__,
            dst,
            err
        );
    }

    return;
}


/******************************************************************************/
