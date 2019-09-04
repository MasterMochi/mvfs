/******************************************************************************/
/*                                                                            */
/* src/mvfs/Close.c                                                           */
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
#define STATE_INI                ( 1 )  /**< 初期状態     */
#define STATE_VFSCLOSE_RESP_WAIT ( 2 )  /**< vfsClose待ち */

/* イベント */
#define EVENT_CLOSE_REQ     ( 1 )   /**< close要求イベント    */
#define EVENT_VFSCLOSE_RESP ( 2 )   /**< vfsClose応答イベント */

/** 状態遷移タスクパラメータ */
typedef struct {
    MkTaskId_t taskId;      /**< タスクID         */
    void       *pBuffer;    /**< イベントバッファ */
    FdInfo_t   *pFdInfo;    /**< FD情報           */
} StateTaskParam_t;


/******************************************************************************/
/* ローカル関数宣言                                                           */
/******************************************************************************/
/* close応答メッセージ送信 */
static void SendMsgCloseResp( MkTaskId_t taskId,
                              uint32_t   result  );
/* vfsClose要求メッセージ送信 */
static void SendMsgVfsCloseReq( MkTaskId_t dst,
                                uint32_t   globalFd );

/* 状態遷移タスク */
static MLibState_t Task0101( void *pArg );
static MLibState_t Task0202( void *pArg );


/******************************************************************************/
/* 静的グローバル変数定義                                                     */
/******************************************************************************/
/** 状態遷移表 */
static const MLibStateTransition_t gStt[] = {
    { STATE_INI               , EVENT_CLOSE_REQ    , Task0101, { STATE_VFSCLOSE_RESP_WAIT, 0 } },
    { STATE_INI               , EVENT_VFSCLOSE_RESP, NULL    , { STATE_INI               , 0 } },
    { STATE_VFSCLOSE_RESP_WAIT, EVENT_CLOSE_REQ    , NULL    , { STATE_VFSCLOSE_RESP_WAIT, 0 } },
    { STATE_VFSCLOSE_RESP_WAIT, EVENT_VFSCLOSE_RESP, Task0202, { STATE_INI               , 0 } }  };

/** 状態遷移ハンドル */
static MLibStateHandle_t gStateHdl;

/* TODO */
static MkTaskId_t gCloseTaskId;


/******************************************************************************/
/* グローバル関数定義                                                         */
/******************************************************************************/
/******************************************************************************/
/**
 * @brief       close機能初期化
 * @details     状態遷移を初期化する。
 */
/******************************************************************************/
void CloseInit( void )
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
 * @brief       vfsClose応答メッセージ受信
 * @details     メッセージ送信元タスクID(taskId)からのvfsClose応答メッセージを
 *              処理する。
 *
 * @param[in]   taskId   メッセージ送信元タスクID
 * @param[in]   *pBuffer メッセージバッファ
 */
/******************************************************************************/
void CloseRcvMsgVfsCloseResp( MkTaskId_t taskId,
                              void       *pBuffer )
{
    uint32_t              errNo;        /* エラー番号               */
    FdInfo_t              *pFdInfo;     /* FD情報                   */
    MLibRet_t             retMLib;      /* MLIB戻り値               */
    MLibState_t           prevState;    /* 遷移前状態               */
    MLibState_t           nextState;    /* 遷移後状態               */
    StateTaskParam_t      param;        /* 状態遷移タスクパラメータ */
    MvfsMsgVfsCloseResp_t *pMsg;        /* メッセージ               */

    /* 初期化 */
    errNo     = MLIB_STATE_ERR_NONE;
    pFdInfo   = NULL;
    retMLib   = MLIB_FAILURE;
    prevState = MLIB_STATE_NULL;
    nextState = MLIB_STATE_NULL;
    pMsg      = ( MvfsMsgVfsCloseResp_t * ) pBuffer;
    memset( &param, 0, sizeof ( param ) );

    DEBUG_LOG_TRC(
        "%s(): start. taskId=0x%X, result=%u",
        __func__,
        taskId,
        pMsg->result
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
                             EVENT_VFSCLOSE_RESP,
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
 * @brief       close要求メッセージ受信
 * @details     メッセージ送信元タスクID(taskId)からのclose要求メッセージを処理
 *              する。
 *
 * @param[in]   taskId   メッセージ送信元タスクID
 * @param[in]   *pBuffer メッセージバッファ
 */
/******************************************************************************/
void CloseRcvMsgCloseReq( MkTaskId_t taskId,
                          void       *pBuffer )
{
    uint32_t          errNo;        /* エラー番号               */
    FdInfo_t          *pFdInfo;     /* FD情報                   */
    MLibRet_t         retMLib;      /* MLIB戻り値               */
    NodeInfo_t        *pNode;       /* ノード                   */
    MLibState_t       prevState;    /* 遷移前状態               */
    MLibState_t       nextState;    /* 遷移後状態               */
    StateTaskParam_t  param;        /* 状態遷移タスクパラメータ */
    MvfsMsgCloseReq_t *pMsg;        /* メッセージ               */

    /* 初期化 */
    errNo     = MLIB_STATE_ERR_NONE;
    pFdInfo   = NULL;
    retMLib   = MLIB_FAILURE;
    pNode     = NULL;
    prevState = MLIB_STATE_NULL;
    nextState = MLIB_STATE_NULL;
    pMsg      = ( MvfsMsgCloseReq_t * ) pBuffer;
    memset( &param, 0, sizeof ( param ) );

    DEBUG_LOG_TRC(
        "%s(): start. taskId=0x%X, globalFd=%u",
        __func__,
        taskId,
        pMsg->globalFd
    );

    /* FD情報取得 */
    pFdInfo = FdGet( pMsg->globalFd );

    /* 取得結果判定 */
    if ( pFdInfo == NULL ) {
        /* 失敗 */

        DEBUG_LOG_ERR( "FdGet(): %u", pMsg->globalFd );

        /* close応答メッセージ送信 */
        SendMsgCloseResp( taskId, MVFS_RESULT_FAILURE );

        DEBUG_LOG_TRC( "%s(): end.", __func__ );
        return;
    }

    /* 状態遷移パラメータ設定 */
    param.taskId  = taskId;
    param.pBuffer = pBuffer;
    param.pFdInfo = pFdInfo;

    /* 状態遷移実行 */
    retMLib = MLibStateExec( &gStateHdl,
                             EVENT_CLOSE_REQ,
                             &param,
                             &prevState,
                             &nextState,
                             &errNo          );

    /* 実行結果判定 */
    if ( retMLib != MLIB_SUCCESS ) {
        /* 失敗 */

        DEBUG_LOG_ERR( "MLibStateExec(): ret=%d, err=0x%X", retMLib, errNo );

        /* close応答メッセージ送信 */
        SendMsgCloseResp( taskId, MVFS_RESULT_FAILURE );

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
 * @brief       close応答メッセージ送信
 * @details     メッセージ送信先タスクID(dst)にclose応答メッセージを送信する。
 *
 * @param[in]   dst      メッセージ送信先タスクID
 * @param[in]   result   処理結果
 *                  - MVFS_RESULT_SUCCESS 成功
 *                  - MVFS_RESULT_FAILURE 失敗
 */
/******************************************************************************/
static void SendMsgCloseResp( MkTaskId_t dst,
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
 * @brief       vfsClose要求メッセージ送信
 * @details     メッセージ送信先タスクID(dst)にvfsClose要求メッセージを送信する。
 *
 * @param[in]   dst      メッセージ送信先タスクID
 * @param[in]   globalFd グローバルFD
 */
/******************************************************************************/
static void SendMsgVfsCloseReq( MkTaskId_t dst,
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
 * @brief       状態遷移タスク0101
 * @details     vfsClose要求メッセージを送信する。
 *
 * @param[in]   *pArg 状態遷移パラメータ
 *
 * @return      遷移先状態を返す。
 * @retval      STATE_INI                初期状態
 * @retval      STATE_VFSCLOSE_RESP_WAIT vfsClose応答待ち
 */
/******************************************************************************/
static MLibState_t Task0101( void *pArg )
{
    NodeInfo_t        *pNode;   /* ノード              */
    StateTaskParam_t  *pParam;  /* 状態遷移パラメータ  */
    MvfsMsgCloseReq_t *pMsg;    /* close要求メッセージ */

    DEBUG_LOG_FNC( "%s(): start. pArg=%p", __func__, pArg );

    /* 初期化 */
    pParam = ( StateTaskParam_t  * ) pArg;
    pNode  = pParam->pFdInfo->pNode;
    pMsg   = ( MvfsMsgCloseReq_t * ) pParam->pBuffer;

    /* vfsClose要求メッセージ送信 */
    SendMsgVfsCloseReq( pNode->mountTaskId,
                        pMsg->globalFd      );

    /* [TODO]close要求元タスクID保存 */
    gCloseTaskId = pParam->taskId;

    DEBUG_LOG_FNC( "%s(): end. ret=%u", __func__, STATE_VFSCLOSE_RESP_WAIT );
    return STATE_VFSCLOSE_RESP_WAIT;
}


/******************************************************************************/
/**
 * @brief       状態遷移タスク0202
 * @details     close応答メッセージを送信する。
 *
 * @param[in]   *pArg 未使用
 *
 * @return      遷移先状態を返す。
 * @retval      STATE_INI 初期状態
 */
/******************************************************************************/
static MLibState_t Task0202( void *pArg )
{
    StateTaskParam_t      *pParam;  /* 状態遷移パラメータ     */
    MvfsMsgVfsCloseResp_t *pMsg;    /* vfsClose応答メッセージ */

    DEBUG_LOG_FNC( "%s(): start. pArg=%p", __func__, pArg );

    /* 初期化 */
    pParam = ( StateTaskParam_t      * ) pArg;
    pMsg   = ( MvfsMsgVfsCloseResp_t * ) pParam->pBuffer;

    /* ファイルディスクリプタ解放 */
    FdFree( pParam->pFdInfo );

    /* close応答メッセージ送信 */
    SendMsgCloseResp( gCloseTaskId, pMsg->result );

    DEBUG_LOG_FNC( "%s(): end. ret=%u", __func__, STATE_INI );
    return STATE_INI;
}


/******************************************************************************/

