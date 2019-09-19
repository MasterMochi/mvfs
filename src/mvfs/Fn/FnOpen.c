/******************************************************************************/
/*                                                                            */
/* src/mvfs/Fn/FnOpen.c                                                       */
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
#include <MLib/MLibState.h>

/* 外部モジュールヘッダ */
#include <mvfs.h>
#include <Debug.h>
#include <Fd.h>
#include <Fn.h>
#include <Msg.h>
#include <Node.h>

/* 内部モジュールヘッダ */
#include "FnOpen.h"


/******************************************************************************/
/* 定義                                                                       */
/******************************************************************************/
/* 状態 */
#define STATE_INI               ( 1 )   /**< 初期状態        */
#define STATE_VFSOPEN_RESP_WAIT ( 2 )   /**< vfsOpen応答待ち */

/* イベント */
#define EVENT_OPEN_REQ     ( 1 )    /**< Open要求イベント    */
#define EVENT_VFSOPEN_RESP ( 2 )    /**< vfsOpen応答イベント */

/** 状態遷移タスクパラメータ */
typedef struct {
    MkTaskId_t taskId;      /**< タスクID         */
    void       *pBuffer;    /**< イベントバッファ */
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
    /*-----------------------+-------------------+---------+---------------------------------------------------------*/
    /* 状態                  | イベント          | タスク  | { 遷移先状態                                          } */
    /*-----------------------+-------------------+---------+---------------------------------------------------------*/
    { STATE_INI              , EVENT_OPEN_REQ    , Task0101, { STATE_INI              , STATE_VFSOPEN_RESP_WAIT, 0 } },
    { STATE_INI              , EVENT_VFSOPEN_RESP, NULL    , { STATE_INI              , 0                          } },
    { STATE_VFSOPEN_RESP_WAIT, EVENT_OPEN_REQ    , NULL    , { STATE_VFSOPEN_RESP_WAIT, 0                          } },
    { STATE_VFSOPEN_RESP_WAIT, EVENT_VFSOPEN_RESP, Task0202, { STATE_INI              , 0                          } }  };
    /*-----------------------+-------------------+---------+---------------------------------------------------------*/

/** 状態遷移ハンドル */
static MLibStateHandle_t gStateHdl;

/* TODO */
static MkTaskId_t gOpenTaskId;
static uint32_t gGlobalFd;


/******************************************************************************/
/* 外部モジュール向けグローバル関数定義                                       */
/******************************************************************************/
/******************************************************************************/
/**
 * @brief       vfsOpen応答メッセージ受信
 * @details     メッセージ送信元タスクID(taskId)からのvfsOpen応答メッセージを処
 *              理する。
 *
 * @param[in]   taskId   メッセージ送信元タスクID
 * @param[in]   *pBuffer メッセージバッファ
 */
/******************************************************************************/
void FnOpenRecvVfsOpenResp( MkTaskId_t taskId,
                            void       *pBuffer )
{
    uint32_t         errNo;     /* エラー番号               */
    MLibRet_t        retMLib;   /* MLIB戻り値               */
    MLibState_t      prevState; /* 遷移前状態               */
    MLibState_t      nextState; /* 遷移後状態               */
    StateTaskParam_t param;     /* 状態遷移タスクパラメータ */

    /* 初期化 */
    errNo     = MLIB_STATE_ERR_NONE;
    retMLib   = MLIB_FAILURE;
    prevState = MLIB_STATE_NULL;
    nextState = MLIB_STATE_NULL;
    memset( &param, 0, sizeof ( param ) );

    DEBUG_LOG_TRC(
        "%s(): start. taskId=%d, pBuffer=%p",
        __func__,
        taskId,
        pBuffer
    );

    /* 状態遷移パラメータ設定 */
    param.taskId  = taskId;
    param.pBuffer = pBuffer;

    /* 状態遷移実行 */
    retMLib = MLibStateExec( &gStateHdl,
                             EVENT_VFSOPEN_RESP,
                             &param,
                             &prevState,
                             &nextState,
                             &errNo              );

    /* 実行結果判定 */
    if ( retMLib != MLIB_SUCCESS ) {
        /* 失敗 */

        DEBUG_LOG_ERR( "MLibStateExec(): ret=%d, err=0x%X", retMLib, errNo );
        return;
    }

    DEBUG_LOG_TRC( "%s(): exec. state=%u->%u", __func__, prevState, nextState );
    DEBUG_LOG_TRC( "%s(): end.", __func__ );
    return;
}


/******************************************************************************/
/**
 * @brief       Open要求メッセージ受信
 * @details     メッセージ送信元タスクID(taskId)からのOpen要求メッセージを処理
 *              する。
 *
 * @param[in]   taskId   メッセージ送信元タスクID
 * @param[in]   *pBuffer メッセージバッファ
 */
/******************************************************************************/
void FnOpenRecvOpenReq( MkTaskId_t taskId,
                        void       *pBuffer )
{
    uint32_t         errNo;     /* エラー番号               */
    MLibRet_t        retMLib;   /* MLIB戻り値               */
    NodeInfo_t       *pNode;    /* ノード                   */
    MLibState_t      prevState; /* 遷移前状態               */
    MLibState_t      nextState; /* 遷移後状態               */
    MvfsMsgOpenReq_t *pMsg;     /* メッセージ               */
    StateTaskParam_t param;     /* 状態遷移タスクパラメータ */

    /* 初期化 */
    errNo     = MLIB_STATE_ERR_NONE;
    retMLib   = MLIB_FAILURE;
    pNode     = NULL;
    prevState = MLIB_STATE_NULL;
    nextState = MLIB_STATE_NULL;
    pMsg      = ( MvfsMsgOpenReq_t * ) pBuffer;
    memset( &param, 0, sizeof ( param ) );

    DEBUG_LOG_TRC(
        "%s(): start. taskId=0x%X, localFd=%d, path=%s",
        __func__,
        taskId,
        pMsg->localFd,
        pMsg->path
    );

    /* 絶対パス判定 */
    if ( pMsg->path[ 0 ] != '/' ) {
        /* 絶対パスでない */

        DEBUG_LOG_ERR( "invalid path: %s", pMsg->path );

        /* Open応答メッセージ送信 */
        MsgSendOpenResp( taskId, MVFS_RESULT_FAILURE, MVFS_FD_NULL );

        DEBUG_LOG_TRC( "%s(): end.", __func__ );
        return;
    }

    /* 状態遷移パラメータ設定 */
    param.taskId  = taskId;
    param.pBuffer = pBuffer;

    /* 状態遷移実行 */
    retMLib = MLibStateExec( &gStateHdl,
                             EVENT_OPEN_REQ,
                             &param,
                             &prevState,
                             &nextState,
                             &errNo          );

    /* 実行結果判定 */
    if ( retMLib != MLIB_SUCCESS ) {
        /* 失敗 */

        DEBUG_LOG_ERR( "MLibStateExec(): ret=%d, err=0x%X", retMLib, errNo );

        /* Open応答メッセージ送信 */
        MsgSendOpenResp( taskId, MVFS_RESULT_FAILURE, MVFS_FD_NULL );

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
 * @brief       Open機能初期化
 * @details     状態遷移を初期化する。
 */
/******************************************************************************/
void OpenInit( void )
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
 * @details     ファイルディスクリプタ払い出しを行い、vfsOpen要求メッセージを送
 *              信する。
 *
 * @param[in]   *pArg 状態遷移パラメータ
 *
 * @return      遷移先状態を返す。
 * @retval      STATE_INI               初期状態
 * @retval      STATE_VFSOPEN_RESP_WAIT vfsOpen応答待ち
 */
/******************************************************************************/
static MLibState_t Task0101( void *pArg )
{
    uint32_t         globalFd;  /* グローバルFD       */
    NodeInfo_t       *pNode;    /* ノード             */
    StateTaskParam_t *pParam;   /* 状態遷移パラメータ */
    MvfsMsgOpenReq_t *pMsg;     /* Open要求メッセージ */

    DEBUG_LOG_FNC( "%s(): start. pArg=%p", __func__, pArg );

    /* 初期化 */
    pNode  = NULL;
    pParam = ( StateTaskParam_t * ) pArg;
    pMsg   = ( MvfsMsgOpenReq_t * ) pParam->pBuffer;

    /* パスからノード取得 */
    pNode = NodeGet( pMsg->path );

    /* 取得結果判定 */
    if ( pNode == NULL ) {
        /* ノード無し */

        DEBUG_LOG_ERR( "not exist: path=%s", pMsg->path );

        /* Open応答メッセージ送信 */
        MsgSendOpenResp( pParam->taskId, MVFS_RESULT_FAILURE, MVFS_FD_NULL );

        DEBUG_LOG_FNC( "%s(): end. ret=%u", __func__, STATE_INI );
        return STATE_INI;
    }

    /* ノードタイプチェック */
    if ( pNode->type != NODE_TYPE_MOUNT_FILE ) {
        /* マウントファイルでない */

        DEBUG_LOG_ERR(
            "not mount file: path=%s, type=%u",
            pMsg->path,
            pNode->type
        );

        /* Open応答メッセージ送信 */
        MsgSendOpenResp( pParam->taskId, MVFS_RESULT_FAILURE, MVFS_FD_NULL );

        DEBUG_LOG_FNC( "%s(): end. ret=%u", __func__, STATE_INI );
        return STATE_INI;
    }

    /* PID毎のFD払い出し */
    globalFd = FdAlloc( pMsg->localFd,
                        MK_TASKID_TO_PID( pParam->taskId ),
                        pNode                               );

    /* vfsOpen要求メッセージ送信 */
    MsgSendVfsOpenReq( pNode->mountTaskId,
                       MK_TASKID_TO_PID( pParam->taskId ),
                       globalFd,
                       pMsg->path                          );

    /* [TODO]Open要求元タスクID保存 */
    gOpenTaskId = pParam->taskId;
    gGlobalFd   = globalFd;

    DEBUG_LOG_FNC( "%s(): end. ret=%u", __func__, STATE_VFSOPEN_RESP_WAIT );
    return STATE_VFSOPEN_RESP_WAIT;
}


/******************************************************************************/
/**
 * @brief       状態遷移タスク0202
 * @details     Open応答メッセージを送信する。
 *
 * @param[in]   *pArg 未使用
 *
 * @return      遷移先状態を返す。
 * @retval      STATE_INI               初期状態
 * @retval      STATE_VFSOPEN_RESP_WAIT vfsOpen応答待ち
 */
/******************************************************************************/
static MLibState_t Task0202( void *pArg )
{
    DEBUG_LOG_FNC( "%s(): start. pArg=%p", __func__, pArg );

    /* Open応答メッセージ送信 */
    MsgSendOpenResp( gOpenTaskId, MVFS_RESULT_SUCCESS, gGlobalFd );

    DEBUG_LOG_FNC( "%s(): end. ret=%u", __func__, STATE_INI );
    return STATE_INI;
}


/******************************************************************************/
