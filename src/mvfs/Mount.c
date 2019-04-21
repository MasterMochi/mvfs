/******************************************************************************/
/*                                                                            */
/* src/mvfs/Mount.c                                                           */
/*                                                                 2019/04/21 */
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
#include <libMk.h>
#include <libmlog.h>
#include <MLib/MLib.h>
#include <MLib/MLibList.h>

/* モジュール共通ヘッダ */
#include <mvfs.h>

/* モジュールヘッダ */
#include "Node.h"
#include "Mount.h"


/******************************************************************************/
/* 定義                                                                       */
/******************************************************************************/


/******************************************************************************/
/* ローカル関数宣言                                                           */
/******************************************************************************/
/* mount応答メッセージ送信 */
static void SendMountResp( MkTaskId_t dst,
                           uint32_t   result );


/******************************************************************************/
/* 静的グローバル変数定義                                                     */
/******************************************************************************/


/******************************************************************************/
/* グローバル関数定義                                                         */
/******************************************************************************/
/******************************************************************************/
/**
 * @brief       マウント実行
 * @details     機能呼出し元タスクIDtaskIdからのマウント要求を処理する。機能パ
 *              ラメータpParamからマウント先パスを取得しノードを生成した後、処
 *              理結果を機能呼出し元タスクIDへ返す。
 *
 * @param[in]   taskId  機能呼出し元タスクID
 * @param[in]   *pParam 機能パラメータ
 */
/******************************************************************************/
void MountDo( MkTaskId_t taskId,
              void       *pParam )
{
    int32_t           ret;          /* 関数戻り値     */
    MLibRet_t         retMLib;      /* MLib関数戻り値 */
    NodeInfo_t        *pRootNode;   /* ルートノード   */
    NodeInfo_t        *pNode;       /* ノード         */
    MvfsMsgMountReq_t *pMsg;        /* メッセージ     */

    /* 初期化 */
    pMsg = ( MvfsMsgMountReq_t * ) pParam;

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
        SendMountResp( taskId, MVFS_RESULT_FAILURE );

        return;
    }

    /* ノード存在判定 */
    /* TODO */

    /* ノード作成 */
    pNode = NodeCreate( pMsg->path, NODE_TYPE_MOUNT_FILE, taskId );

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
        SendMountResp( taskId, MVFS_RESULT_FAILURE );

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
        SendMountResp( taskId, MVFS_RESULT_FAILURE );

        /* ノード解放 */
        NodeDelete( pNode );

        return;
    }

    /* mount応答メッセージ(成功)送信 */
    SendMountResp( taskId, MVFS_RESULT_SUCCESS );

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
 * @brief       マウント応答メッセージ送信
 * @details     メッセージ送信先のタスクID dstに処理結果resultでmount応答メッ
 *              セージを送信する。
 *
 * @param[in]   dst    メッセージ送信先
 * @param[in]   result 処理結果
 *                  - MVFS_RESULT_SUCCESS 成功
 *                  - MVFS_RESULT_FAILURE 失敗
 */
/******************************************************************************/
static void SendMountResp( MkTaskId_t dst,
                           uint32_t   result )
{
    int32_t            ret;     /* 関数戻り値 */
    uint32_t           errNo;   /* エラー番号 */
    MvfsMsgMountResp_t msg;     /* メッセージ */

    /* 初期化 */
    ret   = MK_MSG_RET_FAILURE;
    errNo = MK_MSG_ERR_NONE;
    memset( &msg, 0, sizeof ( MvfsMsgMountResp_t ) );

    /* メッセージ設定 */
    msg.header.funcId = MVFS_FUNCID_MOUNT;
    msg.header.type   = MVFS_TYPE_RESP;
    msg.result        = result;

    LibMlogPut(
        "[mvfs][%s:%d] %s() dst=%u, retult=%u.",
        __FILE__,
        __LINE__,
        __func__,
        dst,
        result
    );

    /* メッセージ送信 */
    ret = MkMsgSend( dst, &msg, sizeof ( MvfsMsgMountResp_t ), &errNo );

    /* 送信結果判定 */
    if ( ret != MK_MSG_RET_SUCCESS ) {
        /* 失敗 */

        LibMlogPut(
            "[mvfs][%s:%d] %s() error. dst=%u, errNo=%u.",
            __FILE__,
            __LINE__,
            __func__,
            dst,
            errNo
        );
    }

    return;
}


/******************************************************************************/
