/******************************************************************************/
/*                                                                            */
/* src/mvfs/Read.c                                                            */
/*                                                                 2019/09/03 */
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
#include <kernel/types.h>

/* ライブラリヘッダ */
#include <libmk.h>
#include <MLib/MLibState.h>

/* モジュール共通ヘッダ */
#include <mvfs.h>

/* モジュールヘッダ */
#include "Debug.h"
#include "Fd.h"


/******************************************************************************/
/* 定義                                                                       */
/******************************************************************************/
/* 状態 */
#define STATE_INI               ( 1 )   /**< 初期状態    */
#define STATE_VFSREAD_RESP_WAIT ( 2 )   /**< vfsRead待ち */

/* イベント */
#define EVENT_READ_REQ     ( 1 )    /**< read要求イベント    */
#define EVENT_VFSREAD_RESP ( 2 )    /**< vfsRead応答イベント */

/** 状態遷移タスクパラメータ */
typedef struct {
    MkTaskId_t taskId;      /**< タスクID         */
    void       *pBuffer;    /**< イベントバッファ */
    FdInfo_t   *pFdInfo;    /**< FD情報           */
} StateTaskParam_t;


/******************************************************************************/
/* ローカル関数宣言                                                           */
/******************************************************************************/
/* read応答メッセージ送信 */
static void SendMsgReadResp( MkTaskId_t taskId,
                             uint32_t   result,
                             void       *pBuffer,
                             size_t     size      );
/* vfsRead要求メッセージ送信 */
static void SendMsgVfsReadReq( MkTaskId_t dst,
                               uint32_t   globalFd,
                               uint64_t   readIdx,
                               size_t     size      );

/* 状態遷移タスク */
static MLibState_t Task0101( void *pArg );
static MLibState_t Task0202( void *pArg );


/******************************************************************************/
/* 静的グローバル変数定義                                                     */
/******************************************************************************/
/** 状態遷移表 */
static const MLibStateTransition_t gStt[] = {
    { STATE_INI              , EVENT_READ_REQ    , Task0101, { STATE_VFSREAD_RESP_WAIT, 0 } },
    { STATE_INI              , EVENT_VFSREAD_RESP, NULL    , { STATE_INI              , 0 } },
    { STATE_VFSREAD_RESP_WAIT, EVENT_READ_REQ    , NULL    , { STATE_VFSREAD_RESP_WAIT, 0 } },
    { STATE_VFSREAD_RESP_WAIT, EVENT_VFSREAD_RESP, Task0202, { STATE_INI              , 0 } }  };

/** 状態遷移ハンドル */
static MLibStateHandle_t gStateHdl;

/* TODO */
static MkTaskId_t gReadTaskId;


/******************************************************************************/
/* グローバル関数定義                                                         */
/******************************************************************************/
/******************************************************************************/
/**
 * @brief       read機能初期化
 * @details     状態遷移を初期化する。
 */
/******************************************************************************/
void ReadInit( void )
{
    uint32_t  errNo;    /* エラー番号 */
    MLibRet_t ret;      /* MLIB戻り値 */

    DEBUG_LOG_FNC( "%s(): start.", __func__ );

    /* 初期化 */
    errNo = MLIB_STATE_ERR_NONE;
    ret   = MLIB_FAILURE;

    /* 状態遷移初期化 */
    ret = MLibStateInit( &gStateHdl,
                         gStt,
                         sizeof ( gStt ),
                         STATE_INI,
                         &errNo           );

    /* 初期化結果判定 */
    if ( ret != MLIB_SUCCESS ) {
        /* 失敗 */

        DEBUG_LOG_ERR( "MLibStateInit(): ret=%d, err=0x%X", ret, errNo );
    }

    DEBUG_LOG_FNC( "%s(): end.", __func__ );
    return;
}


/******************************************************************************/
/**
 * @brief       vfsRead応答メッセージ受信
 * @details     メッセージ送信元タスクID(taskId)からのvfsRead応答メッセージを処
 *              理する。
 *
 * @param[in]   taskId   メッセージ送信元タスクID
 * @param[in]   *pBuffer メッセージバッファ
 */
/******************************************************************************/
void ReadRcvMsgVfsReadResp( MkTaskId_t taskId,
                            void       *pBuffer )
{
    uint32_t             errNo;     /* エラー番号               */
    FdInfo_t             *pFdInfo;  /* FD情報                   */
    MLibRet_t            retMLib;   /* MLIB戻り値               */
    MLibState_t          prevState; /* 遷移前状態               */
    MLibState_t          nextState; /* 遷移後状態               */
    StateTaskParam_t     param;     /* 状態遷移タスクパラメータ */
    MvfsMsgVfsReadResp_t *pMsg;     /* メッセージ               */

    /* 初期化 */
    errNo     = MLIB_STATE_ERR_NONE;
    pFdInfo   = NULL;
    retMLib   = MLIB_FAILURE;
    prevState = MLIB_STATE_NULL;
    nextState = MLIB_STATE_NULL;
    pMsg      = ( MvfsMsgVfsReadResp_t * ) pBuffer;
    memset( &param, 0, sizeof ( param ) );

    DEBUG_LOG_TRC(
        "%s(): start. taskId=0x%X, result=%u, size=%u",
        __func__,
        taskId,
        pMsg->result,
        pMsg->size
    );

    /* FD情報取得 */
    pFdInfo = FdGet( pMsg->globalFd );

    /* 取得結果判定 */
    if ( pFdInfo == NULL ) {
        /* 失敗 */

        DEBUG_LOG_ERR( "FdGet(): %u", pMsg->globalFd );
        DEBUG_LOG_TRC( "%s(): end.", __func__ );
        return;
    }

    /* 状態遷移パラメータ設定 */
    param.taskId  = taskId;
    param.pBuffer = pBuffer;
    param.pFdInfo = pFdInfo;

    /* 状態遷移実行 */
    retMLib = MLibStateExec( &gStateHdl,
                             EVENT_VFSREAD_RESP,
                             &param,
                             &prevState,
                             &nextState,
                             &errNo              );

    /* 実行結果判定 */
    if ( retMLib != MLIB_SUCCESS ) {
        /* 失敗 */

        DEBUG_LOG_ERR( "MLibStateExec(): ret=%d, err=0x%X", retMLib, errNo );
        DEBUG_LOG_TRC( "%s(): end.", __func__ );
        return;
    }

    DEBUG_LOG_TRC( "state: %u->%u", prevState, nextState );
    DEBUG_LOG_TRC( "%s(): end.", __func__ );
    return;
}


/******************************************************************************/
/**
 * @brief       read要求メッセージ受信
 * @details     メッセージ送信元タスクID(taskId)からのread要求メッセージを処理
 *              する。
 *
 * @param[in]   taskId   メッセージ送信元タスクID
 * @param[in]   *pBuffer メッセージバッファ
 */
/******************************************************************************/
void ReadRcvMsgReadReq( MkTaskId_t taskId,
                        void       *pBuffer )
{
    uint32_t         errNo;     /* エラー番号               */
    FdInfo_t         *pFdInfo;  /* FD情報                   */
    MLibRet_t        retMLib;   /* MLIB戻り値               */
    NodeInfo_t       *pNode;    /* ノード                   */
    MLibState_t      prevState; /* 遷移前状態               */
    MLibState_t      nextState; /* 遷移後状態               */
    StateTaskParam_t param;     /* 状態遷移タスクパラメータ */
    MvfsMsgReadReq_t *pMsg;     /* メッセージ               */

    /* 初期化 */
    errNo     = MLIB_STATE_ERR_NONE;
    pFdInfo   = NULL;
    retMLib   = MLIB_FAILURE;
    pNode     = NULL;
    prevState = MLIB_STATE_NULL;
    nextState = MLIB_STATE_NULL;
    pMsg      = ( MvfsMsgReadReq_t * ) pBuffer;
    memset( &param, 0, sizeof ( param ) );

    DEBUG_LOG_TRC(
        "%s(): start. taskId=0x%X, globalFd=%u, readIdx=0x%X, size=%u",
        __func__,
        taskId,
        pMsg->globalFd,
        ( uint32_t ) pMsg->readIdx,
        pMsg->size
    );

    /* FD情報取得 */
    pFdInfo = FdGet( pMsg->globalFd );

    /* 取得結果判定 */
    if ( pFdInfo == NULL ) {
        /* 失敗 */

        DEBUG_LOG_ERR( "FdGet(): %u", pMsg->globalFd );

        /* read応答メッセージ送信 */
        SendMsgReadResp( taskId, MVFS_RESULT_FAILURE, NULL, 0 );

        DEBUG_LOG_TRC( "%s(): end.", __func__ );
        return;
    }

    /* 状態遷移パラメータ設定 */
    param.taskId  = taskId;
    param.pBuffer = pBuffer;
    param.pFdInfo = pFdInfo;

    /* 状態遷移実行 */
    retMLib = MLibStateExec( &gStateHdl,
                             EVENT_READ_REQ,
                             &param,
                             &prevState,
                             &nextState,
                             &errNo          );

    /* 実行結果判定 */
    if ( retMLib != MLIB_SUCCESS ) {
        /* 失敗 */

        DEBUG_LOG_ERR( "MLibStateExec(): ret=%d, err=0x%X", retMLib, errNo );

        /* read応答メッセージ送信 */
        SendMsgReadResp( taskId, MVFS_RESULT_FAILURE, NULL, 0 );

        DEBUG_LOG_TRC( "%s(): end.", __func__ );
        return;
    }

    DEBUG_LOG_TRC( "state: %u->%u", prevState, nextState );
    DEBUG_LOG_TRC( "%s(): end.", __func__ );
    return;
}


/******************************************************************************/
/* ローカル関数定義                                                           */
/******************************************************************************/
/******************************************************************************/
/**
 * @brief       read応答メッセージ送信
 * @details     メッセージ送信先タスクID(dst)にread応答メッセージを送信する。
 *
 * @param[in]   dst      メッセージ送信先タスクID
 * @param[in]   result   処理結果
 *                  - MVFS_RESULT_SUCCESS 成功
 *                  - MVFS_RESULT_FAILURE 失敗
 * @param[in]   *pBuffer 読込みバッファ
 * @param[in]   size     読込み実施サイズ
 */
/******************************************************************************/
static void SendMsgReadResp( MkTaskId_t dst,
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
 * @brief       vfsRead要求メッセージ送信
 * @details     メッセージ送信先タスクID(dst)にvfsRead要求メッセージを送信する。
 *
 * @param[in]   dst      メッセージ送信先タスクID
 * @param[in]   globalFd グローバルFD
 * @param[in]   readIdx  書込みインデックス
 * @param[in]   size     書込みサイズ
 */
/******************************************************************************/
static void SendMsgVfsReadReq( MkTaskId_t dst,
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
 * @brief       状態遷移タスク0101
 * @details     vfsRead要求メッセージを送信する。
 *
 * @param[in]   *pArg 状態遷移パラメータ
 *
 * @return      遷移先状態を返す。
 * @retval      STATE_INI               初期状態
 * @retval      STATE_VFSREAD_RESP_WAIT vfsRead応答待ち
 */
/******************************************************************************/
static MLibState_t Task0101( void *pArg )
{
    NodeInfo_t       *pNode;    /* ノード             */
    StateTaskParam_t *pParam;   /* 状態遷移パラメータ */
    MvfsMsgReadReq_t *pMsg;     /* read要求メッセージ */

    DEBUG_LOG_FNC( "%s(): start. pArg=%p", __func__, pArg );

    /* 初期化 */
    pParam = ( StateTaskParam_t  * ) pArg;
    pNode  = pParam->pFdInfo->pNode;
    pMsg   = ( MvfsMsgReadReq_t * ) pParam->pBuffer;

    /* vfsRead要求メッセージ送信 */
    SendMsgVfsReadReq( pNode->mountTaskId,
                       pMsg->globalFd,
                       pMsg->readIdx,
                       pMsg->size          );

    /* [TODO]read要求元タスクID保存 */
    gReadTaskId = pParam->taskId;

    DEBUG_LOG_FNC( "%s(): end. ret=%u", __func__, STATE_VFSREAD_RESP_WAIT );
    return STATE_VFSREAD_RESP_WAIT;
}


/******************************************************************************/
/**
 * @brief       状態遷移タスク0202
 * @details     read応答メッセージを送信する。
 *
 * @param[in]   *pArg 未使用
 *
 * @return      遷移先状態を返す。
 * @retval      STATE_INI 初期状態
 */
/******************************************************************************/
static MLibState_t Task0202( void *pArg )
{
    StateTaskParam_t     *pParam;  /* 状態遷移パラメータ    */
    MvfsMsgVfsReadResp_t *pMsg;    /* vfsRead応答メッセージ */

    DEBUG_LOG_FNC( "%s(): start. pArg=%p", __func__, pArg );

    /* 初期化 */
    pParam = ( StateTaskParam_t     * ) pArg;
    pMsg   = ( MvfsMsgVfsReadResp_t * ) pParam->pBuffer;

    /* read応答メッセージ送信 */
    SendMsgReadResp( gReadTaskId, pMsg->result, pMsg->pBuffer, pMsg->size );

    DEBUG_LOG_FNC( "%s(): end. ret=%u", __func__, STATE_INI );
    return STATE_INI;
}


/******************************************************************************/

