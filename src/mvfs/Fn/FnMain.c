/******************************************************************************/
/*                                                                            */
/* src/mvfs/Fn/FnMain.c                                                       */
/*                                                                 2019/10/12 */
/* Copyright (C) 2019 Mochi.                                                  */
/*                                                                            */
/******************************************************************************/
/******************************************************************************/
/* インクルード                                                               */
/******************************************************************************/
/* 標準ヘッダ */
#include <string.h>

/* カーネルヘッダ */
#include <kernel/types.h>

/* ライブラリヘッダ */
#include <MLib/MLib.h>
#include <MLib/MLibState.h>

/* 外部モジュールヘッダ */
#include <Debug.h>
#include <Fd.h>
#include <Msg.h>


/******************************************************************************/
/* 定義                                                                       */
/******************************************************************************/
/* 状態 */
#define STATE_INIT          ( 1 )   /**< 初期状態         */
#define STATE_VFSOPEN_WAIT  ( 2 )   /**< VfsOpen待ち状態  */
#define STATE_VFSREAD_WAIT  ( 3 )   /**< VfsRead待ち状態  */
#define STATE_VFSWRITE_WAIT ( 4 )   /**< VfsWrite待ち状態 */
#define STATE_VFSCLOSE_WAIT ( 5 )   /**< VfsClose待ち状態 */

/* イベント */
#define EVENT_OPEN_REQ      ( 1 )   /**< Open要求イベント     */
#define EVENT_READ_REQ      ( 2 )   /**< Read要求イベント     */
#define EVENT_WRITE_REQ     ( 3 )   /**< Write要求イベント    */
#define EVENT_CLOSE_REQ     ( 4 )   /**< Close要求イベント    */
#define EVENT_VFSOPEN_RESP  ( 5 )   /**< VfsOpen応答イベント  */
#define EVENT_VFSREAD_RESP  ( 6 )   /**< VfsRead応答イベント  */
#define EVENT_VFSWRITE_RESP ( 7 )   /**< VfsWrite応答イベント */
#define EVENT_VFSCLOSE_RESP ( 8 )   /**< VfsClose応答イベント */


/** 状態遷移タスクパラメータ */
typedef struct {
    MkTaskId_t taskId;      /**< タスクID           */
    void       *pBuffer;    /**< メッセージバッファ */
    FdInfo_t   *pFdInfo;    /**< FD情報             */
} taskParam_t;


/******************************************************************************/
/* ローカル関数宣言                                                           */
/******************************************************************************/
/* FD割当 */
static MvfsRet_t AllocFd( MkTaskId_t taskId,
                          uint32_t   localFd,
                          const char *pPath,
                          FdInfo_t   **ppFdInfo );
/* 状態遷移タスク */
static MLibState_t Task0101( void *pArg );
static MLibState_t Task0102( void *pArg );
static MLibState_t Task0103( void *pArg );
static MLibState_t Task0104( void *pArg );
static MLibState_t Task0205( void *pArg );
static MLibState_t Task0306( void *pArg );
static MLibState_t Task0407( void *pArg );
static MLibState_t Task0508( void *pArg );


/******************************************************************************/
/* グローバル変数定義                                                         */
/******************************************************************************/
/** 状態遷移表 */
static const MLibStateTransition_t gStt[] = {
    /*--------------------+---------------------+----------+-----------------------------------------------*/
    /* 状態               | イベント            | タスク   | { 遷移先状態                                } */
    /*--------------------+---------------------+----------+-----------------------------------------------*/
    { STATE_INIT          , EVENT_OPEN_REQ      , Task0101 , { STATE_INIT          , STATE_VFSOPEN_WAIT  } },
    { STATE_INIT          , EVENT_READ_REQ      , Task0102 , { STATE_INIT          , STATE_VFSREAD_WAIT  } },
    { STATE_INIT          , EVENT_WRITE_REQ     , Task0103 , { STATE_INIT          , STATE_VFSWRITE_WAIT } },
    { STATE_INIT          , EVENT_CLOSE_REQ     , Task0104 , { STATE_INIT          , STATE_VFSCLOSE_WAIT } },
    /*--------------------+---------------------+----------+-----------------------------------------------*/
    { STATE_VFSOPEN_WAIT  , EVENT_VFSOPEN_RESP  , Task0205 , { STATE_VFSOPEN_WAIT  , STATE_INIT          } },
    { STATE_VFSREAD_WAIT  , EVENT_VFSREAD_RESP  , Task0306 , { STATE_VFSREAD_WAIT  , STATE_INIT          } },
    { STATE_VFSWRITE_WAIT , EVENT_VFSWRITE_RESP , Task0407 , { STATE_VFSWRITE_WAIT , STATE_INIT          } },
    { STATE_VFSCLOSE_WAIT , EVENT_VFSCLOSE_RESP , Task0508 , { STATE_VFSCLOSE_WAIT , STATE_INIT          } }  };
    /*--------------------+---------------------+----------+-----------------------------------------------*/


/******************************************************************************/
/* 外部モジュール向けグローバル関数定義                                       */
/******************************************************************************/
/******************************************************************************/
/**
 * @brief       Close要求メッセージ受信
 * @details     メッセージ送信元タスクID(taskId)から受信したClose要求メッセージ
 *              を処理する。
 *
 * @param[in]   taskId   メッセージ送信元タスクID
 * @param[in]   *pBuffer メッセージバッファ
 * @param[in]   size     メッセージサイズ
 */
/******************************************************************************/
void FnMainRecvCloseReq( MkTaskId_t taskId,
                         void       *pBuffer,
                         size_t     size      )
{
    FdInfo_t          *pFdInfo;     /* FD情報         */
    uint32_t          errMLib;      /* MLIBエラー要因 */
    MLibRet_t         retMLib;      /* MLIB戻り値     */
    MvfsRet_t         retMvfs;      /* 戻り値         */
    MLibState_t       prevState;    /* 遷移前状態     */
    MLibState_t       nextState;    /* 遷移後状態     */
    taskParam_t       param;        /* パラメータ     */
    MvfsMsgCloseReq_t *pMsg;        /* メッセージ     */

    DEBUG_LOG_TRC(
        "%s(): start. taskId=%#X, size=%u",
        __func__,
        taskId,
        size
    );

    /* 初期化 */
    pFdInfo   = NULL;
    errMLib   = MLIB_STATE_ERR_NONE;
    retMLib   = MLIB_FAILURE;
    retMvfs   = MVFS_RET_FAILURE;
    prevState = MLIB_STATE_NULL;
    nextState = MLIB_STATE_NULL;
    pMsg      = ( MvfsMsgCloseReq_t * ) pBuffer;
    memset( &param, 0, sizeof ( param ) );

    /* メッセージチェック */
    retMvfs = MsgCheckCloseReq( taskId, pMsg, size, &pFdInfo );

    /* チェック結果判定 */
    if ( retMvfs != MVFS_RET_SUCCESS ) {
        /* 不正メッセージ */
        return;
    }

    /* パラメータ設定 */
    param.taskId  = taskId;
    param.pBuffer = pBuffer;
    param.pFdInfo = pFdInfo;

    /* 状態遷移実行 */
    retMLib = MLibStateExec( &( pFdInfo->stateHdl ),
                             EVENT_CLOSE_REQ,
                             &param,
                             &prevState,
                             &nextState,
                             &errMLib                );

    /* 実行結果判定 */
    if ( retMLib != MLIB_SUCCESS ) {
        /* 失敗 */

        DEBUG_LOG_ERR(
            "%s(): MLibStateExec(): ret=%d, err=%#X",
            __func__,
            retMLib,
            errMLib
        );

        /* Close応答メッセージ送信 */
        MsgSendCloseResp( taskId, MVFS_RESULT_FAILURE );
    }

    DEBUG_LOG_TRC(
        "%s(): exec. state=%u->%u",
        __func__,
        prevState,
        nextState
    );

    return;
}


/******************************************************************************/
/**
 * @brief       Open要求メッセージ受信
 * @details     メッセージ送信元タスクID(taskId)から受信したOpen要求メッセージ
 *              を処理する。
 *
 * @param[in]   taskId   メッセージ送信元タスクID
 * @param[in]   *pBuffer メッセージバッファ
 * @param[in]   size     メッセージサイズ
 */
/******************************************************************************/
void FnMainRecvOpenReq( MkTaskId_t taskId,
                        void       *pBuffer,
                        size_t     size      )
{
    FdInfo_t         *pFdInfo;  /* FD情報         */
    uint32_t         errMLib;   /* MLIBエラー要因 */
    MLibRet_t        retMLib;   /* MLIB戻り値     */
    MvfsRet_t        retMvfs;   /* 戻り値         */
    MLibState_t      prevState; /* 遷移前状態     */
    MLibState_t      nextState; /* 遷移後状態     */
    taskParam_t      param;     /* パラメータ     */
    MvfsMsgOpenReq_t *pMsg;     /* メッセージ     */

    DEBUG_LOG_TRC(
        "%s(): start. taskId=%#X, size=%u",
        __func__,
        taskId,
        size
    );

    /* 初期化 */
    pFdInfo   = NULL;
    errMLib   = MLIB_STATE_ERR_NONE;
    retMLib   = MLIB_FAILURE;
    retMvfs   = MVFS_RET_FAILURE;
    prevState = MLIB_STATE_NULL;
    nextState = MLIB_STATE_NULL;
    pMsg      = ( MvfsMsgOpenReq_t * ) pBuffer;
    memset( &param, 0, sizeof ( param ) );

    /* メッセージチェック */
    retMvfs = MsgCheckOpenReq( pMsg, size );

    /* チェック結果判定 */
    if ( retMvfs != MVFS_RET_SUCCESS ) {
        /* 不正メッセージ */

        /* Open応答メッセージ送信 */
        MsgSendOpenResp( taskId, MVFS_RESULT_FAILURE, MVFS_FD_NULL );

        return;
    }

    /* FD割当 */
    retMvfs = AllocFd( taskId, pMsg->localFd, pMsg->path, &pFdInfo );

    /* 割当結果判定 */
    if ( retMvfs != MVFS_RET_SUCCESS ) {
        /* 失敗 */

        /* Open応答メッセージ送信 */
        MsgSendOpenResp( taskId, MVFS_RESULT_FAILURE, MVFS_FD_NULL );

        return;
    }

    /* パラメータ設定 */
    param.taskId  = taskId;
    param.pBuffer = pBuffer;
    param.pFdInfo = pFdInfo;

    /* 状態遷移実行 */
    retMLib = MLibStateExec( &( pFdInfo->stateHdl ),
                             EVENT_OPEN_REQ,
                             &param,
                             &prevState,
                             &nextState,
                             &errMLib                );

    /* 実行結果判定 */
    if ( retMLib != MLIB_SUCCESS ) {
        /* 失敗 */

        DEBUG_LOG_ERR(
            "%s(): MLibStateExec(): ret=%d, err=%#X",
            __func__,
            retMLib,
            errMLib
        );

        /* Open応答メッセージ送信 */
        MsgSendOpenResp( taskId, MVFS_RESULT_FAILURE, MVFS_FD_NULL );

        return;
    }

    DEBUG_LOG_TRC(
        "%s(): exec. state=%u->%u",
        __func__,
        prevState,
        nextState
    );

    return;
}


/******************************************************************************/
/**
 * @brief       Read要求メッセージ受信
 * @details     メッセージ送信元タスクID(taskId)から受信したRead要求メッセージ
 *              を処理する。
 *
 * @param[in]   taskId   メッセージ送信元タスクID
 * @param[in]   *pBuffer メッセージバッファ
 * @param[in]   size     メッセージサイズ
 */
/******************************************************************************/
void FnMainRecvReadReq( MkTaskId_t taskId,
                        void       *pBuffer,
                        size_t     size      )
{
    FdInfo_t         *pFdInfo;  /* FD情報         */
    uint32_t         errMLib;   /* MLIBエラー要因 */
    MLibRet_t        retMLib;   /* MLIB戻り値     */
    MvfsRet_t        retMvfs;   /* 戻り値         */
    MLibState_t      prevState; /* 遷移前状態     */
    MLibState_t      nextState; /* 遷移後状態     */
    taskParam_t      param;     /* パラメータ     */
    MvfsMsgReadReq_t *pMsg;     /* メッセージ     */

    DEBUG_LOG_TRC(
        "%s(): start. taskId=%#X, size=%u",
        __func__,
        taskId,
        size
    );

    /* 初期化 */
    pFdInfo   = NULL;
    errMLib   = MLIB_STATE_ERR_NONE;
    retMLib   = MLIB_FAILURE;
    retMvfs   = MVFS_RET_FAILURE;
    prevState = MLIB_STATE_NULL;
    nextState = MLIB_STATE_NULL;
    pMsg      = ( MvfsMsgReadReq_t * ) pBuffer;
    memset( &param, 0, sizeof ( param ) );

    /* メッセージチェック */
    retMvfs = MsgCheckReadReq( taskId, pMsg, size, &pFdInfo );

    /* チェック結果判定 */
    if ( retMvfs != MVFS_RET_SUCCESS ) {
        /* 不正メッセージ */
        return;
    }

    /* パラメータ設定 */
    param.taskId  = taskId;
    param.pBuffer = pBuffer;
    param.pFdInfo = pFdInfo;

    /* 状態遷移実行 */
    retMLib = MLibStateExec( &( pFdInfo->stateHdl ),
                             EVENT_READ_REQ,
                             &param,
                             &prevState,
                             &nextState,
                             &errMLib                );

    /* 実行結果判定 */
    if ( retMLib != MLIB_SUCCESS ) {
        /* 失敗 */

        DEBUG_LOG_ERR(
            "%s(): MLibStateExec(): ret=%d, err=%#X",
            __func__,
            retMLib,
            errMLib
        );

        /* Read応答メッセージ送信 */
        MsgSendReadResp( taskId, MVFS_RESULT_FAILURE, NULL, 0 );

        return;
    }

    DEBUG_LOG_TRC(
        "%s(): exec. state=%u->%u",
        __func__,
        prevState,
        nextState
    );

    return;
}


/******************************************************************************/
/**
 * @brief       VfsClose応答メッセージ受信
 * @details     メッセージ送信元タスクID(taskId)から受信したVfsClose応答メッ
 *              セージを処理する。
 *
 * @param[in]   taskId   メッセージ送信元タスクID
 * @param[in]   *pBuffer メッセージバッファ
 * @param[in]   size     メッセージサイズ
 */
/******************************************************************************/
void FnMainRecvVfsCloseResp( MkTaskId_t taskId,
                             void       *pBuffer,
                             size_t     size      )
{
    FdInfo_t              *pFdInfo;     /* FD情報         */
    uint32_t              errMLib;      /* MLIBエラー要因 */
    MLibRet_t             retMLib;      /* MLIB戻り値     */
    MvfsRet_t             retMvfs;      /* 戻り値         */
    MLibState_t           prevState;    /* 遷移前状態     */
    MLibState_t           nextState;    /* 遷移後状態     */
    taskParam_t           param;        /* パラメータ     */
    MvfsMsgVfsCloseResp_t *pMsg;        /* メッセージ     */

    DEBUG_LOG_TRC(
        "%s(): start. taskId=%#X, size=%u",
        __func__,
        taskId,
        size
    );

    /* 初期化 */
    pFdInfo   = NULL;
    errMLib   = MLIB_STATE_ERR_NONE;
    retMLib   = MLIB_FAILURE;
    retMvfs   = MVFS_RET_FAILURE;
    prevState = MLIB_STATE_NULL;
    nextState = MLIB_STATE_NULL;
    pMsg      = ( MvfsMsgVfsCloseResp_t * ) pBuffer;
    memset( &param, 0, sizeof ( param ) );

    /* メッセージチェック */
    retMvfs = MsgCheckVfsCloseResp( taskId, pMsg, size, &pFdInfo );

    /* チェック結果判定 */
    if ( retMvfs != MVFS_RET_SUCCESS ) {
        /* 不正メッセージ */
        return;
    }

    /* パラメータ設定 */
    param.taskId  = taskId;
    param.pBuffer = pBuffer;
    param.pFdInfo = pFdInfo;

    /* 状態遷移実行 */
    retMLib = MLibStateExec( &( pFdInfo->stateHdl ),
                             EVENT_VFSCLOSE_RESP,
                             &param,
                             &prevState,
                             &nextState,
                             &errMLib                );

    /* 実行結果判定 */
    if ( retMLib != MLIB_SUCCESS ) {
        /* 失敗 */

        DEBUG_LOG_ERR(
            "%s(): MLibStateExec(): ret=%d, err=%#X",
            __func__,
            retMLib,
            errMLib
        );
        return;
    }

    DEBUG_LOG_TRC(
        "%s(): exec. state=%u->%u",
        __func__,
        prevState,
        nextState
    );

    return;
}


/******************************************************************************/
/**
 * @brief       VfsOpen応答メッセージ受信
 * @details     メッセージ送信元タスクID(taskId)から受信したVfsOpen応答メッセー
 *              ジを処理する。
 *
 * @param[in]   taskId   メッセージ送信元タスクID
 * @param[in]   *pBuffer メッセージバッファ
 * @param[in]   size     メッセージサイズ
 */
/******************************************************************************/
void FnMainRecvVfsOpenResp( MkTaskId_t taskId,
                            void       *pBuffer,
                            size_t     size      )
{
    FdInfo_t             *pFdInfo;  /* FD情報         */
    uint32_t             errMLib;   /* MLIBエラー要因 */
    MLibRet_t            retMLib;   /* MLIB戻り値     */
    MvfsRet_t            retMvfs;   /* 戻り値         */
    MLibState_t          prevState; /* 遷移前状態     */
    MLibState_t          nextState; /* 遷移後状態     */
    taskParam_t          param;     /* パラメータ     */
    MvfsMsgVfsOpenResp_t *pMsg;     /* メッセージ     */

    DEBUG_LOG_TRC(
        "%s(): start. taskId=%#X, size=%u",
        __func__,
        taskId,
        size
    );

    /* 初期化 */
    pFdInfo   = NULL;
    errMLib   = MLIB_STATE_ERR_NONE;
    retMLib   = MLIB_FAILURE;
    retMvfs   = MVFS_RET_FAILURE;
    prevState = MLIB_STATE_NULL;
    nextState = MLIB_STATE_NULL;
    pMsg      = ( MvfsMsgVfsOpenResp_t * ) pBuffer;
    memset( &param, 0, sizeof ( param ) );

    /* メッセージチェック */
    retMvfs = MsgCheckVfsOpenResp( taskId, pMsg, size, &pFdInfo );

    /* チェック結果判定 */
    if ( retMvfs != MVFS_RET_SUCCESS ) {
        /* 不正メッセージ */
        return;
    }

    /* パラメータ設定 */
    param.taskId  = taskId;
    param.pBuffer = pBuffer;
    param.pFdInfo = pFdInfo;

    /* 状態遷移実行 */
    retMLib = MLibStateExec( &( pFdInfo->stateHdl ),
                             EVENT_VFSOPEN_RESP,
                             &param,
                             &prevState,
                             &nextState,
                             &errMLib                );

    /* 実行結果判定 */
    if ( retMLib != MLIB_SUCCESS ) {
        /* 失敗 */

        DEBUG_LOG_ERR(
            "%s(): MLibStateExec(): ret=%d, err=%#X",
            __func__,
            retMLib,
            errMLib
        );
        return;
    }

    DEBUG_LOG_TRC(
        "%s(): exec. state=%u->%u",
        __func__,
        prevState,
        nextState
    );

    return;
}


/******************************************************************************/
/**
 * @brief       VfsRead応答メッセージ受信
 * @details     メッセージ送信元タスクID(taskId)から受信したVfsRead応答メッセー
 *              ジを処理する。
 *
 * @param[in]   taskId   メッセージ送信元タスクID
 * @param[in]   *pBuffer メッセージバッファ
 * @param[in]   size     メッセージサイズ
 */
/******************************************************************************/
void FnMainRecvVfsReadResp( MkTaskId_t taskId,
                            void       *pBuffer,
                            size_t     size      )
{
    FdInfo_t             *pFdInfo;  /* FD情報         */
    uint32_t             errMLib;   /* MLIBエラー要因 */
    MLibRet_t            retMLib;   /* MLIB戻り値     */
    MvfsRet_t            retMvfs;   /* 戻り値         */
    MLibState_t          prevState; /* 遷移前状態     */
    MLibState_t          nextState; /* 遷移後状態     */
    taskParam_t          param;     /* パラメータ     */
    MvfsMsgVfsReadResp_t *pMsg;     /* メッセージ     */

    DEBUG_LOG_TRC(
        "%s(): start. taskId=%#X, size=%u",
        __func__,
        taskId,
        size
    );

    /* 初期化 */
    pFdInfo   = NULL;
    errMLib   = MLIB_STATE_ERR_NONE;
    retMLib   = MLIB_FAILURE;
    retMvfs   = MVFS_RET_FAILURE;
    prevState = MLIB_STATE_NULL;
    nextState = MLIB_STATE_NULL;
    pMsg      = ( MvfsMsgVfsReadResp_t * ) pBuffer;
    memset( &param, 0, sizeof ( param ) );

    /* メッセージチェック */
    retMvfs = MsgCheckVfsReadResp( taskId, pMsg, size, &pFdInfo );

    /* チェック結果判定 */
    if ( retMvfs != MVFS_RET_SUCCESS ) {
        /* 不正メッセージ */
        return;
    }

    /* パラメータ設定 */
    param.taskId  = taskId;
    param.pBuffer = pBuffer;
    param.pFdInfo = pFdInfo;

    /* 状態遷移実行 */
    retMLib = MLibStateExec( &( pFdInfo->stateHdl ),
                             EVENT_VFSREAD_RESP,
                             &param,
                             &prevState,
                             &nextState,
                             &errMLib                );

    /* 実行結果判定 */
    if ( retMLib != MLIB_SUCCESS ) {
        /* 失敗 */

        DEBUG_LOG_ERR(
            "%s(): MLibStateExec(): ret=%d, err=%#X",
            __func__,
            retMLib,
            errMLib
        );
        return;
    }

    DEBUG_LOG_TRC(
        "%s(): exec. state=%u->%u",
        __func__,
        prevState,
        nextState
    );

    return;
}


/******************************************************************************/
/**
 * @brief       VfsWrite応答メッセージ受信
 * @details     メッセージ送信元タスクID(taskId)から受信したVfsWrite応答メッ
 *              セージを処理する。
 *
 * @param[in]   taskId   メッセージ送信元タスクID
 * @param[in]   *pBuffer メッセージバッファ
 * @param[in]   size     メッセージサイズ
 */
/******************************************************************************/
void FnMainRecvVfsWriteResp( MkTaskId_t taskId,
                             void       *pBuffer,
                             size_t     size      )
{
    FdInfo_t              *pFdInfo;  /* FD情報         */
    uint32_t              errMLib;   /* MLIBエラー要因 */
    MLibRet_t             retMLib;   /* MLIB戻り値     */
    MvfsRet_t             retMvfs;   /* 戻り値         */
    MLibState_t           prevState; /* 遷移前状態     */
    MLibState_t           nextState; /* 遷移後状態     */
    taskParam_t           param;     /* パラメータ     */
    MvfsMsgVfsWriteResp_t *pMsg;     /* メッセージ     */

    DEBUG_LOG_TRC(
        "%s(): start. taskId=%#X, size=%u",
        __func__,
        taskId,
        size
    );

    /* 初期化 */
    pFdInfo   = NULL;
    errMLib   = MLIB_STATE_ERR_NONE;
    retMLib   = MLIB_FAILURE;
    retMvfs   = MVFS_RET_FAILURE;
    prevState = MLIB_STATE_NULL;
    nextState = MLIB_STATE_NULL;
    pMsg      = ( MvfsMsgVfsWriteResp_t * ) pBuffer;
    memset( &param, 0, sizeof ( param ) );

    /* メッセージチェック */
    retMvfs = MsgCheckVfsWriteResp( taskId, pMsg, size, &pFdInfo );

    /* チェック結果判定 */
    if ( retMvfs != MVFS_RET_SUCCESS ) {
        /* 不正メッセージ */
        return;
    }

    /* パラメータ設定 */
    param.taskId  = taskId;
    param.pBuffer = pBuffer;
    param.pFdInfo = pFdInfo;

    /* 状態遷移実行 */
    retMLib = MLibStateExec( &( pFdInfo->stateHdl ),
                             EVENT_VFSWRITE_RESP,
                             &param,
                             &prevState,
                             &nextState,
                             &errMLib                );

    /* 実行結果判定 */
    if ( retMLib != MLIB_SUCCESS ) {
        /* 失敗 */

        DEBUG_LOG_ERR(
            "%s(): MLibStateExec(): ret=%d, err=%#X",
            __func__,
            retMLib,
            errMLib
        );
        return;
    }

    DEBUG_LOG_TRC(
        "%s(): exec. state=%u->%u",
        __func__,
        prevState,
        nextState
    );

    return;
}


/******************************************************************************/
/**
 * @brief       Write要求メッセージ受信
 * @details     メッセージ送信元タスクID(taskId)から受信したWrite要求メッセージ
 *              を処理する。
 *
 * @param[in]   taskId   メッセージ送信元タスクID
 * @param[in]   *pBuffer メッセージバッファ
 * @param[in]   size     メッセージサイズ
 */
/******************************************************************************/
void FnMainRecvWriteReq( MkTaskId_t taskId,
                         void       *pBuffer,
                         size_t     size      )
{
    FdInfo_t          *pFdInfo;     /* FD情報         */
    uint32_t          errMLib;      /* MLIBエラー要因 */
    MLibRet_t         retMLib;      /* MLIB戻り値     */
    MvfsRet_t         retMvfs;      /* 戻り値         */
    MLibState_t       prevState;    /* 遷移前状態     */
    MLibState_t       nextState;    /* 遷移後状態     */
    taskParam_t       param;        /* パラメータ     */
    MvfsMsgWriteReq_t *pMsg;        /* メッセージ     */

    DEBUG_LOG_TRC(
        "%s(): start. taskId=%#X, size=%u",
        __func__,
        taskId,
        size
    );

    /* 初期化 */
    pFdInfo   = NULL;
    errMLib   = MLIB_STATE_ERR_NONE;
    retMLib   = MLIB_FAILURE;
    retMvfs   = MVFS_RET_FAILURE;
    prevState = MLIB_STATE_NULL;
    nextState = MLIB_STATE_NULL;
    pMsg      = ( MvfsMsgWriteReq_t * ) pBuffer;
    memset( &param, 0, sizeof ( param ) );

    /* メッセージチェック */
    retMvfs = MsgCheckWriteReq( taskId, pMsg, size, &pFdInfo );

    /* チェック結果判定 */
    if ( retMvfs != MVFS_RET_SUCCESS ) {
        /* 不正メッセージ */
        return;
    }

    /* パラメータ設定 */
    param.taskId  = taskId;
    param.pBuffer = pBuffer;
    param.pFdInfo = pFdInfo;

    /* 状態遷移実行 */
    retMLib = MLibStateExec( &( pFdInfo->stateHdl ),
                             EVENT_WRITE_REQ,
                             &param,
                             &prevState,
                             &nextState,
                             &errMLib                );

    /* 実行結果判定 */
    if ( retMLib != MLIB_SUCCESS ) {
        /* 失敗 */

        DEBUG_LOG_ERR(
            "%s(): MLibStateExec(): ret=%d, err=%#X",
            __func__,
            retMLib,
            errMLib
        );

        /* Write応答メッセージ送信 */
        MsgSendWriteResp( taskId, MVFS_RESULT_FAILURE, 0 );

        return;
    }

    DEBUG_LOG_TRC(
        "%s(): exec. state=%u->%u",
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
 * @brief       グローバルFD割当
 * @details     グローバルFDを新しく割り当てる。
 *
 * @param[in]   taskId     要求元タスクID
 * @param[in]   localFd    ローカルファイルディスクリプタ
 * @param[in]   *pPath     パス
 * @param[out]  *pGlobalFd グローバルファイルディスクリプタ
 *
 * @return      割当結果を返す。
 * @retval      MVFS_RET_SUCCESS 成功
 * @retval      MVFS_RET_FAILURE 失敗
 */
/******************************************************************************/
static MvfsRet_t AllocFd( MkTaskId_t taskId,
                          uint32_t   localFd,
                          const char *pPath,
                          FdInfo_t   **ppFdInfo )
{
    NodeInfo_t *pNode;  /* ノード情報 */

    /* 初期化 */
    pNode = NULL;

    /* パスからノード取得 */
    pNode = NodeGet( pPath );

    /* 取得結果判定 */
    if ( pNode == NULL ) {
        /* ノード無し */

        DEBUG_LOG_ERR(
            "%s(): NodeGet(): path=%s",
            __func__,
            pPath
        );
        return MVFS_RET_FAILURE;
    }

    /* ノードタイプチェック */
    if ( pNode->type != NODE_TYPE_MOUNT_FILE ) {
        /* マウントファイルでない */

        DEBUG_LOG_ERR(
            "%s(): invalid type( %#X ). path=%s",
            __func__,
            pNode->type,
            pPath
        );
        return MVFS_RET_FAILURE;
    }

    /* PID毎のFD払い出し */
    *ppFdInfo = FdAlloc( localFd, pNode );

    /* 払出結果判定 */
    if ( *ppFdInfo == NULL ) {
        /* 失敗 */

        DEBUG_LOG_ERR(
            "%s(): FdAlloc(): path=%s",
            __func__,
            pPath
        );
        return MVFS_RET_FAILURE;
    }

    /* 状態遷移初期化 */
    MLibStateInit(
        &( ( *ppFdInfo )->stateHdl ),
        gStt,
        sizeof ( gStt ),
        STATE_INIT,
        NULL
    );

    return MVFS_RET_SUCCESS;
}


/******************************************************************************/
/**
 * @brief       状態遷移タスク0101
 * @details     VfsOpen要求メッセージをMountプロセスに送信する。
 *
 * @param[in]   *pArg パラメータ
 *
 * @return      遷移先状態を返す。
 * @retval      STATE_INIT         初期状態
 * @retval      STATE_VFSOPEN_WAIT VfsOpen待ち状態
 */
/******************************************************************************/
static MLibState_t Task0101( void *pArg )
{
    FdInfo_t         *pFdInfo;  /* FD情報     */
    taskParam_t      *pParam;   /* パラメータ */
    MvfsMsgOpenReq_t *pMsg;     /* メッセージ */

    /* 初期化 */
    pParam  = ( taskParam_t      * ) pArg;
    pMsg    = ( MvfsMsgOpenReq_t * ) pParam->pBuffer;
    pFdInfo = pParam->pFdInfo;

    /* 要求元タスクID登録 */
    pFdInfo->taskId = pParam->taskId;

    /* vfsOpen要求メッセージ送信 */
    MsgSendVfsOpenReq( pFdInfo->pNode->mountTaskId,
                       MK_TASKID_TO_PID( pParam->taskId ),
                       pFdInfo->globalFd,
                       pMsg->path                          );

    return STATE_VFSOPEN_WAIT;
}


/******************************************************************************/
/**
 * @brief       状態遷移タスク0102
 * @details     VfsRead要求メッセージをMountプロセスに送信する。
 *
 * @param[in]   *pArg パラメータ
 *
 * @return      遷移先状態を返す。
 * @retval      STATE_INIT         初期状態
 * @retval      STATE_VFSREAD_WAIT VfsRead待ち状態
 */
/******************************************************************************/
static MLibState_t Task0102( void *pArg )
{
    FdInfo_t         *pFdInfo;  /* FD情報     */
    taskParam_t      *pParam;   /* パラメータ */
    MvfsMsgReadReq_t *pMsg;     /* メッセージ */

    /* 初期化 */
    pParam  = ( taskParam_t      * ) pArg;
    pMsg    = ( MvfsMsgReadReq_t * ) pParam->pBuffer;
    pFdInfo = pParam->pFdInfo;

    /* 要求元タスクID登録 */
    pFdInfo->taskId = pParam->taskId;

    /* VfsRead要求メッセージ送信 */
    MsgSendVfsReadReq( pFdInfo->pNode->mountTaskId,
                       pMsg->globalFd,
                       pMsg->readIdx,
                       pMsg->size                   );

    return STATE_VFSREAD_WAIT;
}


/******************************************************************************/
/**
 * @brief       状態遷移タスク0103
 * @details     VfsWrite要求メッセージをMountプロセスに送信する。
 *
 * @param[in]   *pArg パラメータ
 *
 * @return      遷移先状態を返す。
 * @retval      STATE_INIT          初期状態
 * @retval      STATE_VFSWRITE_WAIT VfsWrite待ち状態
 */
/******************************************************************************/
static MLibState_t Task0103( void *pArg )
{
    FdInfo_t          *pFdInfo; /* FD情報     */
    taskParam_t       *pParam;  /* パラメータ */
    MvfsMsgWriteReq_t *pMsg;    /* メッセージ */

    /* 初期化 */
    pParam  = ( taskParam_t       * ) pArg;
    pMsg    = ( MvfsMsgWriteReq_t * ) pParam->pBuffer;
    pFdInfo = pParam->pFdInfo;

    /* 要求元タスクID登録 */
    pFdInfo->taskId = pParam->taskId;

    /* VfsWrite要求メッセージ送信 */
    MsgSendVfsWriteReq( pFdInfo->pNode->mountTaskId,
                        pMsg->globalFd,
                        pMsg->writeIdx,
                        pMsg->pBuffer,
                        pMsg->size                   );

    return STATE_VFSWRITE_WAIT;
}


/******************************************************************************/
/**
 * @brief       状態遷移タスク0104
 * @details     VfsClose要求メッセージをMountプロセスに送信する。
 *
 * @param[in]   *pArg パラメータ
 *
 * @return      遷移先状態を返す。
 * @retval      STATE_INIT          初期状態
 * @retval      STATE_VFSCLOSE_WAIT VfsClose待ち状態
 */
/******************************************************************************/
static MLibState_t Task0104( void *pArg )
{
    FdInfo_t          *pFdInfo; /* FD情報     */
    taskParam_t       *pParam;  /* パラメータ */
    MvfsMsgCloseReq_t *pMsg;    /* メッセージ */

    /* 初期化 */
    pParam  = ( taskParam_t       * ) pArg;
    pMsg    = ( MvfsMsgCloseReq_t * ) pParam->pBuffer;
    pFdInfo = pParam->pFdInfo;

    /* 要求元タスクID登録 */
    pFdInfo->taskId = pParam->taskId;

    /* VfsClose要求メッセージ送信 */
    MsgSendVfsCloseReq( pFdInfo->pNode->mountTaskId,
                        pFdInfo->globalFd            );

    return STATE_VFSCLOSE_WAIT;
}


/******************************************************************************/
/**
 * @brief       状態遷移タスク0205
 * @details     Open応答メッセージを要求元プロセスに送信する。
 *
 * @param[in]   *pArg パラメータ
 *
 * @return      遷移先状態を返す。
 * @retval      STATE_INIT         初期状態
 * @retval      STATE_VFSOPEN_WAIT VfsOpen待ち状態
 */
/******************************************************************************/
static MLibState_t Task0205( void *pArg )
{
    FdInfo_t             *pFdInfo;  /* FD情報     */
    taskParam_t          *pParam;   /* パラメータ */
    MvfsMsgVfsOpenResp_t *pMsg;     /* メッセージ */

    /* 初期化 */
    pParam  = ( taskParam_t          * ) pArg;
    pMsg    = ( MvfsMsgVfsOpenResp_t * ) pParam->pBuffer;
    pFdInfo = pParam->pFdInfo;

    /* Open応答メッセージ送信 */
    MsgSendOpenResp( pFdInfo->taskId,
                     MVFS_RESULT_SUCCESS,
                     pFdInfo->globalFd    );

    return STATE_INIT;
}


/******************************************************************************/
/**
 * @brief       状態遷移タスク0306
 * @details     Read応答メッセージを要求元プロセスに送信する。
 *
 * @param[in]   *pArg パラメータ
 *
 * @return      遷移先状態を返す。
 * @retval      STATE_INIT         初期状態
 * @retval      STATE_VFSREAD_WAIT VfsRead待ち状態
 */
/******************************************************************************/
static MLibState_t Task0306( void *pArg )
{
    FdInfo_t             *pFdInfo;  /* FD情報     */
    taskParam_t          *pParam;   /* パラメータ */
    MvfsMsgVfsReadResp_t *pMsg;     /* メッセージ */

    /* 初期化 */
    pParam  = ( taskParam_t          * ) pArg;
    pMsg    = ( MvfsMsgVfsReadResp_t * ) pParam->pBuffer;
    pFdInfo = pParam->pFdInfo;

    /* Ready状態設定 */
    pFdInfo->pNode->ready &= ~MVFS_READY_READ;
    pFdInfo->pNode->ready |= pMsg->ready;

    /* Read応答メッセージ送信 */
    MsgSendReadResp( pFdInfo->taskId,
                     pMsg->result,
                     pMsg->pBuffer,
                     pMsg->size       );

    return STATE_INIT;
}


/******************************************************************************/
/**
 * @brief       状態遷移タスク0407
 * @details     Write応答メッセージを要求元プロセスに送信する。
 *
 * @param[in]   *pArg パラメータ
 *
 * @return      遷移先状態を返す。
 * @retval      STATE_INIT          初期状態
 * @retval      STATE_VFSWRITE_WAIT VfsWrite待ち状態
 */
/******************************************************************************/
static MLibState_t Task0407( void *pArg )
{
    FdInfo_t              *pFdInfo; /* FD情報     */
    taskParam_t           *pParam;  /* パラメータ */
    MvfsMsgVfsWriteResp_t *pMsg;    /* メッセージ */

    /* 初期化 */
    pParam  = ( taskParam_t           * ) pArg;
    pMsg    = ( MvfsMsgVfsWriteResp_t * ) pParam->pBuffer;
    pFdInfo = pParam->pFdInfo;

    /* ready状態設定 */
    pFdInfo->pNode->ready &= ~MVFS_READY_READ;
    pFdInfo->pNode->ready |= pMsg->ready;

    /* Write応答メッセージ送信 */
    MsgSendWriteResp( pFdInfo->taskId, pMsg->result, pMsg->size );

    return STATE_INIT;
}


/******************************************************************************/
/**
 * @brief       状態遷移タスク0508
 * @details     Close応答メッセージを要求元プロセスに送信する。
 *
 * @param[in]   *pArg パラメータ
 *
 * @return      遷移先状態を返す。
 * @retval      STATE_INIT          初期状態
 * @retval      STATE_VFSCLOSE_WAIT VfsClose待ち状態
 */
/******************************************************************************/
static MLibState_t Task0508( void *pArg )
{
    FdInfo_t              *pFdInfo; /* FD情報     */
    taskParam_t           *pParam;  /* パラメータ */
    MvfsMsgVfsCloseResp_t *pMsg;    /* メッセージ */

    /* 初期化 */
    pParam  = ( taskParam_t           * ) pArg;
    pMsg    = ( MvfsMsgVfsCloseResp_t * ) pParam->pBuffer;
    pFdInfo = pParam->pFdInfo;

    /* Close応答メッセージ送信 */
    MsgSendCloseResp( pFdInfo->taskId,
                      pMsg->result     );

    return STATE_INIT;
}


/******************************************************************************/
