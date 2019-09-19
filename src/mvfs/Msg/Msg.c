/******************************************************************************/
/*                                                                            */
/* src/mvfs/Msg/Msg.c                                                         */
/*                                                                 2019/09/18 */
/* Copyright (C) 2019 Mochi.                                                  */
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
#include <Msg.h>


/******************************************************************************/
/* 外部モジュール向けグローバル関数定義                                       */
/******************************************************************************/
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

    DEBUG_LOG_TRC( "%s(): start. dst=0x%X, result=%d", __func__, dst, result );

    /* メッセージ設定 */
    msg.header.funcId = MVFS_FUNCID_CLOSE;
    msg.header.type   = MVFS_TYPE_RESP;
    msg.result        = result;

    /* メッセージ送信 */
    ret = LibMkMsgSend( dst, &msg, sizeof ( msg ), &err );

    /* 送信結果判定 */
    if ( ret != MK_RET_SUCCESS ) {
        /* 失敗 */

        DEBUG_LOG_ERR( "LibMkMsgSend(): ret=%d, err=0x%X", ret, err );
    }

    DEBUG_LOG_FNC( "%s(): end.", __func__ );
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

    /* メッセージ設定 */
    msg.header.funcId = MVFS_FUNCID_OPEN;
    msg.header.type   = MVFS_TYPE_RESP;
    msg.result        = result;
    msg.globalFd      = globalFd;

    DEBUG_LOG_TRC(
        "%s(): dst=0x%X, result=%d, globalFd=%u",
        __func__,
        dst,
        result,
        globalFd
    );

    /* メッセージ送信 */
    ret = LibMkMsgSend( dst, &msg, sizeof ( MvfsMsgOpenResp_t ), &err );

    /* 送信結果判定 */
    if ( ret != MK_RET_SUCCESS ) {
        /* 失敗 */

        DEBUG_LOG_ERR( "LibMkMsgSend(): ret=%d, err=0x%X", ret, err );
    }

    DEBUG_LOG_FNC( "%s(): end.", __func__ );
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
        "%s(): start. dst=0x%X, result=%d, size=%u",
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

        DEBUG_LOG_ERR( "malloc(): %u", sizeof ( MvfsMsgReadResp_t ) + size );
        DEBUG_LOG_FNC( "%s(): end.", __func__ );
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
    ret = LibMkMsgSend( dst, pMsg, sizeof ( MvfsMsgReadResp_t ) + size, &err );

    /* 送信結果判定 */
    if ( ret != MK_RET_SUCCESS ) {
        /* 失敗 */

        DEBUG_LOG_ERR( "LibMkMsgSend(): ret=%d, err=0x%X", ret, err );
    }

    /* バッファ解放 */
    free( pMsg );

    DEBUG_LOG_FNC( "%s(): end.", __func__ );
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
        "%s(): start. dst=0x%X, result=%d, readFdNum=%u, writeFdNum=%u",
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
        DEBUG_LOG_ERR( "malloc(): %u", size );
        DEBUG_LOG_FNC( "%s():end.", __func__ );
        return;
    }

    /* バッファ初期化 */
    memset( pMsg, 0, size );

    /* メッセージ設定 */
    pMsg->header.funcId = MVFS_FUNCID_READ;
    pMsg->header.type   = MVFS_TYPE_RESP;
    pMsg->result        = result;

    /* 読込レディグローバルFDリスト設定 */
    memcpy( &( pMsg->fd[ 0 ] ),
            pReadFdList,
            sizeof ( uint32_t ) * readFdNum );

    /* 書込レディグローバルFDリスト設定 */
    memcpy( &( pMsg->fd[ readFdNum ] ),
            pWriteFdList,
            sizeof ( uint32_t ) * writeFdNum );

    /* メッセージ送信 */
    ret = LibMkMsgSend( dst, pMsg, size, &err );

    /* 送信結果判定 */
    if ( ret != MK_RET_SUCCESS ) {
        /* 失敗 */
        DEBUG_LOG_ERR( "LibMkMsgSend(): ret=%d, err=0x%X", ret, err );
    }

    /* バッファ解放 */
    free( pMsg );

    DEBUG_LOG_FNC( "%s(): end.", __func__ );
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
        "%s(): start. dst=0x%X, globalFd=%u",
        __func__,
        dst,
        globalFd
    );

    /* メッセージ設定 */
    msg.header.funcId = MVFS_FUNCID_VFSCLOSE;
    msg.header.type   = MVFS_TYPE_REQ;
    msg.globalFd      = globalFd;

    /* メッセージ送信 */
    ret = LibMkMsgSend( dst, &msg, sizeof ( msg ), &err );

    /* 送信結果判定 */
    if ( ret != MK_RET_SUCCESS ) {
        /* 失敗 */

        DEBUG_LOG_ERR( "LibMkMsgSend(): ret=%d, err=0x%X", ret, err );
    }

    DEBUG_LOG_FNC( "%s(): end.", __func__ );
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

    /* メッセージ設定 */
    msg.header.funcId = MVFS_FUNCID_VFSOPEN;
    msg.header.type   = MVFS_TYPE_REQ;
    msg.pid           = pid;
    msg.globalFd      = globalFd;
    strncpy( msg.path, pPath, MVFS_PATH_MAXLEN );

    DEBUG_LOG_TRC(
        "%s(): start. dst=0x%X, pid=%u, globalFd=%u, pPath=%s",
        __func__,
        dst,
        pid,
        globalFd,
        pPath
    );

    /* メッセージ送信 */
    ret = LibMkMsgSend( dst, &msg, sizeof ( MvfsMsgVfsOpenReq_t ), &err );

    /* 送信結果判定 */
    if ( ret != MK_RET_SUCCESS ) {
        /* 失敗 */

        DEBUG_LOG_ERR( "LibMkMsgSend(): ret=%d, err=0x%X", ret, err );
    }

    DEBUG_LOG_FNC( "%s(): end.", __func__ );
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

    /* メッセージ設定 */
    msg.header.funcId = MVFS_FUNCID_VFSREAD;
    msg.header.type   = MVFS_TYPE_REQ;
    msg.globalFd      = globalFd;
    msg.readIdx       = readIdx;
    msg.size          = size;

    DEBUG_LOG_TRC(
        "%s(): start. dst=0x%X, globalFd=%u, readIdx=0x%X, size=%u",
        __func__,
        dst,
        globalFd,
        ( uint32_t ) readIdx,
        size
    );

    /* メッセージ送信 */
    ret = LibMkMsgSend( dst, &msg, sizeof ( MvfsMsgVfsReadReq_t ), &err );

    /* 送信結果判定 */
    if ( ret != MK_RET_SUCCESS ) {
        /* 失敗 */

        DEBUG_LOG_ERR( "LibMkMsgSend(): ret=%d, err=0x%X", ret, err );
    }

    DEBUG_LOG_FNC( "%s(): end.", __func__ );
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
        "%s(): start. dst=0x%X, globalFd=%u, writeIdx=0x%X, size=%u",
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

        DEBUG_LOG_ERR( "malloc(): %u", sizeof ( MvfsMsgVfsWriteReq_t ) + size );
        DEBUG_LOG_FNC( "%s(): end.", __func__ );
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
    ret = LibMkMsgSend( dst,
                        pMsg,
                        sizeof ( MvfsMsgVfsWriteReq_t ) + size,
                        &err                                    );

    /* 送信結果判定 */
    if ( ret != MK_RET_SUCCESS ) {
        /* 失敗 */

        DEBUG_LOG_ERR( "LibMkMsgSend(): ret=%d, err=0x%X", ret, err );
    }

    /* バッファ解放 */
    free( pMsg );

    DEBUG_LOG_FNC( "%s(): end.", __func__ );
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
        "%s(): start. dst=0x%X, result=%d, size=%u",
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
    ret = LibMkMsgSend( dst, &msg, sizeof ( MvfsMsgWriteResp_t ), &err );

    /* 送信結果判定 */
    if ( ret != MK_RET_SUCCESS ) {
        /* 失敗 */

        DEBUG_LOG_ERR( "LibMkMsgSend(): ret=%d, err=0x%X", ret, err );
    }

    DEBUG_LOG_FNC( "%s(): end.", __func__ );
    return;
}


/******************************************************************************/
