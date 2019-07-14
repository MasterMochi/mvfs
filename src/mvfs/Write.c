/******************************************************************************/
/*                                                                            */
/* src/mvfs/Write.c                                                           */
/*                                                                 2019/07/13 */
/* Copyright (C) 2019 Mochi.                                                  */
/*                                                                            */
/******************************************************************************/
/******************************************************************************/
/* インクルード                                                               */
/******************************************************************************/
/* 標準ヘッダ */
#include <stddef.h>
#include <stdint.h>
#include <string.h>

/* カーネルヘッダ */
#include <kernel/types.h>

/* ライブラリヘッダ */
#include <libMk.h>
#include <libmlog.h>
#include <MLib/MLibState.h>

/* モジュール共通ヘッダ */
#include <mvfs.h>

/* モジュールヘッダ */
#include "Fd.h"


/******************************************************************************/
/* 定義                                                                       */
/******************************************************************************/
/* 状態 */
#define STATE_INI                ( 1 )  /**< 初期状態     */
#define STATE_VFSWRITE_RESP_WAIT ( 2 )  /**< vfsWrite待ち */

/* イベント */
#define EVENT_WRITE_REQ     ( 1 )   /**< write要求イベント    */
#define EVENT_VFSWRITE_RESP ( 2 )   /**< vdsWrite応答イベント */

/** 状態遷移タスクパラメータ */
typedef struct {
    MkTaskId_t taskId;      /**< タスクID         */
    void       *pBuffer;    /**< イベントバッファ */
    FdInfo_t   *pFdInfo;    /**< FD情報           */
} StateTaskParam_t;


/******************************************************************************/
/* ローカル関数宣言                                                           */
/******************************************************************************/
/* write応答メッセージ送信 */
static void SendMsgWriteResp( MkTaskId_t taskId,
                              uint32_t   globalFd,
                              size_t     size      );
/* vfsWrite要求メッセージ送信 */
static void SendMsgVfsWriteReq( MkTaskId_t dst,
                                uint32_t   globalFd,
                                uint64_t   writeIdx,
                                const char *pBuffer,
                                size_t     size      );

/* 状態遷移タスク */
static MLibState_t Task0101( void *pArg );
static MLibState_t Task0202( void *pArg );


/******************************************************************************/
/* 静的グローバル変数定義                                                     */
/******************************************************************************/
/** 状態遷移表 */
static const MLibStateTransition_t gStt[] = {
    { STATE_INI               , EVENT_WRITE_REQ    , Task0101, { STATE_VFSWRITE_RESP_WAIT, 0 } },
    { STATE_INI               , EVENT_VFSWRITE_RESP, NULL    , { STATE_INI               , 0 } },
    { STATE_VFSWRITE_RESP_WAIT, EVENT_WRITE_REQ    , NULL    , { STATE_VFSWRITE_RESP_WAIT, 0 } },
    { STATE_VFSWRITE_RESP_WAIT, EVENT_VFSWRITE_RESP, Task0202, { STATE_INI               , 0 } }  };

/** 状態遷移ハンドル */
static MLibStateHandle_t gStateHdl;

/* TODO */
static MkTaskId_t gWriteTaskId;


/******************************************************************************/
/* グローバル関数定義                                                         */
/******************************************************************************/
/******************************************************************************/
/**
 * @brief       write機能初期化
 * @details     状態遷移を初期化する。
 */
/******************************************************************************/
void WriteInit( void )
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
            "[mvfs][%s:%d] %s(): MLibStateInit() error. ret=%d, errNo=0x%X",
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
 * @brief       vfsWrite応答メッセージ受信
 * @details     メッセージ送信元タスクID(taskId)からのvfsWrite応答メッセージを
 *              処理する。
 *
 * @param[in]   taskId   メッセージ送信元タスクID
 * @param[in]   *pBuffer メッセージバッファ
 */
/******************************************************************************/
void WriteRcvMsgVfsWriteResp( MkTaskId_t taskId,
                              void       *pBuffer )
{
    uint32_t              errNo;        /* エラー番号               */
    FdInfo_t              *pFdInfo;     /* FD情報                   */
    MLibRet_t             retMLib;      /* MLIB戻り値               */
    MLibState_t           prevState;    /* 遷移前状態               */
    MLibState_t           nextState;    /* 遷移後状態               */
    StateTaskParam_t      param;        /* 状態遷移タスクパラメータ */
    MvfsMsgVfsWriteResp_t *pMsg;        /* メッセージ               */

    /* 初期化 */
    errNo     = MLIB_STATE_ERR_NONE;
    pFdInfo   = NULL;
    retMLib   = MLIB_FAILURE;
    prevState = MLIB_STATE_NULL;
    nextState = MLIB_STATE_NULL;
    pMsg      = ( MvfsMsgVfsWriteResp_t * ) pBuffer;
    memset( &param, 0, sizeof ( param ) );

    LibMlogPut(
        "[mvfs][%s:%d] %s() start. taskId=%d, result=%u, size=%u",
        __FILE__,
        __LINE__,
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

        LibMlogPut(
            "[mvfs][%s:%d] %s(): FD error.",
            __FILE__,
            __LINE__,
            __func__
        );

        return;
    }

    /* 状態遷移パラメータ設定 */
    param.taskId  = taskId;
    param.pBuffer = pBuffer;
    param.pFdInfo = pFdInfo;

    /* 状態遷移実行 */
    retMLib = MLibStateExec( &gStateHdl,
                             EVENT_VFSWRITE_RESP,
                             &param,
                             &prevState,
                             &nextState,
                             &errNo               );

    /* 実行結果判定 */
    if ( retMLib != MLIB_SUCCESS ) {
        /* 失敗 */

        LibMlogPut(
            "[mvfs][%s:%d] %s(): MLibStateExec() error. ret=%d, errNo=0x%X",
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
 * @brief       write要求メッセージ受信
 * @details     メッセージ送信元タスクID(taskId)からのwrite要求メッセージを処理
 *              する。
 *
 * @param[in]   taskId   メッセージ送信元タスクID
 * @param[in]   *pBuffer メッセージバッファ
 */
/******************************************************************************/
void WriteRcvMsgWriteReq( MkTaskId_t taskId,
                          void       *pBuffer )
{
    uint32_t          errNo;        /* エラー番号               */
    FdInfo_t          *pFdInfo;     /* FD情報                   */
    MLibRet_t         retMLib;      /* MLIB戻り値               */
    NodeInfo_t        *pNode;       /* ノード                   */
    MLibState_t       prevState;    /* 遷移前状態               */
    MLibState_t       nextState;    /* 遷移後状態               */
    StateTaskParam_t  param;        /* 状態遷移タスクパラメータ */
    MvfsMsgWriteReq_t *pMsg;        /* メッセージ               */

    /* 初期化 */
    errNo     = MLIB_STATE_ERR_NONE;
    pFdInfo   = NULL;
    retMLib   = MLIB_FAILURE;
    pNode     = NULL;
    prevState = MLIB_STATE_NULL;
    nextState = MLIB_STATE_NULL;
    pMsg      = ( MvfsMsgWriteReq_t * ) pBuffer;
    memset( &param, 0, sizeof ( param ) );

    LibMlogPut(
        "[mvfs][%s:%d] %s() start. taskId=%d, globalFd=%d, writeIdx=0x%X, size=%u",
        __FILE__,
        __LINE__,
        __func__,
        taskId,
        pMsg->globalFd,
        ( uint32_t ) pMsg->writeIdx,
        pMsg->size
    );

    /* FD情報取得 */
    pFdInfo = FdGet( pMsg->globalFd );

    /* 取得結果判定 */
    if ( pFdInfo == NULL ) {
        /* 失敗 */

        LibMlogPut(
            "[mvfs][%s:%d] %s(): FD error.",
            __FILE__,
            __LINE__,
            __func__
        );

        /* write応答メッセージ送信 */
        SendMsgWriteResp( taskId, MVFS_RESULT_FAILURE, 0 );

        return;
    }

    /* 状態遷移パラメータ設定 */
    param.taskId  = taskId;
    param.pBuffer = pBuffer;
    param.pFdInfo = pFdInfo;

    /* 状態遷移実行 */
    retMLib = MLibStateExec( &gStateHdl,
                             EVENT_WRITE_REQ,
                             &param,
                             &prevState,
                             &nextState,
                             &errNo          );

    /* 実行結果判定 */
    if ( retMLib != MLIB_SUCCESS ) {
        /* 失敗 */

        LibMlogPut(
            "[mvfs][%s:%d] %s(): MLibStateExec() error. ret=%d, errNo=0x%X",
            __FILE__,
            __LINE__,
            __func__,
            retMLib,
            errNo
        );

        /* write応答メッセージ送信 */
        SendMsgWriteResp( taskId, MVFS_RESULT_FAILURE, 0 );

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
 * @brief       write応答メッセージ送信
 * @details     メッセージ送信先タスクID(dst)にwrite応答メッセージを送信する。
 *
 * @param[in]   dst      メッセージ送信先タスクID
 * @param[in]   result   処理結果
 *                  - MVFS_RESULT_SUCCESS 成功
 *                  - MVFS_RESULT_FAILURE 失敗
 * @param[in]   size     書込み実施サイズ
 */
/******************************************************************************/
static void SendMsgWriteResp( MkTaskId_t dst,
                              uint32_t   result,
                              size_t     size      )
{
    int32_t            ret;     /* 関数戻り値 */
    uint32_t           errNo;   /* エラー番号 */
    MvfsMsgWriteResp_t msg;     /* メッセージ */

    /* 初期化 */
    ret   = MK_MSG_RET_FAILURE;
    errNo = MK_MSG_ERR_NONE;
    memset( &msg, 0, sizeof ( MvfsMsgWriteResp_t ) );

    /* メッセージ設定 */
    msg.header.funcId = MVFS_FUNCID_WRITE;
    msg.header.type   = MVFS_TYPE_RESP;
    msg.result        = result;
    msg.size          = size;

    LibMlogPut(
        "[mvfs][%s:%d] %s() dst=%u, result=%u, size=%u.",
        __FILE__,
        __LINE__,
        __func__,
        dst,
        result,
        size
    );

    /* メッセージ送信 */
    ret = MkMsgSend( dst, &msg, sizeof ( MvfsMsgWriteResp_t ), &errNo );

    /* 送信結果判定 */
    if ( ret != MK_MSG_RET_SUCCESS ) {
        /* 失敗 */

        LibMlogPut(
            "[mvfs][%s:%d] %s() error. ret=%d, errNo=%#x",
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
 * @brief       vfsWrite要求メッセージ送信
 * @details     メッセージ送信先タスクID(dst)にvfsWrite要求メッセージを送信する。
 *
 * @param[in]   dst      メッセージ送信先タスクID
 * @param[in]   globalFd グローバルFD
 * @param[in]   writeIdx 書込みインデックス
 * @param[in]   *pBuffer 書込みデータ
 * @param[in]   size     書込みサイズ
 */
/******************************************************************************/
static void SendMsgVfsWriteReq( MkTaskId_t dst,
                                uint32_t   globalFd,
                                uint64_t   writeIdx,
                                const char *pBuffer,
                                size_t     size      )
{
    int32_t              ret;   /* 関数戻り値 */
    uint32_t             errNo; /* エラー番号 */
    MvfsMsgVfsWriteReq_t msg;   /* メッセージ */

    /* 初期化 */
    ret   = MK_MSG_RET_FAILURE;
    errNo = MK_MSG_ERR_NONE;
    memset( &msg, 0, sizeof ( MvfsMsgVfsWriteReq_t ) );

    /* メッセージ設定 */
    msg.header.funcId = MVFS_FUNCID_VFSWRITE;
    msg.header.type   = MVFS_TYPE_REQ;
    msg.globalFd      = globalFd;
    msg.writeIdx      = writeIdx;
    msg.size          = size;
    memcpy( msg.pBuffer, pBuffer, size );

    LibMlogPut(
        "[mvfs][%s:%d] %s() dst=%u, globalFd=%u, writeIdx=0x%X, size=%u",
        __FILE__,
        __LINE__,
        __func__,
        dst,
        globalFd,
        ( uint32_t ) writeIdx,
        size
    );

    /* メッセージ送信 */
    ret = MkMsgSend( dst,
                     &msg,
                     sizeof ( MvfsMsgVfsWriteReq_t ) + size,
                     &errNo                                  );

    /* 送信結果判定 */
    if ( ret != MK_MSG_RET_SUCCESS ) {
        /* 失敗 */

        LibMlogPut(
            "[mvfs][%s:%d] %s() error. ret=%d, errNo=%#x",
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
 * @brief       状態遷移タスク0101
 * @details     vfsWrite要求メッセージを送信する。
 *
 * @param[in]   *pArg 状態遷移パラメータ
 *
 * @return      遷移先状態を返す。
 * @retval      STATE_INI                初期状態
 * @retval      STATE_VFSWRITE_RESP_WAIT vfsWrite応答待ち
 */
/******************************************************************************/
static MLibState_t Task0101( void *pArg )
{
    NodeInfo_t        *pNode;   /* ノード             */
    StateTaskParam_t  *pParam;  /* 状態遷移パラメータ */
    MvfsMsgWriteReq_t *pMsg;    /* open要求メッセージ */

    /* 初期化 */
    pParam = ( StateTaskParam_t  * ) pArg;
    pNode  = pParam->pFdInfo->pNode;
    pMsg   = ( MvfsMsgWriteReq_t * ) pParam->pBuffer;

    /* vfsWrite要求メッセージ送信 */
    SendMsgVfsWriteReq( pNode->mountTaskId,
                        pMsg->globalFd,
                        pMsg->writeIdx,
                        pMsg->pBuffer,
                        pMsg->size          );

    /* [TODO]write要求元タスクID保存 */
    gWriteTaskId = pParam->taskId;

    return STATE_VFSWRITE_RESP_WAIT;
}


/******************************************************************************/
/**
 * @brief       状態遷移タスク0202
 * @details     write応答メッセージを送信する。
 *
 * @param[in]   *pArg 未使用
 *
 * @return      遷移先状態を返す。
 * @retval      STATE_INI 初期状態
 */
/******************************************************************************/
static MLibState_t Task0202( void *pArg )
{
    StateTaskParam_t      *pParam; /* 状態遷移パラメータ  */
    MvfsMsgVfsWriteResp_t *pMsg;   /* vfsWrite応答メッセージ */

    /* 初期化 */
    pParam = ( StateTaskParam_t      * ) pArg;
    pMsg   = ( MvfsMsgVfsWriteResp_t * ) pParam->pBuffer;

    /* write応答メッセージ送信 */
    SendMsgWriteResp( gWriteTaskId, pMsg->result, pMsg->size );

    return STATE_INI;
}


/******************************************************************************/

