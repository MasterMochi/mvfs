/******************************************************************************/
/*                                                                            */
/* src/mvfs/Open.c                                                            */
/*                                                                 2019/07/28 */
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
#include <libmlog.h>
#include <MLib/MLibState.h>

/* モジュール共通ヘッダ */
#include <mvfs.h>

/* モジュールヘッダ */
#include "Fd.h"
#include "Open.h"
#include "Node.h"


/******************************************************************************/
/* 定義                                                                       */
/******************************************************************************/
/* 状態 */
#define STATE_INI               ( 1 )   /**< 初期状態        */
#define STATE_VFSOPEN_RESP_WAIT ( 2 )   /**< vfsOpen応答待ち */

/* イベント */
#define EVENT_OPEN_REQ     ( 1 )    /**< open要求イベント    */
#define EVENT_VFSOPEN_RESP ( 2 )    /**< vfsOpen応答イベント */

/** 状態遷移タスクパラメータ */
typedef struct {
    MkTaskId_t taskId;      /**< タスクID         */
    void       *pBuffer;    /**< イベントバッファ */
} StateTaskParam_t;


/******************************************************************************/
/* ローカル関数宣言                                                           */
/******************************************************************************/
/* open応答メッセージ送信 */
static void SendMsgOpenResp( MkTaskId_t taskId,
                             uint32_t   result,
                             uint32_t   globalFd );
/* vfsOpen要求メッセージ送信 */
static void SendMsgVfsOpenReq( MkTaskId_t dst,
                               MkPid_t    pid,
                               uint32_t   fd,
                               const char *pPath );

/* 状態遷移タスク */
static MLibState_t Task0101( void *pArg );
static MLibState_t Task0202( void *pArg );


/******************************************************************************/
/* 静的グローバル変数定義                                                     */
/******************************************************************************/
/** 状態遷移表 */
static const MLibStateTransition_t gStt[] = {
    /*-----------------------+-------------------+---------+--------------------------------*/
    /* 状態                  | イベント          | タスク  | { 遷移先状態                 } */
    /*-----------------------+-------------------+---------+--------------------------------*/
    { STATE_INI              , EVENT_OPEN_REQ    , Task0101, { STATE_VFSOPEN_RESP_WAIT, 0 } },
    { STATE_INI              , EVENT_VFSOPEN_RESP, NULL    , { STATE_INI              , 0 } },
    { STATE_VFSOPEN_RESP_WAIT, EVENT_OPEN_REQ    , NULL    , { STATE_VFSOPEN_RESP_WAIT, 0 } },
    { STATE_VFSOPEN_RESP_WAIT, EVENT_VFSOPEN_RESP, Task0202, { STATE_INI              , 0 } }  };
    /*-----------------------+-------------------+---------+--------------------------------*/

/** 状態遷移ハンドル */
static MLibStateHandle_t gStateHdl;

/* TODO */
static MkTaskId_t gOpenTaskId;
static uint32_t gGlobalFd;


/******************************************************************************/
/* グローバル関数定義                                                         */
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

        LibMlogPut(
            "[mvfs][%s:%d] %s(): MLibStateInit() error. ret=%d, errNo=%#X",
            __FILE__,
            __LINE__,
            __func__,
            ret,
            errNo
        );
    }

    return;
}


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
void OpenRcvMsgVfsOpenResp( MkTaskId_t taskId,
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

    LibMlogPut(
        "[mvfs][%s:%d] %s() start. taskId=%d",
        __FILE__,
        __LINE__,
        __func__,
        taskId
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

        LibMlogPut(
            "[mvfs][%s:%d] %s(): MLibStateExec() error. ret=%d, errNo=%#X",
            __FILE__,
            __LINE__,
            __func__,
            retMLib,
            errNo
        );

        return;
    }

    LibMlogPut(
        "[mvfs][%s:%d] %s(): exec. state=%u->%u",
        __FILE__,
        __LINE__,
        __func__,
        prevState,
        nextState
    );

    return;
}


/******************************************************************************/
/**
 * @brief       open要求メッセージ受信
 * @details     メッセージ送信元タスクID(taskId)からのopen要求メッセージを処理
 *              する。
 *
 * @param[in]   taskId   メッセージ送信元タスクID
 * @param[in]   *pBuffer メッセージバッファ
 */
/******************************************************************************/
void OpenRcvMsgOpenReq( MkTaskId_t taskId,
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

    LibMlogPut(
        "[mvfs][%s:%d] %s() start. taskId=%d, localFd=%d, path=%s.",
        __FILE__,
        __LINE__,
        __func__,
        taskId,
        pMsg->localFd,
        pMsg->path
    );

    /* 絶対パス判定 */
    if ( pMsg->path[ 0 ] != '/' ) {
        /* 絶対パスでない */

        LibMlogPut(
            "[mvfs][%s:%d] %s() error. taskId=%u, path=%s.",
            __FILE__,
            __LINE__,
            __func__,
            taskId,
            pMsg->path
        );

        /* open応答メッセージ送信 */
        SendMsgOpenResp( taskId, MVFS_RESULT_FAILURE, MVFS_FD_NULL );

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

        LibMlogPut(
            "[mvfs][%s:%d] %s(): MLibStateExec() error. ret=%d, errNo=%#X",
            __FILE__,
            __LINE__,
            __func__,
            retMLib,
            errNo
        );

        /* open応答メッセージ送信 */
        SendMsgOpenResp( taskId, MVFS_RESULT_FAILURE, MVFS_FD_NULL );

        return;
    }

    LibMlogPut(
        "[mvfs][%s:%d] %s(): exec. state=%u->%u",
        __FILE__,
        __LINE__,
        __func__,
        prevState,
        nextState
    );

    return;
}


/******************************************************************************/
/* ローカル関数定義                                                           */
/******************************************************************************/
/******************************************************************************/
/**
 * @brief       open応答メッセージ送信
 * @details     メッセージ送信先タスクID(dst)にopen応答メッセージを送信する。
 *
 * @param[in]   dst      メッセージ送信先タスクID
 * @param[in]   result   処理結果
 *                  - MVFS_RESULT_SUCCESS 成功
 *                  - MVFS_RESULT_FAILURE 失敗
 * @param[in]   globalFd グローバルFD
 */
/******************************************************************************/
static void SendMsgOpenResp( MkTaskId_t dst,
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

    LibMlogPut(
        "[mvfs][%s:%d] %s() dst=%u, result=%u, globalFd=%u.",
        __FILE__,
        __LINE__,
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

        LibMlogPut(
            "[mvfs][%s:%d] %s() error. ret=%d, err=%#x",
            __FILE__,
            __LINE__,
            __func__,
            ret,
            err
        );
    }

    return;
}


/******************************************************************************/
/**
 * @brief       vfsOpen要求メッセージ送信
 * @details     メッセージ送信先タスクID(dst)にvfsOpen要求メッセージを送信する。
 *
 * @param[in]   dst      メッセージ送信先タスクID
 * @param[in]   pid      プロセスID
 * @param[in]   globalFd グローバルFD
 * @param[in]   *pPath   ファイルパス（絶対パス）
 */
/******************************************************************************/
static void SendMsgVfsOpenReq( MkTaskId_t dst,
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

    LibMlogPut(
        "[mvfs][%s:%d] %s() dst=%u, pid=%u, globalFd=%u, pPath=%s",
        __FILE__,
        __LINE__,
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

        LibMlogPut(
            "[mvfs][%s:%d] %s() error. ret=%d, err=%#x",
            __FILE__,
            __LINE__,
            __func__,
            ret,
            err
        );
    }

    return;
}


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
    MvfsMsgOpenReq_t *pMsg;     /* open要求メッセージ */

    /* 初期化 */
    pNode  = NULL;
    pParam = ( StateTaskParam_t * ) pArg;
    pMsg   = ( MvfsMsgOpenReq_t * ) pParam->pBuffer;

    /* パスからノード取得 */
    pNode = NodeGet( pMsg->path );

    /* 取得結果判定 */
    if ( pNode == NULL ) {
        /* ノード無し */

        LibMlogPut(
            "[mvfs][%s:%d] %s(): not exist. path=%s",
            __FILE__,
            __LINE__,
            __func__,
            pMsg->path
        );

        /* open応答メッセージ送信 */
        SendMsgOpenResp( pParam->taskId, MVFS_RESULT_FAILURE, MVFS_FD_NULL );

        return STATE_INI;
    }

    /* ノードタイプチェック */
    if ( pNode->type != NODE_TYPE_MOUNT_FILE ) {
        /* マウントファイルでない */

        LibMlogPut(
            "[mvfs][%s:%d] %s(): not mount file. path=%s, type=%u",
            __FILE__,
            __LINE__,
            __func__,
            pMsg->path,
            pNode->type
        );

        /* open応答メッセージ送信 */
        SendMsgOpenResp( pParam->taskId, MVFS_RESULT_FAILURE, MVFS_FD_NULL );

        return STATE_INI;
    }

    /* PID毎のFD払い出し */
    globalFd = FdAlloc( pMsg->localFd,
                        MK_TASKID_TO_PID( pParam->taskId ),
                        pNode                               );

    /* vfsOpen要求メッセージ送信 */
    SendMsgVfsOpenReq( pNode->mountTaskId,
                       MK_TASKID_TO_PID( pParam->taskId ),
                       globalFd,
                       pMsg->path                          );

    /* [TODO]open要求元タスクID保存 */
    gOpenTaskId = pParam->taskId;
    gGlobalFd   = globalFd;

    return STATE_VFSOPEN_RESP_WAIT;
}


/******************************************************************************/
/**
 * @brief       状態遷移タスク0202
 * @details     open応答メッセージを送信する。
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
    /* open応答メッセージ送信 */
    SendMsgOpenResp( gOpenTaskId, MVFS_RESULT_SUCCESS, gGlobalFd );

    return STATE_INI;
}


/******************************************************************************/
