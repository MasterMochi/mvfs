/******************************************************************************/
/*                                                                            */
/* src/libmvfs/Mount.c                                                        */
/*                                                                 2019/06/12 */
/* Copyright (C) 2019 Mochi.                                                  */
/*                                                                            */
/******************************************************************************/
/******************************************************************************/
/* インクルード                                                               */
/******************************************************************************/
/* 標準ヘッダ */
#include <stdbool.h>
#include <string.h>

/* ライブラリヘッダ */
#include <libMk.h>

/* 共通ヘッダ */
#include <mvfs.h>


/******************************************************************************/
/* 定義                                                                       */
/******************************************************************************/
/* リトライ */
#define RETRY_MAX  (    10 )    /**< リトライ上限回数             */
#define RETRY_WAIT ( 10000 )    /**< リトライウェイト(マイクロ秒) */


/******************************************************************************/
/* グローバル関数定義                                                         */
/******************************************************************************/
/******************************************************************************/
/**
 * @brief       mount
 * @details     マウントパスpPathをマウントする。
 *
 * @param[in]   *pPath マウントパス
 */
/******************************************************************************/
void LibMvfsMount( const char *pPath )
{
    int32_t            ret;     /* 関数戻り値           */
    int32_t            size;    /* 受信メッセージサイズ */
    uint32_t           errNo;   /* エラー番号           */
    uint32_t           retry;   /* リトライカウンタ     */
    MkTaskId_t         taskId;  /* タスクID             */
    MvfsMsgMountReq_t  reqMsg;  /* 要求メッセージ       */
    MvfsMsgMountResp_t respMsg; /* 応答メッセージ       */

    /* 初期化 */
    ret    = MK_TASKNAME_RET_FAILURE;
    errNo  = MK_TASKNAME_ERR_NONE;
    retry  = 0;
    taskId = MK_TASKID_NULL;
    memset( &reqMsg,  0, sizeof ( MvfsMsgMountReq_t  ) );
    memset( &respMsg, 0, sizeof ( MvfsMsgMountResp_t ) );

    /* リトライ上限まで繰り返す */
    do {
        /* タスクID取得 */
        ret = MkTaskNameGet( "VFS", &taskId, &errNo );

        /* 取得結果判定 */
        if ( ret == MK_TASKNAME_RET_SUCCESS ) {
            /* 成功 */

            break;
        }

        /* リトライ判定 */
        if ( ( errNo == MK_TASKNAME_ERR_NOREGISTERED ) &&
             ( retry <= RETRY_MAX                    )    ) {
            /* リトライ要 */

            /* ビジーウェイト */
            MkTimerSleep( RETRY_WAIT, NULL );
            /* リトライ回数更新 */
            retry++;
            /* リトライ */
            continue;

        } else {
            /* リトライ不要 */

            return;
        }

    } while ( true );

    /* メッセージ作成 */
    reqMsg.header.funcId = MVFS_FUNCID_MOUNT;
    reqMsg.header.type   = MVFS_TYPE_REQ;
    strncpy( reqMsg.path, pPath, MVFS_PATH_MAXLEN );

    /* メッセージ送信 */
    ret = MkMsgSend( taskId, &reqMsg, sizeof ( MvfsMsgMountReq_t ), &errNo );

    /* メッセージ受信 */
    size = MkMsgReceive( taskId,                            /* 受信タスクID   */
                         &respMsg,                          /* バッファ       */
                         sizeof ( MvfsMsgMountResp_t ),     /* バッファサイズ */
                         NULL,                              /* 送信元タスクID */
                         &errNo                         );  /* エラー番号     */

    /* メッセージ受信結果判定 */
    if ( size != sizeof ( MvfsMsgMountResp_t ) ) {
        /* 失敗 */

        return;
    }

    /* メッセージ応答結果判定 */
    if ( ( respMsg.header.funcId != MVFS_FUNCID_MOUNT   ) &&
         ( respMsg.header.type   != MVFS_TYPE_RESP      ) &&
         ( respMsg.result        != MVFS_RESULT_SUCCESS )    ) {
        /* 失敗 */

        return;
    }

    return;
}


/******************************************************************************/
