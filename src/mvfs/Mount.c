/******************************************************************************/
/*                                                                            */
/* src/mvfs/Mount.c                                                           */
/*                                                                 2019/09/03 */
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
#include <MLib/MLib.h>
#include <MLib/MLibList.h>

/* モジュール共通ヘッダ */
#include <mvfs.h>

/* モジュールヘッダ */
#include "Debug.h"
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

    DEBUG_LOG_TRC(
        "%s(): start. taskId=0x%X, path=%s",
        __func__,
        taskId,
        pMsg->path
    );

    /* 絶対パス判定 */
    if ( pMsg->path[ 0 ] != '/' ) {
        /* 絶対パスでない */

        DEBUG_LOG_ERR( "invalid path: %s", pMsg->path );

        /* mount応答メッセージ(失敗)送信 */
        SendMsgMountResp( taskId, MVFS_RESULT_FAILURE );

        DEBUG_LOG_TRC( "%s(): end.", __func__ );
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

        DEBUG_LOG_ERR( "NodeCreate() ");

        /* mount応答メッセージ(失敗)送信 */
        SendMsgMountResp( taskId, MVFS_RESULT_FAILURE );

        DEBUG_LOG_TRC( "%s(): end.", __func__ );
        return;
    }

    /* ルートノード取得 */
    pRootNode = NodeGetRoot();

    /* ノードエントリ追加 */
    ret = NodeAddEntry( pRootNode, pNode );

    /* 追加結果判定 */
    if ( ret != MVFS_OK ) {
        /* 失敗 */

        DEBUG_LOG_ERR( "NodeAddEntry(): ret=%d", ret );

        /* mount応答メッセージ(失敗)送信 */
        SendMsgMountResp( taskId, MVFS_RESULT_FAILURE );

        /* ノード解放 */
        NodeDelete( pNode );

        DEBUG_LOG_TRC( "%s(): end.", __func__ );
        return;
    }

    /* mount応答メッセージ(成功)送信 */
    SendMsgMountResp( taskId, MVFS_RESULT_SUCCESS );

    DEBUG_LOG_TRC( "%s(): end.", __func__ );

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

    DEBUG_LOG_TRC( "%s(): start. dst=0x%X, result=%d", __func__, dst, result );

    /* メッセージ送信 */
    ret = LibMkMsgSend( dst, &msg, sizeof ( MvfsMsgMountResp_t ), &err );

    /* 送信結果判定 */
    if ( ret != MK_RET_SUCCESS ) {
        /* 失敗 */

        DEBUG_LOG_ERR( "LibMkMsgSend(): ret=%d, err=0x%X", ret, err );
    }

    DEBUG_LOG_FNC( "%s(): end.", __func__ );
    return;
}


/******************************************************************************/
