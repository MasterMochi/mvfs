/******************************************************************************/
/*                                                                            */
/* src/mvfs/Msg/Msg.c                                                         */
/*                                                                 2020/07/25 */
/* Copyright (C) 2019-2020 Mochi.                                             */
/*                                                                            */
/******************************************************************************/
/******************************************************************************/
/* インクルード                                                               */
/******************************************************************************/
/* 標準ヘッダ */
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* カーネルヘッダ */
#include <libmk.h>
#include <kernel/types.h>

/* 外部モジュールヘッダ */
#include <mvfs.h>
#include <Debug.h>
#include <Fd.h>
#include <Msg.h>
#include <Node.h>


/******************************************************************************/
/* 外部モジュール向けグローバル関数定義                                       */
/******************************************************************************/
/******************************************************************************/
/**
 * @brief       Close要求メッセージチェック
 * @details     Close要求メッセージの正当性をチェックする。
 *
 * @param[in]   taskId     メッセージ送信元タスクID
 * @param[in]   *pMsg      メッセージ
 * @param[in]   size       メッセージサイズ
 * @param[out]  **ppFdInfo FD情報
 *
 * @return      チェック結果を返す。
 * @retval      MVFS_RET_SUCCESS 正常
 * @retval      MVFS_RET_FAILURE 不正
 */
/******************************************************************************/
MvfsRet_t MsgCheckCloseReq( MkTaskId_t        taskId,
                            MvfsMsgCloseReq_t *pMsg,
                            size_t            size,
                            FdInfo_t          **ppFdInfo )
{
    /* サイズチェック */
    if ( size < sizeof ( MvfsMsgCloseReq_t ) ) {
        /* 不正 */

        DEBUG_LOG_ERR(
            "%s(): invalid size( %u < %u ).",
            __func__,
            size,
            sizeof ( MvfsMsgCloseReq_t )
        );
        return MVFS_RET_FAILURE;
    }

    /* タイプチェック */
    if ( pMsg->header.type != MVFS_TYPE_REQ ) {
        /* 不正 */

        DEBUG_LOG_ERR(
            "%s(): invalid type( %#X != %#X ).",
            __func__,
            pMsg->header.type,
            MVFS_TYPE_REQ
        );
        return MVFS_RET_FAILURE;
    }

    /* FD情報取得 */
    *ppFdInfo = FdGet( pMsg->globalFd );

    /* 取得結果判定 */
    if ( *ppFdInfo == NULL ) {
        /* 失敗 */

        DEBUG_LOG_ERR(
            "%s(): FdGet() error. ret=%#p, globalFd=%u",
            __func__,
            *ppFdInfo,
            pMsg->globalFd
        );
        return MVFS_RET_FAILURE;
    }

    /* プロセスID比較 */
    if ( MK_TASKID_TO_PID( taskId                ) !=
         MK_TASKID_TO_PID( ( *ppFdInfo )->taskId )    ) {
        /* 不一致 */

        DEBUG_LOG_ERR(
            "%s(): invalid pid( %#X != %#X ). globalFd=%u",
            __func__,
            MK_TASKID_TO_PID( taskId                ),
            MK_TASKID_TO_PID( ( *ppFdInfo )->taskId ),
            pMsg->globalFd
        );
        return MVFS_RET_FAILURE;
    }

    DEBUG_LOG_TRC(
        "%s(): OK. globalFd=%u(%s)",
        __func__,
        pMsg->globalFd,
        ( *ppFdInfo )->pNode->path
    );

    return MVFS_RET_SUCCESS;
}


/******************************************************************************/
/**
 * @brief       Mount要求メッセージチェック
 * @details     Mount要求メッセージの正当性をチェックする。
 *
 * @param[in]   *pMsg メッセージ
 * @param[in]   size  メッセージサイズ
 *
 * @return      チェック結果を返す。
 * @retval      MVFS_RET_SUCCESS 正常
 * @retval      MVFS_RET_FAILURE 不正
 */
/******************************************************************************/
MvfsRet_t MsgCheckMountReq( MvfsMsgMountReq_t *pMsg,
                            size_t            size   )
{
    /* サイズチェック */
    if ( size < sizeof ( MvfsMsgMountReq_t ) ) {
        /* 不正 */

        DEBUG_LOG_ERR(
            "%s(): invalid size( %u < %u ).",
            __func__,
            size,
            sizeof ( MvfsMsgMountReq_t )
        );
        return MVFS_RET_FAILURE;
    }

    /* タイプチェック */
    if ( pMsg->header.type != MVFS_TYPE_REQ ) {
        /* 不正 */

        DEBUG_LOG_ERR(
            "%s(): invalid type( %#X != %#X ).",
            __func__,
            pMsg->header.type,
            MVFS_TYPE_REQ
        );
        return MVFS_RET_FAILURE;
    }

    /* 絶対パス判定 */
    if ( pMsg->path[ 0 ] != '/' ) {
        /* 絶対パスではない */

        DEBUG_LOG_ERR(
            "%s(): invalid path( %s ).",
            __func__,
            pMsg->path
        );
        return MVFS_RET_FAILURE;
    }

    DEBUG_LOG_TRC( "%s(): OK. path=%s", __func__, pMsg->path );

    return MVFS_RET_SUCCESS;
}


/******************************************************************************/
/**
 * @brief       Open要求メッセージチェック
 * @details     Open要求メッセージの正当性をチェックする。
 *
 * @param[in]   *pMsg メッセージ
 * @param[in]   size  メッセージサイズ
 *
 * @return      チェック結果を返す。
 * @retval      MVFS_RET_SUCCESS 正常
 * @retval      MVFS_RET_FAILURE 不正
 */
/******************************************************************************/
MvfsRet_t MsgCheckOpenReq( MvfsMsgOpenReq_t *pMsg,
                           size_t           size   )
{
    /* サイズチェック */
    if ( size < sizeof ( MvfsMsgOpenReq_t ) ) {
        /* 不正 */

        DEBUG_LOG_ERR(
            "%s(): invalid size( %u < %u ).",
            __func__,
            size,
            sizeof ( MvfsMsgOpenReq_t )
        );
        return MVFS_RET_FAILURE;
    }

    /* タイプチェック */
    if ( pMsg->header.type != MVFS_TYPE_REQ ) {
        /* 不正 */

        DEBUG_LOG_ERR(
            "%s(): invalid type( %#X != %#X ).",
            __func__,
            pMsg->header.type,
            MVFS_TYPE_REQ
        );
        return MVFS_RET_FAILURE;
    }

    DEBUG_LOG_TRC(
        "%s(): OK. localFd=%u, path=%s",
        __func__,
        pMsg->localFd,
        pMsg->path
    );

    return MVFS_RET_SUCCESS;
}


/******************************************************************************/
/**
 * @brief       Read要求メッセージチェック
 * @details     Read要求メッセージの正当性をチェックする。
 *
 * @param[in]   taskId     メッセージ送信元タスクID
 * @param[in]   *pMsg      メッセージ
 * @param[in]   size       メッセージサイズ
 * @param[out]  **ppFdInfo FD情報
 *
 * @return      チェック結果を返す。
 * @retval      MVFS_RET_SUCCESS 正常
 * @retval      MVFS_RET_FAILURE 不正
 */
/******************************************************************************/
MvfsRet_t MsgCheckReadReq( MkTaskId_t       taskId,
                           MvfsMsgReadReq_t *pMsg,
                           size_t           size,
                           FdInfo_t         **ppFdInfo )
{
    /* サイズチェック */
    if ( size < sizeof ( MvfsMsgReadReq_t ) ) {
        /* 不正 */

        DEBUG_LOG_ERR(
            "%s(): invalid size( %u < %u ).",
            __func__,
            size,
            sizeof ( MvfsMsgReadReq_t )
        );
        return MVFS_RET_FAILURE;
    }

    /* タイプチェック */
    if ( pMsg->header.type != MVFS_TYPE_REQ ) {
        /* 不正 */

        DEBUG_LOG_ERR(
            "%s(): invalid type( %#X != %#X ).",
            __func__,
            pMsg->header.type,
            MVFS_TYPE_REQ
        );
        return MVFS_RET_FAILURE;
    }

    /* FD情報取得 */
    *ppFdInfo = FdGet( pMsg->globalFd );

    /* 取得結果判定 */
    if ( *ppFdInfo == NULL ) {
        /* 失敗 */

        DEBUG_LOG_ERR(
            "%s(): FdGet() error. ret=%#p, globalFd=%u",
            __func__,
            *ppFdInfo,
            pMsg->globalFd
        );
        return MVFS_RET_FAILURE;
    }

    /* プロセスID比較 */
    if ( MK_TASKID_TO_PID( taskId ) !=
         MK_TASKID_TO_PID( ( *ppFdInfo )->taskId ) ) {
        /* 不一致 */

        DEBUG_LOG_ERR(
            "%s(): invalid pid( %#X != %#X ). globalFd=%u",
            __func__,
            MK_TASKID_TO_PID( taskId                ),
            MK_TASKID_TO_PID( ( *ppFdInfo )->taskId ),
            pMsg->globalFd
        );
        return MVFS_RET_FAILURE;
    }

    DEBUG_LOG_TRC(
        "%s(): OK. globalFd=%u(%s), readIdx=%#X, size=%u",
        __func__,
        pMsg->globalFd,
        ( *ppFdInfo )->pNode->path,
        ( uint32_t ) pMsg->readIdx,
        pMsg->size
    );

    return MVFS_RET_SUCCESS;
}


/******************************************************************************/
/**
 * @brief       Select要求メッセージチェック
 * @details     Select要求メッセージの正当性をチェックする。
 *
 * @param[in]   taskId     メッセージ送信元タスクID
 * @param[in]   *pMsg      メッセージ
 * @param[in]   size       メッセージサイズ
 *
 * @return      チェック結果を返す。
 * @retval      MVFS_RET_SUCCESS 正常
 * @retval      MVFS_RET_FAILURE 不正
 */
/******************************************************************************/
MvfsRet_t MsgCheckSelectReq( MkTaskId_t         taskId,
                             MvfsMsgSelectReq_t *pMsg,
                             size_t             size    )
{
    size_t   fdNum;     /* FDリストサイズ */
    uint32_t idx;       /* インデックス   */
    FdInfo_t *pFdInfo;  /* FD情報         */

    /* 初期化 */
    fdNum   = 0;
    idx     = 0;
    pFdInfo = NULL;

    /* サイズチェック */
    if ( size < sizeof ( MvfsMsgSelectReq_t ) ) {
        /* 不正 */

        DEBUG_LOG_ERR(
            "%s(): invalid size( %u < %u ).",
            __func__,
            size,
            sizeof ( MvfsMsgSelectReq_t )
        );
        return MVFS_RET_FAILURE;
    }

    /* タイプチェック */
    if ( pMsg->header.type != MVFS_TYPE_REQ ) {
        /* 不正 */

        DEBUG_LOG_ERR(
            "%s(): invalid type( %#X != %#X ).",
            __func__,
            pMsg->header.type,
            MVFS_TYPE_REQ
        );
        return MVFS_RET_FAILURE;
    }

    /* FDリストサイズ計算 */
    fdNum = pMsg->readFdNum + pMsg->writeFdNum;

    /* オーバーフローチェック */
    if ( ( fdNum < pMsg->readFdNum  ) ||
         ( fdNum < pMsg->writeFdNum )    ) {
        /* オーバーフロー */

        DEBUG_LOG_ERR(
            "%s(): invalid FdNum( %u = %u + %u ).",
            __func__,
            fdNum,
            pMsg->readFdNum,
            pMsg->writeFdNum
        );
        return MVFS_RET_FAILURE;
    }

    /* FDリストサイズチェック */
    if ( size < ( sizeof ( MvfsMsgSelectReq_t ) +
                  sizeof ( uint32_t ) * fdNum     ) ) {
        /* 不正 */

        DEBUG_LOG_ERR(
            "%s(): invalid size( %u < %u ).",
            __func__,
            size,
            sizeof ( MvfsMsgSelectReq_t ) + sizeof ( uint32_t ) * fdNum
        );
        return MVFS_RET_FAILURE;
    }

    /* 読込監視グローバルFD毎に繰り返す */
    for ( idx = 0; idx < pMsg->readFdNum; idx++ ) {
        /* FD情報取得 */
        pFdInfo = FdGet( pMsg->fd[ idx ] );

        /* 取得結果判定 */
        if ( pFdInfo == NULL ) {
            /* 失敗 */

            DEBUG_LOG_ERR(
                "%s(): invalid FD( %u ). idx=%u",
                __func__,
                pMsg->fd[ idx ],
                idx
            );
            return MVFS_RET_FAILURE;
        }

        /* Openチェック */
        if ( MK_TASKID_TO_PID( pFdInfo->taskId ) !=
             MK_TASKID_TO_PID( taskId          )    ) {
            /* 非Open */

            DEBUG_LOG_ERR(
                "%s(): invalid PID( %#X != %#X ). globalFd=%u",
                __func__,
                MK_TASKID_TO_PID( pFdInfo->taskId ),
                MK_TASKID_TO_PID( taskId          ),
                pMsg->fd[ idx ]
            );
            return MVFS_RET_FAILURE;
        }
    }

    /* 書込監視グローバルFD毎に繰り返す */
    for ( idx = 0; idx < pMsg->writeFdNum; idx++ ) {
        /* FD情報取得 */
        pFdInfo = FdGet( pMsg->fd[ pMsg->readFdNum + idx ] );

        /* 取得結果判定 */
        if ( pFdInfo == NULL ) {
            /* 失敗 */

            DEBUG_LOG_ERR(
                "%s(): invalid FD( %u ). idx=%u",
                __func__,
                pMsg->fd[ pMsg->readFdNum + idx ],
                pMsg->readFdNum + idx
            );
            return MVFS_RET_FAILURE;
        }

        /* Openチェック */
        if ( MK_TASKID_TO_PID( pFdInfo->taskId ) !=
             MK_TASKID_TO_PID( taskId          )    ) {
            /* 非Open */

            DEBUG_LOG_ERR(
                "%s(): invalid PID( %#X != %#X ). globalFd=%u",
                __func__,
                MK_TASKID_TO_PID( pFdInfo->taskId ),
                MK_TASKID_TO_PID( taskId          ),
                pMsg->fd[ pMsg->readFdNum + idx ]
            );
            return MVFS_RET_FAILURE;
        }
    }

    DEBUG_LOG_TRC(
        "%s(): OK. readFdNum=%u, writeFdNum=%u",
        __func__,
        pMsg->readFdNum,
        pMsg->writeFdNum
    );

    return MVFS_RET_SUCCESS;
}


/******************************************************************************/
/**
 * @brief       VfsClose応答メッセージチェック
 * @details     VfsClose応答メッセージの正当性をチェックする。
 *
 * @param[in]   taskId     メッセージ送信元タスクID
 * @param[in]   *pMsg      メッセージ
 * @param[in]   size       メッセージサイズ
 * @param[out]  **ppFdInfo FD情報
 *
 * @return      チェック結果を返す。
 * @retval      MVFS_RET_SUCCESS 正常
 * @retval      MVFS_RET_FAILURE 不正
 */
/******************************************************************************/
MvfsRet_t MsgCheckVfsCloseResp( MkTaskId_t            taskId,
                                MvfsMsgVfsCloseResp_t *pMsg,
                                size_t                size,
                                FdInfo_t              **ppFdInfo )
{
    /* サイズチェック */
    if ( size < sizeof ( MvfsMsgVfsCloseResp_t ) ) {
        /* 不正 */

        DEBUG_LOG_ERR(
            "%s(): invalid size( %u < %u ).",
            __func__,
            size,
            sizeof ( MvfsMsgVfsCloseResp_t )
        );
        return MVFS_RET_FAILURE;
    }

    /* タイプチェック */
    if ( pMsg->header.type != MVFS_TYPE_REQ ) {
        /* 不正 */

        DEBUG_LOG_ERR(
            "%s(): invalid type( %#X != %#X ).",
            __func__,
            pMsg->header.type,
            MVFS_TYPE_RESP
        );
        return MVFS_RET_FAILURE;
    }

    /* 処理結果チェック */
    if ( ( pMsg->result != MVFS_RESULT_SUCCESS ) &&
         ( pMsg->result != MVFS_RESULT_FAILURE )    ) {
        /* 不正 */

        DEBUG_LOG_ERR(
            "%s(): invalid result( %#X ).",
            __func__,
            pMsg->result
        );
        return MVFS_RET_FAILURE;
    }

    /* FD情報取得 */
    *ppFdInfo = FdGet( pMsg->globalFd );

    /* 取得結果判定 */
    if ( *ppFdInfo == NULL ) {
        /* 失敗 */

        DEBUG_LOG_ERR(
            "%s(): FdGet() error. ret=%#p, globalFd=%u",
            __func__,
            *ppFdInfo,
            pMsg->globalFd
        );
        return MVFS_RET_FAILURE;
    }

    /* プロセスID比較 */
    if ( MK_TASKID_TO_PID( taskId ) !=
         MK_TASKID_TO_PID( ( *ppFdInfo )->pNode->mountTaskId ) ) {
        /* 不一致 */

        DEBUG_LOG_ERR(
            "%s(): invalid pid( %#X != %#X ). globalFd=%u",
            __func__,
            MK_TASKID_TO_PID( taskId                            ),
            MK_TASKID_TO_PID( ( *ppFdInfo )->pNode->mountTaskId ),
            pMsg->globalFd
        );
        return MVFS_RET_FAILURE;
    }

    DEBUG_LOG_TRC(
        "%s(): OK. globalFd=%u(%s), result=%#X",
        __func__,
        pMsg->globalFd,
        ( *ppFdInfo )->pNode->path,
        pMsg->result
    );

    return MVFS_RET_SUCCESS;
}


/******************************************************************************/
/**
 * @brief       VfsOpen応答メッセージチェック
 * @details     VfsOpen応答メッセージの正当性をチェックする。
 *
 * @param[in]   taskId     メッセージ送信元タスクID
 * @param[in]   *pMsg      メッセージ
 * @param[in]   size       メッセージサイズ
 * @param[out]  **ppFdInfo FD情報
 *
 * @return      チェック結果を返す。
 * @retval      MVFS_RET_SUCCESS 正常
 * @retval      MVFS_RET_FAILURE 不正
 */
/******************************************************************************/
MvfsRet_t MsgCheckVfsOpenResp( MkTaskId_t           taskId,
                               MvfsMsgVfsOpenResp_t *pMsg,
                               size_t               size,
                               FdInfo_t             **ppFdInfo )
{
    /* サイズチェック */
    if ( size < sizeof ( MvfsMsgVfsOpenResp_t ) ) {
        /* 不正 */

        DEBUG_LOG_ERR(
            "%s(): invalid size( %u < %u ).",
            __func__,
            size,
            sizeof ( MvfsMsgVfsOpenResp_t )
        );
        return MVFS_RET_FAILURE;
    }

    /* タイプチェック */
    if ( pMsg->header.type != MVFS_TYPE_RESP ) {
        /* 不正 */

        DEBUG_LOG_ERR(
            "%s(): invalid type( %#X != %#X ).",
            __func__,
            pMsg->header.type,
            MVFS_TYPE_RESP
        );
        return MVFS_RET_FAILURE;
    }

    /* 処理結果チェック */
    if ( ( pMsg->result != MVFS_RESULT_SUCCESS ) &&
         ( pMsg->result != MVFS_RESULT_FAILURE )    ) {
        /* 不正 */

        DEBUG_LOG_ERR(
            "%s(): invalid result( %#X ).",
            __func__,
            pMsg->result
        );
        return MVFS_RET_FAILURE;
    }

    /* FD情報取得 */
    *ppFdInfo = FdGet( pMsg->globalFd );

    /* 取得結果判定 */
    if ( *ppFdInfo == NULL ) {
        /* 失敗 */

        DEBUG_LOG_ERR(
            "%s(): FdGet() error. ret=%#p, globalFd=%u",
            __func__,
            *ppFdInfo,
            pMsg->globalFd
        );
        return MVFS_RET_FAILURE;
    }

    /* プロセスID比較 */
    if ( MK_TASKID_TO_PID( taskId ) !=
         MK_TASKID_TO_PID( ( *ppFdInfo )->pNode->mountTaskId ) ) {
        /* 不一致 */

        DEBUG_LOG_ERR(
            "%s(): invalid pid( %#X != %#X ). globalFd=%u",
            __func__,
            MK_TASKID_TO_PID( taskId                            ),
            MK_TASKID_TO_PID( ( *ppFdInfo )->pNode->mountTaskId ),
            pMsg->globalFd
        );
        return MVFS_RET_FAILURE;
    }

    DEBUG_LOG_TRC(
        "%s(): OK. globalFd=%u(%s), result=%#X",
        __func__,
        pMsg->globalFd,
        ( *ppFdInfo )->pNode->path,
        pMsg->result
    );

    return MVFS_RET_SUCCESS;
}


/******************************************************************************/
/**
 * @brief       VfsRead応答メッセージチェック
 * @details     VfsRead応答メッセージの正当性をチェックする。
 *
 * @param[in]   taskId     メッセージ送信元タスクID
 * @param[in]   *pMsg      メッセージ
 * @param[in]   size       メッセージサイズ
 * @param[out]  **ppFdInfo FD情報
 *
 * @return      チェック結果を返す。
 * @retval      MVFS_RET_SUCCESS 正常
 * @retval      MVFS_RET_FAILURE 不正
 */
/******************************************************************************/
MvfsRet_t MsgCheckVfsReadResp( MkTaskId_t           taskId,
                               MvfsMsgVfsReadResp_t *pMsg,
                               size_t               size,
                               FdInfo_t             **ppFdInfo )
{
    /* サイズチェック */
    if ( size < sizeof ( MvfsMsgVfsReadResp_t ) ) {
        /* 不正 */

        DEBUG_LOG_ERR(
            "%s(): invalid size( %u < %u ).",
            __func__,
            size,
            sizeof ( MvfsMsgVfsReadResp_t )
        );
        return MVFS_RET_FAILURE;
    }

    /* バッファサイズチェック */
    if ( size < sizeof ( MvfsMsgVfsReadResp_t ) + pMsg->size ) {
        /* 不正 */

        DEBUG_LOG_ERR(
            "%s(): invalid size( %u < %u ).",
            __func__,
            size,
            sizeof ( MvfsMsgVfsReadResp_t ) + pMsg->size
        );
        return MVFS_RET_FAILURE;
    }

    /* タイプチェック */
    if ( pMsg->header.type != MVFS_TYPE_RESP ) {
        /* 不正 */

        DEBUG_LOG_ERR(
            "%s(): invalid type( %#X != %#X ).",
            __func__,
            pMsg->header.type,
            MVFS_TYPE_RESP
        );
        return MVFS_RET_FAILURE;
    }

    /* 処理結果チェック */
    if ( ( pMsg->result != MVFS_RESULT_SUCCESS ) &&
         ( pMsg->result != MVFS_RESULT_FAILURE )    ) {
        /* 不正 */

        DEBUG_LOG_ERR(
            "%s(): invalid result( %#X ).",
            __func__,
            pMsg->result
        );
        return MVFS_RET_FAILURE;
    }

    /* FD情報取得 */
    *ppFdInfo = FdGet( pMsg->globalFd );

    /* 取得結果判定 */
    if ( *ppFdInfo == NULL ) {
        /* 失敗 */

        DEBUG_LOG_ERR(
            "%s(): FdGet() error. ret=%#p, globalFd=%u",
            __func__,
            *ppFdInfo,
            pMsg->globalFd
        );
        return MVFS_RET_FAILURE;
    }

    /* プロセスID比較 */
    if ( MK_TASKID_TO_PID( taskId ) !=
         MK_TASKID_TO_PID( ( *ppFdInfo )->pNode->mountTaskId ) ) {
        /* 不一致 */

        DEBUG_LOG_ERR(
            "%s(): invalid pid( %#X != %#X ). globalFd=%u",
            __func__,
            MK_TASKID_TO_PID( taskId                            ),
            MK_TASKID_TO_PID( ( *ppFdInfo )->pNode->mountTaskId ),
            pMsg->globalFd
        );
        return MVFS_RET_FAILURE;
    }

    DEBUG_LOG_TRC(
        "%s(): OK. globalFd=%u(%s), result=%#X, ready=%#X, size=%u",
        __func__,
        pMsg->globalFd,
        ( *ppFdInfo )->pNode->path,
        pMsg->result,
        pMsg->ready,
        pMsg->size
    );

    return MVFS_RET_SUCCESS;
}


/******************************************************************************/
/**
 * @brief       VfsReady通知メッセージチェック
 * @details     VfsReady通知メッセージの正当性をチェックする。
 *
 * @param[in]   taskId   メッセージ送信元タスクID
 * @param[in]   *pMsg    メッセージ
 * @param[in]   size     メッセージサイズ
 * @param[out]  **ppNode ノード情報
 *
 * @return      チェック結果を返す。
 * @retval      MVFS_RET_SUCCESS 正常
 * @retval      MVFS_RET_FAILURE 不正
 */
/******************************************************************************/
MvfsRet_t MsgCheckVfsReadyNtc( MkTaskId_t           taskId,
                               MvfsMsgVfsReadyNtc_t *pMsg,
                               size_t               size,
                               NodeInfo_t           **ppNode )
{
    /* サイズチェック */
    if ( size < sizeof ( MvfsMsgVfsReadyNtc_t ) ) {
        /* 不正 */

        DEBUG_LOG_ERR(
            "%s(): invalid size( %u < %u ).",
            __func__,
            size,
            sizeof ( MvfsMsgVfsReadyNtc_t )
        );
        return MVFS_RET_FAILURE;
    }

    /* タイプチェック */
    if ( pMsg->header.type != MVFS_TYPE_NTC ) {
        /* 不正 */

        DEBUG_LOG_ERR(
            "%s(): invalid type( %#X != %#X ).",
            __func__,
            pMsg->header.type,
            MVFS_TYPE_NTC
        );
        return MVFS_RET_FAILURE;
    }

    /* レディ状態チェック */
    if ( ( pMsg->ready & ~( MVFS_READY_READ | MVFS_READY_WRITE ) ) != 0 ) {
        /* 不正 */

        DEBUG_LOG_ERR(
            "%s(): invalid ready( %#X ).",
            __func__,
            pMsg->ready
        );
        return MVFS_RET_FAILURE;
    }

    /* ノード取得 */
    *ppNode = NodeGet( pMsg->path );

    /* 取得結果判定 */
    if ( *ppNode == NULL ) {
        /* 失敗 */

        DEBUG_LOG_ERR(
            "%s(): invalid path( %s ).",
            __func__,
            pMsg->path
        );
        return MVFS_RET_FAILURE;
    }

    /* ファイルタイプ判定 */
    if ( ( *ppNode )->type != NODE_TYPE_MOUNT_FILE ) {
        /* 非マウントファイル */

        DEBUG_LOG_ERR(
            "%s(): invalid type( %#X ). path=%s",
            __func__,
            ( *ppNode )->type,
            pMsg->path
        );
        return MVFS_RET_FAILURE;
    }

    /* プロセス判定 */
    if ( MK_TASKID_TO_PID( taskId                   ) !=
         MK_TASKID_TO_PID( ( *ppNode )->mountTaskId )    ) {
        /* 不一致 */

        DEBUG_LOG_ERR(
            "%s(): invalid pid( %#X != %#X ). path=%s",
            __func__,
            MK_TASKID_TO_PID( taskId                   ),
            MK_TASKID_TO_PID( ( *ppNode )->mountTaskId ),
            pMsg->path
        );
        return MVFS_RET_FAILURE;
    }

    DEBUG_LOG_TRC(
        "%s(): OK. path=%s, ready=%#X",
        __func__,
        pMsg->path,
        pMsg->ready
    );

    return MVFS_RET_SUCCESS;
}


/******************************************************************************/
/**
 * @brief       VfsWrite応答メッセージチェック
 * @details     VfsWrite応答メッセージの正当性をチェックする。
 *
 * @param[in]   taskId     メッセージ送信元タスクID
 * @param[in]   *pMsg      メッセージ
 * @param[in]   size       メッセージサイズ
 * @param[out]  **ppFdInfo FD情報
 *
 * @return      チェック結果を返す。
 * @retval      MVFS_RET_SUCCESS 正常
 * @retval      MVFS_RET_FAILURE 不正
 */
/******************************************************************************/
MvfsRet_t MsgCheckVfsWriteResp( MkTaskId_t            taskId,
                                MvfsMsgVfsWriteResp_t *pMsg,
                                size_t                size,
                                FdInfo_t              **ppFdInfo )
{
    /* サイズチェック */
    if ( size < sizeof ( MvfsMsgVfsWriteResp_t ) ) {
        /* 不正 */

        DEBUG_LOG_ERR(
            "%s(): invalid size( %u < %u ).",
            __func__,
            size,
            sizeof ( MvfsMsgVfsWriteResp_t )
        );
        return MVFS_RET_FAILURE;
    }

    /* タイプチェック */
    if ( pMsg->header.type != MVFS_TYPE_RESP ) {
        /* 不正 */

        DEBUG_LOG_ERR(
            "%s(): invalid type( %#X != %#X ).",
            __func__,
            pMsg->header.type,
            MVFS_TYPE_RESP
        );
        return MVFS_RET_FAILURE;
    }

    /* 処理結果チェック */
    if ( ( pMsg->result != MVFS_RESULT_SUCCESS ) &&
         ( pMsg->result != MVFS_RESULT_FAILURE )    ) {
        /* 不正 */

        DEBUG_LOG_ERR(
            "%s(): invalid result( %#X ).",
            __func__,
            pMsg->result
        );
        return MVFS_RET_FAILURE;
    }

    /* FD情報取得 */
    *ppFdInfo = FdGet( pMsg->globalFd );

    /* 取得結果判定 */
    if ( *ppFdInfo == NULL ) {
        /* 失敗 */

        DEBUG_LOG_ERR(
            "%s(): FdGet() error. ret=%#p, globalFd=%u",
            __func__,
            *ppFdInfo,
            pMsg->globalFd
        );
        return MVFS_RET_FAILURE;
    }

    /* プロセスID比較 */
    if ( MK_TASKID_TO_PID( taskId ) !=
         MK_TASKID_TO_PID( ( *ppFdInfo )->pNode->mountTaskId ) ) {
        /* 不一致 */

        DEBUG_LOG_ERR(
            "%s(): invalid pid( %#X != %#X ). globalFd=%u",
            __func__,
            MK_TASKID_TO_PID( taskId                            ),
            MK_TASKID_TO_PID( ( *ppFdInfo )->pNode->mountTaskId ),
            pMsg->globalFd
        );
        return MVFS_RET_FAILURE;
    }

    DEBUG_LOG_TRC(
        "%s(): OK. globalFd=%u(%s), result=%#X, ready=%#X, size=%u",
        __func__,
        pMsg->globalFd,
        ( *ppFdInfo )->pNode->path,
        pMsg->result,
        pMsg->ready,
        pMsg->size
    );

    return MVFS_RET_SUCCESS;
}


/******************************************************************************/
/**
 * @brief       Write要求メッセージチェック
 * @details     Write要求メッセージの正当性をチェックする。
 *
 * @param[in]   taskId     メッセージ送信元タスクID
 * @param[in]   *pMsg      メッセージ
 * @param[in]   size       メッセージサイズ
 * @param[out]  **ppFdInfo FD情報
 *
 * @return      チェック結果を返す。
 * @retval      MVFS_RET_SUCCESS 正常
 * @retval      MVFS_RET_FAILURE 不正
 */
/******************************************************************************/
MvfsRet_t MsgCheckWriteReq( MkTaskId_t        taskId,
                            MvfsMsgWriteReq_t *pMsg,
                            size_t            size,
                            FdInfo_t          **ppFdInfo )
{
    /* サイズチェック */
    if ( size < sizeof ( MvfsMsgWriteReq_t ) ) {
        /* 不正 */

        DEBUG_LOG_ERR(
            "%s(): invalid size( %u < %u ).",
            __func__,
            size,
            sizeof ( MvfsMsgWriteReq_t )
        );
        return MVFS_RET_FAILURE;
    }

    /* バッファサイズチェック */
    if ( size < sizeof ( MvfsMsgWriteResp_t ) + pMsg->size ) {
        /* 不正 */

        DEBUG_LOG_ERR(
            "%s(): invalid size( %u < %u ).",
            __func__,
            size,
            sizeof ( MvfsMsgWriteResp_t ) + pMsg->size
        );
        return MVFS_RET_FAILURE;
    }

    /* タイプチェック */
    if ( pMsg->header.type != MVFS_TYPE_REQ ) {
        /* 不正 */

        DEBUG_LOG_ERR(
            "%s(): invalid type( %#X != %#X ).",
            __func__,
            pMsg->header.type,
            MVFS_TYPE_REQ
        );
        return MVFS_RET_FAILURE;
    }

    /* FD情報取得 */
    *ppFdInfo = FdGet( pMsg->globalFd );

    /* 取得結果判定 */
    if ( *ppFdInfo == NULL ) {
        /* 失敗 */

        DEBUG_LOG_ERR(
            "%s(): FdGet() error. ret=%#p, globalFd=%u",
            __func__,
            *ppFdInfo,
            pMsg->globalFd
        );
        return MVFS_RET_FAILURE;
    }

    /* プロセスID比較 */
    if ( MK_TASKID_TO_PID( taskId ) !=
         MK_TASKID_TO_PID( ( *ppFdInfo )->taskId ) ) {
        /* 不一致 */

        DEBUG_LOG_ERR(
            "%s(): invalid pid( %#X != %#X ). globalFd=%u",
            __func__,
            MK_TASKID_TO_PID( taskId                ),
            MK_TASKID_TO_PID( ( *ppFdInfo )->taskId ),
            pMsg->globalFd
        );
        return MVFS_RET_FAILURE;
    }

    DEBUG_LOG_TRC(
        "%s(): OK. globalFd=%u(%s), writeIdx=%#X, size=%u",
        __func__,
        pMsg->globalFd,
        ( *ppFdInfo )->pNode->path,
        ( uint32_t ) pMsg->writeIdx,
        pMsg->size
    );

    return MVFS_RET_SUCCESS;
}


/******************************************************************************/
/**
 * @brief       Close応答メッセージ送信
 * @details     メッセージ送信先タスクID(dst)にClose応答メッセージを送信する。
 *
 * @param[in]   dst      メッセージ送信先タスクID
 * @param[in]   result   処理結果
 *                  - MVFS_RESULT_SUCCESS 成功
 *                  - MVFS_RESULT_FAILURE 失敗
 */
/******************************************************************************/
void MsgSendCloseResp( MkTaskId_t dst,
                       uint32_t   result )
{
    MkRet_t            ret;     /* 関数戻り値 */
    MkErr_t            err;     /* エラー内容 */
    MvfsMsgCloseResp_t msg;     /* メッセージ */

    /* 初期化 */
    ret = MK_RET_FAILURE;
    err = MK_ERR_NONE;
    memset( &msg, 0, sizeof ( msg ) );

    DEBUG_LOG_TRC(
        "%s(): dst=%#X, result=%#X",
        __func__,
        dst,
        result
    );

    /* メッセージ設定 */
    msg.header.funcId = MVFS_FUNCID_CLOSE;
    msg.header.type   = MVFS_TYPE_RESP;
    msg.result        = result;

    /* メッセージ送信 */
    ret = LibMkMsgSendNB( dst, &msg, sizeof ( msg ), &err );

    /* 送信結果判定 */
    if ( ret != MK_RET_SUCCESS ) {
        /* 失敗 */

        DEBUG_LOG_ERR(
            "%s(): LibMkMsgSendNB(): ret=%d, err=%#X",
            __func__,
            ret,
            err
        );
    }

    return;
}


/******************************************************************************/
/**
 * @brief       Mount応答メッセージ送信
 * @details     メッセージ送信先のタスクID(dst)に処理結果resultでMount応答メッ
 *              セージを送信する。
 *
 * @param[in]   dst    メッセージ送信先
 * @param[in]   result 処理結果
 *                  - MVFS_RESULT_SUCCESS 成功
 *                  - MVFS_RESULT_FAILURE 失敗
 */
/******************************************************************************/
void MsgSendMountResp( MkTaskId_t dst,
                       uint32_t   result )
{
    MkRet_t            ret;     /* 関数戻り値 */
    MkErr_t            err;     /* エラー内容 */
    MvfsMsgMountResp_t msg;     /* メッセージ */

    /* 初期化 */
    ret = MK_RET_FAILURE;
    err = MK_ERR_NONE;
    memset( &msg, 0, sizeof ( MvfsMsgMountResp_t ) );

    DEBUG_LOG_TRC(
        "%s(): start. dst=%#X, result=%#X",
        __func__,
        dst,
        result
    );

    /* メッセージ設定 */
    msg.header.funcId = MVFS_FUNCID_MOUNT;
    msg.header.type   = MVFS_TYPE_RESP;
    msg.result        = result;

    /* メッセージ送信 */
    ret = LibMkMsgSendNB( dst, &msg, sizeof ( MvfsMsgMountResp_t ), &err );

    /* 送信結果判定 */
    if ( ret != MK_RET_SUCCESS ) {
        /* 失敗 */

        DEBUG_LOG_ERR(
            "%s(): LibMkMsgSendNB(): ret=%d, err=%#X",
            __func__,
            ret,
            err
        );
    }

    return;
}


/******************************************************************************/
/**
 * @brief       Open応答メッセージ送信
 * @details     メッセージ送信先タスクID(dst)にOpen応答メッセージを送信する。
 *
 * @param[in]   dst      メッセージ送信先タスクID
 * @param[in]   result   処理結果
 *                  - MVFS_RESULT_SUCCESS 成功
 *                  - MVFS_RESULT_FAILURE 失敗
 * @param[in]   globalFd グローバルFD
 */
/******************************************************************************/
void MsgSendOpenResp( MkTaskId_t dst,
                      uint32_t   result,
                      uint32_t   globalFd )
{
    MkRet_t           ret;      /* 関数戻り値 */
    MkErr_t           err;      /* エラー内容 */
    MvfsMsgOpenResp_t msg;      /* メッセージ */

    /* 初期化 */
    ret = MK_RET_FAILURE;
    err = MK_ERR_NONE;
    memset( &msg, 0, sizeof ( MvfsMsgOpenResp_t ) );

    DEBUG_LOG_TRC(
        "%s(): dst=%#X, result=%#X, globalFd=%u",
        __func__,
        dst,
        result,
        globalFd
    );

    /* メッセージ設定 */
    msg.header.funcId = MVFS_FUNCID_OPEN;
    msg.header.type   = MVFS_TYPE_RESP;
    msg.result        = result;
    msg.globalFd      = globalFd;

    /* メッセージ送信 */
    ret = LibMkMsgSendNB( dst, &msg, sizeof ( MvfsMsgOpenResp_t ), &err );

    /* 送信結果判定 */
    if ( ret != MK_RET_SUCCESS ) {
        /* 失敗 */

        DEBUG_LOG_ERR(
            "%s(): LibMkMsgSendNB(): ret=%d, err=%#X",
            __func__,
            ret,
            err
        );
    }

    return;
}


/******************************************************************************/
/**
 * @brief       Read応答メッセージ送信
 * @details     メッセージ送信先タスクID(dst)にRead応答メッセージを送信する。
 *
 * @param[in]   dst      メッセージ送信先タスクID
 * @param[in]   result   処理結果
 *                  - MVFS_RESULT_SUCCESS 成功
 *                  - MVFS_RESULT_FAILURE 失敗
 * @param[in]   *pBuffer 読込みバッファ
 * @param[in]   size     読込み実施サイズ
 */
/******************************************************************************/
void MsgSendReadResp( MkTaskId_t dst,
                      uint32_t   result,
                      void       *pBuffer,
                      size_t     size      )
{
    MkRet_t           ret;      /* 関数戻り値 */
    MkErr_t           err;      /* エラー内容 */
    MvfsMsgReadResp_t *pMsg;    /* メッセージ */

    /* 初期化 */
    ret = MK_RET_FAILURE;
    err = MK_ERR_NONE;

    DEBUG_LOG_TRC(
        "%s(): dst=%#X, result=%#X, size=%u",
        __func__,
        dst,
        result,
        size
    );

    /* バッファ確保 */
    pMsg = malloc( sizeof ( MvfsMsgReadResp_t ) + size );

    /* 確保結果判定 */
    if ( pMsg == NULL ) {
        /* 失敗 */

        DEBUG_LOG_ERR(
            "%s(): malloc(): size=%u",
            __func__,
            sizeof ( MvfsMsgReadResp_t ) + size
        );
        return;
    }

    /* バッファ初期化 */
    memset( pMsg, 0, sizeof ( MvfsMsgReadResp_t ) + size );

    /* メッセージ設定 */
    pMsg->header.funcId = MVFS_FUNCID_READ;
    pMsg->header.type   = MVFS_TYPE_RESP;
    pMsg->result        = result;
    pMsg->size          = size;

    /* 読込みバッファ有無チェック */
    if ( pBuffer != NULL ) {
        /* 有り */

        /* 読込みバッファコピー */
        memcpy( pMsg->pBuffer, pBuffer, size );
    }

    /* メッセージ送信 */
    ret = LibMkMsgSendNB( dst,
                          pMsg,
                          sizeof ( MvfsMsgReadResp_t ) + size,
                          &err                                 );

    /* 送信結果判定 */
    if ( ret != MK_RET_SUCCESS ) {
        /* 失敗 */

        DEBUG_LOG_ERR(
            "%s(): LibMkMsgSendNB(): ret=%d, err=%#X",
            __func__,
            ret,
            err
        );
    }

    /* バッファ解放 */
    free( pMsg );

    return;
}


/******************************************************************************/
/**
 * @brief       Select応答メッセージ送信
 * @details     メッセージ送信先タスクID(dst)にSelect応答メッセージを送信する。
 *
 * @param[in]   dst           メッセージ送信先タスクID
 * @param[in]   result        処理結果
 *                  - MVFS_RESULT_SUCCESS 成功
 *                  - MVFS_RESULT_FAILURE 失敗
 * @param[in]   *pReadFdList  読込レディグローバルFDリスト
 * @param[in]   readFdNum     読込レディグローバルFD数
 * @param[in]   *pWriteFdList 書込レディグローバルFDリスト
 * @param[in]   writeFdNum    書込レディグローバルFD数
 */
/******************************************************************************/
void MsgSendSelectResp( MkTaskId_t dst,
                        uint32_t   result,
                        uint32_t   *pReadFdList,
                        size_t     readFdNum,
                        uint32_t   *pWriteFdList,
                        size_t     writeFdNum     )
{
    size_t              size;   /* メッセージサイズ */
    MkRet_t             ret;    /* カーネル戻り値   */
    MkErr_t             err;    /* カーネルエラー   */
    MvfsMsgSelectResp_t *pMsg;  /* メッセージ       */

    /* 初期化 */
    size = sizeof ( MvfsMsgSelectResp_t ) +
           sizeof ( uint32_t ) * ( readFdNum + writeFdNum );
    ret  = MK_RET_FAILURE;
    err  = MK_ERR_NONE;

    DEBUG_LOG_TRC(
        "%s(): dst=%#X, result=%#X, readFdNum=%u, writeFdNum=%u",
        __func__,
        dst,
        result,
        readFdNum,
        writeFdNum
    );

    /* バッファ確保 */
    pMsg = malloc( size );

    /* 確保結果判定 */
    if ( pMsg == NULL ) {
        /* 失敗 */

        DEBUG_LOG_ERR( "%s(): malloc(): size=%u", __func__, size );
        return;
    }

    /* バッファ初期化 */
    memset( pMsg, 0, size );

    /* メッセージ設定 */
    pMsg->header.funcId = MVFS_FUNCID_READ;
    pMsg->header.type   = MVFS_TYPE_RESP;
    pMsg->result        = result;
    pMsg->readFdNum     = readFdNum;
    pMsg->writeFdNum    = writeFdNum;

    /* 読込レディグローバルFDリスト設定 */
    memcpy( &( pMsg->fd[ 0 ] ),
            pReadFdList,
            sizeof ( uint32_t ) * readFdNum );

    /* 書込レディグローバルFDリスト設定 */
    memcpy( &( pMsg->fd[ readFdNum ] ),
            pWriteFdList,
            sizeof ( uint32_t ) * writeFdNum );

    /* メッセージ送信 */
    ret = LibMkMsgSendNB( dst, pMsg, size, &err );

    /* 送信結果判定 */
    if ( ret != MK_RET_SUCCESS ) {
        /* 失敗 */
        DEBUG_LOG_ERR(
            "%s(): LibMkMsgSendNB(): ret=%d, err=%#X",
            __func__,
            ret,
            err
        );
    }

    /* バッファ解放 */
    free( pMsg );

    return;
}


/******************************************************************************/
/**
 * @brief       VfsClose要求メッセージ送信
 * @details     メッセージ送信先タスクID(dst)にVfsClose要求メッセージを送信する。
 *
 * @param[in]   dst      メッセージ送信先タスクID
 * @param[in]   globalFd グローバルFD
 */
/******************************************************************************/
void MsgSendVfsCloseReq( MkTaskId_t dst,
                         uint32_t   globalFd )
{
    MkRet_t              ret;   /* 関数戻り値 */
    MkErr_t              err;   /* エラー内容 */
    MvfsMsgVfsCloseReq_t msg;   /* メッセージ */

    /* 初期化 */
    ret = MK_RET_FAILURE;
    err = MK_ERR_NONE;
    memset( &msg, 0, sizeof ( MvfsMsgVfsCloseReq_t ) );

    DEBUG_LOG_TRC(
        "%s(): dst=%#X, globalFd=%u",
        __func__,
        dst,
        globalFd
    );

    /* メッセージ設定 */
    msg.header.funcId = MVFS_FUNCID_VFSCLOSE;
    msg.header.type   = MVFS_TYPE_REQ;
    msg.globalFd      = globalFd;

    /* メッセージ送信 */
    ret = LibMkMsgSendNB( dst, &msg, sizeof ( msg ), &err );

    /* 送信結果判定 */
    if ( ret != MK_RET_SUCCESS ) {
        /* 失敗 */

        DEBUG_LOG_ERR(
            "%s(): LibMkMsgSendNB(): ret=%d, err=%#X",
            __func__,
            ret,
            err
        );
    }

    return;
}


/******************************************************************************/
/**
 * @brief       VfsOpen要求メッセージ送信
 * @details     メッセージ送信先タスクID(dst)にVfsOpen要求メッセージを送信する。
 *
 * @param[in]   dst      メッセージ送信先タスクID
 * @param[in]   pid      プロセスID
 * @param[in]   globalFd グローバルFD
 * @param[in]   *pPath   ファイルパス（絶対パス）
 */
/******************************************************************************/
void MsgSendVfsOpenReq( MkTaskId_t dst,
                        MkPid_t    pid,
                        uint32_t   globalFd,
                        const char *pPath    )
{
    MkRet_t             ret;    /* 関数戻り値 */
    MkErr_t             err;    /* エラー内容 */
    MvfsMsgVfsOpenReq_t msg;    /* メッセージ */

    /* 初期化 */
    ret = MK_RET_FAILURE;
    err = MK_ERR_NONE;
    memset( &msg, 0, sizeof ( MvfsMsgVfsOpenReq_t ) );

    DEBUG_LOG_TRC(
        "%s(): dst=%#X, pid=%#X, globalFd=%u, pPath=%s",
        __func__,
        dst,
        pid,
        globalFd,
        pPath
    );

    /* メッセージ設定 */
    msg.header.funcId = MVFS_FUNCID_VFSOPEN;
    msg.header.type   = MVFS_TYPE_REQ;
    msg.pid           = pid;
    msg.globalFd      = globalFd;
    strncpy( msg.path, pPath, MVFS_PATH_MAXLEN );

    /* メッセージ送信 */
    ret = LibMkMsgSendNB( dst, &msg, sizeof ( MvfsMsgVfsOpenReq_t ), &err );

    /* 送信結果判定 */
    if ( ret != MK_RET_SUCCESS ) {
        /* 失敗 */

        DEBUG_LOG_ERR(
            "%s(): LibMkMsgSendNB(): ret=%d, err=%#X",
            __func__,
            ret,
            err
        );
    }

    return;
}


/******************************************************************************/
/**
 * @brief       VfsRead要求メッセージ送信
 * @details     メッセージ送信先タスクID(dst)にVfsRead要求メッセージを送信する。
 *
 * @param[in]   dst      メッセージ送信先タスクID
 * @param[in]   globalFd グローバルFD
 * @param[in]   readIdx  書込みインデックス
 * @param[in]   size     書込みサイズ
 */
/******************************************************************************/
void MsgSendVfsReadReq( MkTaskId_t dst,
                        uint32_t   globalFd,
                        uint64_t   readIdx,
                        size_t     size      )
{
    MkRet_t             ret;    /* 関数戻り値 */
    MkErr_t             err;    /* エラー内容 */
    MvfsMsgVfsReadReq_t msg;    /* メッセージ */

    /* 初期化 */
    ret = MK_RET_FAILURE;
    err = MK_ERR_NONE;
    memset( &msg, 0, sizeof ( MvfsMsgVfsReadReq_t ) );

    DEBUG_LOG_TRC(
        "%s(): dst=%#X, globalFd=%u, readIdx=%#X, size=%u",
        __func__,
        dst,
        globalFd,
        ( uint32_t ) readIdx,
        size
    );

    /* メッセージ設定 */
    msg.header.funcId = MVFS_FUNCID_VFSREAD;
    msg.header.type   = MVFS_TYPE_REQ;
    msg.globalFd      = globalFd;
    msg.readIdx       = readIdx;
    msg.size          = size;

    /* メッセージ送信 */
    ret = LibMkMsgSendNB( dst, &msg, sizeof ( MvfsMsgVfsReadReq_t ), &err );

    /* 送信結果判定 */
    if ( ret != MK_RET_SUCCESS ) {
        /* 失敗 */

        DEBUG_LOG_ERR(
            "%s(): LibMkMsgSendNB(): ret=%d, err=%#X",
            __func__,
            ret,
            err
        );
    }

    return;
}


/******************************************************************************/
/**
 * @brief       VfsWrite要求メッセージ送信
 * @details     メッセージ送信先タスクID(dst)にVfsWrite要求メッセージを送信する。
 *
 * @param[in]   dst      メッセージ送信先タスクID
 * @param[in]   globalFd グローバルFD
 * @param[in]   writeIdx 書込みインデックス
 * @param[in]   *pBuffer 書込みデータ
 * @param[in]   size     書込みサイズ
 */
/******************************************************************************/
void MsgSendVfsWriteReq( MkTaskId_t dst,
                         uint32_t   globalFd,
                         uint64_t   writeIdx,
                         const char *pBuffer,
                         size_t     size      )
{
    uint8_t              buffer[ MK_MSG_SIZE_MAX ]; /* バッファ   */
    MkRet_t              ret;                       /* 関数戻り値 */
    MkErr_t              err;                       /* エラー内容 */
    MvfsMsgVfsWriteReq_t *pMsg;                     /* メッセージ */

    /* 初期化 */
    ret  = MK_RET_FAILURE;
    err  = MK_ERR_NONE;
    pMsg = ( MvfsMsgVfsWriteReq_t * ) buffer;

    DEBUG_LOG_TRC(
        "%s(): dst=%#X, globalFd=%u, writeIdx=%#X, size=%u",
        __func__,
        dst,
        globalFd,
        ( uint32_t ) writeIdx,
        size
    );

    /* バッファ確保 */
    pMsg = malloc( sizeof ( MvfsMsgVfsWriteReq_t ) + size );

    /* 確保結果判定 */
    if ( pMsg == NULL ) {
        /* 失敗 */

        DEBUG_LOG_ERR(
            "%s(): malloc(): size=%u",
            __func__,
            sizeof ( MvfsMsgVfsWriteReq_t ) + size
        );
        return;
    }

    /* バッファ初期化 */
    memset( pMsg, 0, sizeof ( MvfsMsgVfsWriteReq_t ) + size );

    /* メッセージ設定 */
    pMsg->header.funcId = MVFS_FUNCID_VFSWRITE;
    pMsg->header.type   = MVFS_TYPE_REQ;
    pMsg->globalFd      = globalFd;
    pMsg->writeIdx      = writeIdx;
    pMsg->size          = size;
    memcpy( pMsg->pBuffer, pBuffer, size );

    /* メッセージ送信 */
    ret = LibMkMsgSendNB( dst,
                          pMsg,
                          sizeof ( MvfsMsgVfsWriteReq_t ) + size,
                          &err                                    );

    /* 送信結果判定 */
    if ( ret != MK_RET_SUCCESS ) {
        /* 失敗 */

        DEBUG_LOG_ERR(
            "%s(): LibMkMsgSendNB(): ret=%d, err=%#X",
            __func__,
            ret,
            err
        );
    }

    /* バッファ解放 */
    free( pMsg );

    return;
}


/******************************************************************************/
/**
 * @brief       Write応答メッセージ送信
 * @details     メッセージ送信先タスクID(dst)にWrite応答メッセージを送信する。
 *
 * @param[in]   dst    メッセージ送信先タスクID
 * @param[in]   result 処理結果
 *                  - MVFS_RESULT_SUCCESS 成功
 *                  - MVFS_RESULT_FAILURE 失敗
 * @param[in]   size   書込み実施サイズ
 */
/******************************************************************************/
void MsgSendWriteResp( MkTaskId_t dst,
                       uint32_t   result,
                       size_t     size    )
{
    MkRet_t            ret;     /* 関数戻り値 */
    MkErr_t            err;     /* エラー内容 */
    MvfsMsgWriteResp_t msg;     /* メッセージ */

    /* 初期化 */
    ret = MK_RET_FAILURE;
    err = MK_ERR_NONE;
    memset( &msg, 0, sizeof ( MvfsMsgWriteResp_t ) );

    DEBUG_LOG_TRC(
        "%s(): dst=%#X, result=%d, size=%u",
        __func__,
        dst,
        result,
        size
    );

    /* メッセージ設定 */
    msg.header.funcId = MVFS_FUNCID_WRITE;
    msg.header.type   = MVFS_TYPE_RESP;
    msg.result        = result;
    msg.size          = size;

    /* メッセージ送信 */
    ret = LibMkMsgSendNB( dst, &msg, sizeof ( MvfsMsgWriteResp_t ), &err );

    /* 送信結果判定 */
    if ( ret != MK_RET_SUCCESS ) {
        /* 失敗 */

        DEBUG_LOG_ERR(
            "%s(): LibMkMsgSendNB(): ret=%d, err=%#X",
            __func__,
            ret,
            err
        );
    }

    return;
}


/******************************************************************************/
