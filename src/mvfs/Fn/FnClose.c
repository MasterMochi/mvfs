/******************************************************************************/
/*                                                                            */
/* src/mvfs/Fn/FnClose.c                                                      */
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
#include <kernel/types.h>

/* ライブラリヘッダ */
#include <libmk.h>
#include <MLib/MLibState.h>

/* 外部モジュールヘッダ */
#include <mvfs.h>
#include <Debug.h>
#include <Fd.h>
#include <Msg.h>


/******************************************************************************/
/* 定義                                                                       */
/******************************************************************************/
/* 状態 */
#define STATE_INI                ( 1 )  /**< 初期状態     */
#define STATE_VFSCLOSE_RESP_WAIT ( 2 )  /**< VfsClose待ち */

/* イベント */
#define EVENT_CLOSE_REQ     ( 1 )   /**< Close要求イベント    */
#define EVENT_VFSCLOSE_RESP ( 2 )   /**< VfsClose応答イベント */

/** 状態遷移タスクパラメータ */
typedef struct {
    MkTaskId_t taskId;      /**< タスクID         */
    void       *pBuffer;    /**< イベントバッファ */
    FdInfo_t   *pFdInfo;    /**< FD情報           */
} StateTaskParam_t;


/******************************************************************************/
/* ローカル関数宣言                                                           */
/******************************************************************************/
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
/* 外部モジュール向けグローバル関数定義                                       */
/******************************************************************************/
/******************************************************************************/
/**
 * @brief       Close要求メッセージ受信
 * @details     メッセージ送信元タスクID(taskId)からのClose要求メッセージを処理
 *              する。
 *
 * @param[in]   taskId   メッセージ送信元タスクID
 * @param[in]   *pBuffer メッセージバッファ
 */
/******************************************************************************/
void FnCloseRecvCloseReq( MkTaskId_t taskId,
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

        /* Close応答メッセージ送信 */
        MsgSendCloseResp( taskId, MVFS_RESULT_FAILURE );

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

        /* Close応答メッセージ送信 */
        MsgSendCloseResp( taskId, MVFS_RESULT_FAILURE );

        DEBUG_LOG_TRC( "%s(): end.", __func__ );
        return;
    }

    DEBUG_LOG_TRC( "state: %u->%u", prevState, nextState );

    DEBUG_LOG_TRC( "%s(): end.", __func__ );
    return;
}


/******************************************************************************/
/**
 * @brief       VfsClose応答メッセージ受信
 * @details     メッセージ送信元タスクID(taskId)からのVfsClose応答メッセージを
 *              処理する。
 *
 * @param[in]   taskId   メッセージ送信元タスクID
 * @param[in]   *pBuffer メッセージバッファ
 */
/******************************************************************************/
void FnCloseRecvVfsCloseResp( MkTaskId_t taskId,
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
/* 内部モジュール向けグローバル関数定義                                       */
/******************************************************************************/
/******************************************************************************/
/**
 * @brief       Close機能初期化
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
/* ローカル関数定義                                                           */
/******************************************************************************/
/******************************************************************************/
/**
 * @brief       状態遷移タスク0101
 * @details     VfsClose要求メッセージを送信する。
 *
 * @param[in]   *pArg 状態遷移パラメータ
 *
 * @return      遷移先状態を返す。
 * @retval      STATE_INI                初期状態
 * @retval      STATE_VFSCLOSE_RESP_WAIT VfsClose応答待ち
 */
/******************************************************************************/
static MLibState_t Task0101( void *pArg )
{
    NodeInfo_t        *pNode;   /* ノード              */
    StateTaskParam_t  *pParam;  /* 状態遷移パラメータ  */
    MvfsMsgCloseReq_t *pMsg;    /* Close要求メッセージ */

    DEBUG_LOG_FNC( "%s(): start. pArg=%p", __func__, pArg );

    /* 初期化 */
    pParam = ( StateTaskParam_t  * ) pArg;
    pNode  = pParam->pFdInfo->pNode;
    pMsg   = ( MvfsMsgCloseReq_t * ) pParam->pBuffer;

    /* VfsClose要求メッセージ送信 */
    MsgSendVfsCloseReq( pNode->mountTaskId,
                        pMsg->globalFd      );

    /* [TODO]Close要求元タスクID保存 */
    gCloseTaskId = pParam->taskId;

    DEBUG_LOG_FNC( "%s(): end. ret=%u", __func__, STATE_VFSCLOSE_RESP_WAIT );
    return STATE_VFSCLOSE_RESP_WAIT;
}


/******************************************************************************/
/**
 * @brief       状態遷移タスク0202
 * @details     Close応答メッセージを送信する。
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
    MvfsMsgVfsCloseResp_t *pMsg;    /* VfsClose応答メッセージ */

    DEBUG_LOG_FNC( "%s(): start. pArg=%p", __func__, pArg );

    /* 初期化 */
    pParam = ( StateTaskParam_t      * ) pArg;
    pMsg   = ( MvfsMsgVfsCloseResp_t * ) pParam->pBuffer;

    /* ファイルディスクリプタ解放 */
    FdFree( pParam->pFdInfo );

    /* Close応答メッセージ送信 */
    MsgSendCloseResp( gCloseTaskId, pMsg->result );

    DEBUG_LOG_FNC( "%s(): end. ret=%u", __func__, STATE_INI );
    return STATE_INI;
}


/******************************************************************************/

