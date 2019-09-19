/******************************************************************************/
/*                                                                            */
/* src/mvfs/Fn/FnMount.c                                                      */
/*                                                                 2019/09/18 */
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

/* 外部モジュールヘッダ */
#include <mvfs.h>
#include <Debug.h>
#include <Msg.h>
#include <Node.h>


/******************************************************************************/
/* 外部モジュール向けグローバル関数定義                                       */
/******************************************************************************/
/******************************************************************************/
/**
 * @brief       Mount要求メッセージ受信
 * @details     メッセージ送信元タスクID(taskId)からのMount要求メッセージを処理
 *              する。メッセージバッファ(*pBuffer)からマウント先パスを取得し、
 *              ノードを生成した後、処理結果をメッセージ送信元タスクIDへ返す。
 *
 * @param[in]   taskId   メッセージ送信元タスクID
 * @param[in]   *pBuffer メッセージバッファ
 */
/******************************************************************************/
void FnMountRecvMountReq( MkTaskId_t taskId,
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

        /* Mount応答メッセージ(失敗)送信 */
        MsgSendMountResp( taskId, MVFS_RESULT_FAILURE );

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
        MsgSendMountResp( taskId, MVFS_RESULT_FAILURE );

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
        MsgSendMountResp( taskId, MVFS_RESULT_FAILURE );

        /* ノード解放 */
        NodeDelete( pNode );

        DEBUG_LOG_TRC( "%s(): end.", __func__ );
        return;
    }

    /* mount応答メッセージ(成功)送信 */
    MsgSendMountResp( taskId, MVFS_RESULT_SUCCESS );

    DEBUG_LOG_TRC( "%s(): end.", __func__ );

    return;
}


/******************************************************************************/
