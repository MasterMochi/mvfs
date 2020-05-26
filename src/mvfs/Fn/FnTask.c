/******************************************************************************/
/*                                                                            */
/* src/mvfs/Fn/FnTask.c                                                       */
/*                                                                 2020/05/25 */
/* Copyright (C) 2019 Mochi.                                                  */
/*                                                                            */
/******************************************************************************/
/******************************************************************************/
/* インクルード                                                               */
/******************************************************************************/
/* 標準ヘッダ */
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

/* カーネルヘッダ */
#include <kernel/types.h>

/* ライブラリヘッダ */
#include <MLib/MLib.h>
#include <MLib/MLibDynamicArray.h>
#include <MLib/MLibState.h>

/* 外部モジュールヘッダ */
#include <mvfs.h>
#include <Debug.h>
#include <Fd.h>
#include <Msg.h>
#include <Node.h>


/******************************************************************************/
/* 定義                                                                       */
/******************************************************************************/
/** タスク単位情報管理テーブルチャンクサイズ */
#define MNGTBL_CHUNK_SIZE ( 256 )

/* 状態 */
#define STATE_INIT          ( 1 )   /**< 初期状態     */
#define STATE_VFSREADY_WAIT ( 2 )   /**< Select中状態 */

/* イベント */
#define EVENT_MOUNT_REQ    ( 1 )    /**< Mount要求イベント    */
#define EVENT_SELECT_REQ   ( 2 )    /**< Select要求イベント   */
#define EVENT_VFSREADY_NTC ( 3 )    /**< VfsReady通知イベント */

/** タスク単位情報 */
typedef struct {
    uint_t            idx;              /**< インデックス               */
    MkTaskId_t        taskId;           /**< タスクID                   */
    MLibStateHandle_t stateHdl;         /**< 状態遷移ハンドル           */
    uint32_t          *pReadFdList;     /**< 読込監視グローバルFDリスト */
    uint32_t          *pWriteFdList;    /**< 書込監視グローバルFDリスト */
    size_t            readFdNum;        /**< 読込監視グローバルFD数     */
    size_t            writeFdNum;       /**< 書込監視グローバルFD数     */
} taskInfo_t;

/** 状態遷移タスクパラメータ */
typedef struct {
    MkTaskId_t taskId;      /**< タスクID           */
    void       *pBuffer;    /**< メッセージバッファ */
    taskInfo_t *pTaskInfo;  /**< タスク単位情報     */
    NodeInfo_t *pNode;      /**< ノード情報         */
} taskParam_t;


/******************************************************************************/
/* ローカル関数宣言                                                           */
/******************************************************************************/
/* タスクID比較 */
static bool CompareTaskId( uint32_t idx,
                           void     *pEntry,
                           va_list  vaList   );

/* 状態遷移タスク */
static MLibState_t DoTask0101( void *pArg );
static MLibState_t DoTask0102( void *pArg );
static MLibState_t DoTask0202( void *pArg );
static MLibState_t DoTask0203( void *pArg );

/* VfsReady通知イベント実行 */
static void ExecEventVfsReadyNtc( uint_t  idx,
                                  void    *pEntry,
                                  va_list vaList   );
/* タスク単位情報初期化 */
static void FreeTaskInfo( taskInfo_t *pTaskInfo );
/* タスク単位情報取得 */
static taskInfo_t *GetTaskInfo( MkTaskId_t taskId );


/******************************************************************************/
/* グローバル変数定義                                                         */
/******************************************************************************/
/** 状態遷移表 */
static const MLibStateTransition_t gStt[] = {
    /*--------------------+--------------------+------------+------------------------------------------*/
    /* 状態               | イベント           | タスク     | { 遷移先状態                           } */
    /*--------------------+--------------------+------------+------------------------------------------*/
    { STATE_INIT          , EVENT_MOUNT_REQ    , DoTask0101 , { STATE_INIT                       , 0 } },
    { STATE_INIT          , EVENT_SELECT_REQ   , DoTask0102 , { STATE_INIT , STATE_VFSREADY_WAIT , 0 } },
    { STATE_INIT          , EVENT_VFSREADY_NTC , NULL       , { STATE_INIT                       , 0 } },
    { STATE_VFSREADY_WAIT , EVENT_SELECT_REQ   , DoTask0202 , { STATE_INIT , STATE_VFSREADY_WAIT , 0 } },
    { STATE_VFSREADY_WAIT , EVENT_VFSREADY_NTC , DoTask0203 , { STATE_INIT , STATE_VFSREADY_WAIT , 0 } }  };
    /*--------------------+--------------------+------------+------------------------------------------*/

/** タスク単位管理テーブル */
static MLibDynamicArray_t *pgMngTbl;


/******************************************************************************/
/* 外部モジュール向けグローバル関数定義                                       */
/******************************************************************************/
/******************************************************************************/
/**
 * @brief       Mount要求メッセージ受信
 * @details     メッセージ送信元タスクID(taskId)から受信したMount要求メッセージ
 *              を処理する。
 *
 * @param[in]   taskId   メッセージ送信元タスクID
 * @param[in]   *pBuffer メッセージバッファ
 * @param[in]   size     メッセージサイズ
 */
/******************************************************************************/
void FnTaskRecvMountReq( MkTaskId_t taskId,
                         void       *pBuffer,
                         size_t     size      )
{
    uint32_t          errMLib;      /* MLIBエラー要因 */
    MLibRet_t         retMLib;      /* MLIB戻り値     */
    MvfsRet_t         retMvfs;      /* 戻り値         */
    taskInfo_t        *pTaskInfo;   /* タスク単位情報 */
    MLibState_t       prevState;    /* 遷移前状態     */
    MLibState_t       nextState;    /* 遷移後状態     */
    taskParam_t       param;        /* パラメータ     */
    MvfsMsgMountReq_t *pMsg;        /* メッセージ     */

    /* 初期化 */
    errMLib   = MLIB_STATE_ERR_NONE;
    retMLib   = MLIB_FAILURE;
    retMvfs   = MVFS_RET_FAILURE;
    pTaskInfo = NULL;
    prevState = MLIB_STATE_NULL;
    nextState = MLIB_STATE_NULL;
    pMsg      = ( MvfsMsgMountReq_t * ) pBuffer;
    memset( &param, 0, sizeof ( param ) );

    DEBUG_LOG_TRC(
        "%s(): taskId=%#X, size=%u",
        __func__,
        taskId,
        size
    );

    /* メッセージチェック */
    retMvfs = MsgCheckMountReq( pMsg, size );

    /* チェック結果判定 */
    if ( retMvfs != MVFS_RET_SUCCESS ) {
        /* 不正メッセージ */
        return;
    }

    /* タスク単位情報取得 */
    pTaskInfo = GetTaskInfo( taskId );

    /* 取得結果判定 */
    if ( pTaskInfo == NULL ) {
        /* 失敗 */

        DEBUG_LOG_ERR(
            "%s(): GetTaskInfo(): taskId=%#X",
            __func__,
            taskId
        );

        /* Mount応答メッセージ送信 */
        MsgSendMountResp( taskId, MVFS_RESULT_FAILURE );

        return;
    }

    /* パラメータ設定 */
    param.taskId    = taskId;
    param.pBuffer   = pBuffer;
    param.pTaskInfo = pTaskInfo;
    param.pNode     = NULL;

    /* 状態遷移実行 */
    retMLib = MLibStateExec( &( pTaskInfo->stateHdl ),
                             EVENT_MOUNT_REQ,
                             &param,
                             &prevState,
                             &nextState,
                             &errMLib                  );

    /* 実行結果判定 */
    if ( retMLib != MLIB_SUCCESS ) {
        /* 失敗 */

        DEBUG_LOG_ERR(
            "%s(): MLibStateExec(): ret=%d, err=%#X",
            __func__,
            retMLib,
            errMLib
        );

        /* Mount応答メッセージ送信 */
        MsgSendMountResp( taskId, MVFS_RESULT_FAILURE );
    }

    DEBUG_LOG_TRC(
        "%s(): exec. state=%u->%u",
        __func__,
        prevState,
        nextState
    );

    /* タスク単位情報解放 */
    FreeTaskInfo( pTaskInfo );

    return;
}


/******************************************************************************/
/**
 * @brief       Select要求メッセージ受信
 * @details     メッセージ送信元タスクID(taskId)から受信したSelect要求メッセー
 *              ジを処理する。
 *
 * @param[in]   taskId   メッセージ送信元タスクID
 * @param[in]   *pBuffer メッセージバッファ
 * @param[in]   size     メッセージサイズ
 */
/******************************************************************************/
void FnTaskRecvSelectReq( MkTaskId_t taskId,
                          void       *pBuffer,
                          size_t     size      )
{
    uint32_t           errMLib;     /* MLIBエラー要因 */
    MLibRet_t          retMLib;     /* MLIB戻り値     */
    MvfsRet_t          retMvfs;     /* 戻り値         */
    taskInfo_t         *pTaskInfo;  /* タスク単位情報 */
    MLibState_t        prevState;   /* 遷移前状態     */
    MLibState_t        nextState;   /* 遷移後状態     */
    taskParam_t        param;       /* パラメータ     */
    MvfsMsgSelectReq_t *pMsg;       /* メッセージ     */

    /* 初期化 */
    errMLib   = MLIB_STATE_ERR_NONE;
    retMLib   = MLIB_FAILURE;
    retMvfs   = MVFS_RET_FAILURE;
    prevState = MLIB_STATE_NULL;
    nextState = MLIB_STATE_NULL;
    pMsg      = ( MvfsMsgSelectReq_t * ) pBuffer;
    memset( &param, 0, sizeof ( param ) );

    DEBUG_LOG_TRC(
        "%s(): taskId=%#X, size=%u",
        __func__,
        taskId,
        size
    );

    /* メッセージチェック */
    retMvfs = MsgCheckSelectReq( taskId, pMsg, size );

    /* チェック結果判定 */
    if ( retMvfs != MVFS_RET_SUCCESS ) {
        /* 不正メッセージ */
        return;
    }

    /* タスク単位情報取得 */
    pTaskInfo = GetTaskInfo( taskId );

    /* 取得結果判定 */
    if ( pTaskInfo == NULL ) {
        /* 失敗 */

        DEBUG_LOG_ERR(
            "%s(): GetTaskInfo(): taskId=%#X",
            __func__,
            taskId
        );

        /* Select応答メッセージ送信 */
        MsgSendSelectResp( taskId, MVFS_RESULT_FAILURE, NULL, 0, NULL, 0 );

        return;
    }

    /* パラメータ設定 */
    param.taskId    = taskId;
    param.pBuffer   = pBuffer;
    param.pTaskInfo = pTaskInfo;
    param.pNode     = NULL;

    /* 状態遷移実行 */
    retMLib = MLibStateExec( &( pTaskInfo->stateHdl ),
                             EVENT_SELECT_REQ,
                             &param,
                             &prevState,
                             &nextState,
                             &errMLib                  );

    /* 実行結果判定 */
    if ( retMLib != MLIB_SUCCESS ) {
        /* 失敗 */

        DEBUG_LOG_ERR(
            "%s(): MLibStateExec(): ret=%d, err=%#X",
            __func__,
            retMLib,
            errMLib
        );

        /* Select応答メッセージ送信 */
        MsgSendSelectResp( taskId, MVFS_RESULT_FAILURE, NULL, 0, NULL, 0 );
    }

    DEBUG_LOG_TRC(
        "%s(): exec. state=%u->%u",
        __func__,
        prevState,
        nextState
    );

    /* タスク単位情報解放 */
    FreeTaskInfo( pTaskInfo );

    return;
}


/******************************************************************************/
/**
 * @brief       VfsReady通知メッセージ受信
 * @details     メッセージ送信元タスクID(taskId)から受信したVfsReady通知メッ
 *              セージを処理する。
 *
 * @param[in]   taskId   メッセージ送信元タスクID
 * @param[in]   *pBuffer メッセージバッファ
 * @param[in]   size     メッセージサイズ
 */
/******************************************************************************/
void FnTaskRecvVfsReadyNtc( MkTaskId_t taskId,
                            void       *pBuffer,
                            size_t     size      )
{
    NodeInfo_t           *pNode;        /* ノード情報     */
    uint32_t             errMLib;       /* MLIBエラー要因 */
    MLibRet_t            retMLib;       /* MLIB戻り値     */
    MvfsRet_t            retMvfs;       /* 戻り値         */
    taskInfo_t           *pTaskInfo;    /* タスク単位情報 */
    taskParam_t          param;         /* パラメータ     */
    MvfsMsgVfsReadyNtc_t *pMsg;         /* メッセージ     */

    /* 初期化 */
    pNode     = NULL;
    errMLib   = MLIB_STATE_ERR_NONE;
    retMLib   = MLIB_FAILURE;
    retMvfs   = MVFS_RET_FAILURE;
    pMsg      = ( MvfsMsgVfsReadyNtc_t * ) pBuffer;
    memset( &param, 0, sizeof ( param ) );

    DEBUG_LOG_TRC(
        "%s(): taskId=%#X, size=%u",
        __func__,
        taskId,
        size
    );

    /* メッセージチェック */
    retMvfs = MsgCheckVfsReadyNtc( taskId, pMsg, size, &pNode );

    /* チェック結果判定 */
    if ( retMvfs != MVFS_RET_SUCCESS ) {
        /* 不正メッセージ */
        return;
    }

    /* レディ状態設定 */
    pNode->ready = pMsg->ready;

    /* パラメータ設定 */
    param.taskId    = taskId;
    param.pBuffer   = pBuffer;
    param.pTaskInfo = NULL;
    param.pNode     = pNode;

    /* タスク単位情報全操作 */
    retMLib = MLibDynamicArrayForeach( pgMngTbl,
                                       &errMLib,
                                       &ExecEventVfsReadyNtc,
                                       &param                 );

    /* 実行結果判定 */
    if ( retMLib != MLIB_SUCCESS ) {
        /* 失敗 */
        DEBUG_LOG_ERR(
            "%s(): MLibDynamicArrayForeach(): ret=%d, err=%#X",
            __func__,
            retMLib,
            errMLib
        );
    }

    return;
}


/******************************************************************************/
/* 内部モジュール向けグローバル関数定義                                       */
/******************************************************************************/
/******************************************************************************/
/**
 * @brief       機能(タスク単位)モジュール初期化
 * @details     タスク単位管理テーブルを初期化する。
 */
/******************************************************************************/
void TaskInit( void )
{
    MLibRet_t retMLib;  /* MLIB戻り値     */
    MLibErr_t errMLib;  /* MLIBエラー要因 */

    /* 初期化 */
    retMLib = MLIB_RET_FAILURE;
    errMLib = MLIB_ERR_NONE;

    /* タスク単位管理テーブル初期化 */
    retMLib = MLibDynamicArrayInit( &pgMngTbl,
                                    sizeof ( taskInfo_t ),
                                    MNGTBL_CHUNK_SIZE,
                                    SIZE_MAX,
                                    &errMLib               );

    /* 初期化結果判定 */
    if ( retMLib != MLIB_RET_SUCCESS ) {
        /* 失敗 */
        DEBUG_LOG_ERR(
            "%s(): MLibDynamicArrayInit(): ret=%d, err=%#X",
            __func__,
            retMLib,
            errMLib
        );
        DEBUG_ABORT();
    }

    return;
}


/******************************************************************************/
/* ローカル関数定義                                                           */
/******************************************************************************/
/******************************************************************************/
/**
 * @brief       タスクID比較
 * @details     タスク単位情報のタスクIDを比較する。
 *
 * @param[in]   idx     タスク単位情報インデックス
 * @param[in]   *pEntry タスク単位情報
 * @param[in]   vaList  可変引数リスト
 *
 * @return      比較結果を返す。
 * @retval      true  一致
 * @retval      false 不一致
 */
/******************************************************************************/
static bool CompareTaskId( uint_t   idx,
                           void     *pEntry,
                           va_list  vaList   )
{
    bool       ret;         /* 戻り値         */
    taskInfo_t *pTaskInfo;  /* タスク単位情報 */
    MkTaskId_t *pTaskId;    /* タスクID       */

    /* 初期化 */
    ret       = false;
    pTaskInfo = ( taskInfo_t * ) pEntry;
    pTaskId   = va_arg( vaList, MkTaskId_t * );

    /* タスクID判定 */
    if ( pTaskInfo->taskId == *pTaskId ) {
        /* 一致 */
        ret = true;
    }

    return ret;
}


/******************************************************************************/
/**
 * @brief       状態遷移タスク0101
 * @details     要求パスのノードを作成し、Mount応答メッセージを送信する。
 *
 * @param[in]   *pArg パラメータ
 *
 * @return      遷移先状態を返す。
 * @retval      STATE_INIT 初期状態
 */
/******************************************************************************/
static MLibState_t  DoTask0101( void *pArg )
{
    MvfsRet_t         retMvfs;      /* 関数戻り値   */
    NodeInfo_t        *pRootNode;   /* ルートノード */
    NodeInfo_t        *pNode;       /* ノード       */
    taskParam_t       *pParam;      /* パラメータ   */
    MvfsMsgMountReq_t *pMsg;        /* メッセージ   */

    /* 初期化 */
    retMvfs   = MVFS_RET_FAILURE;
    pRootNode = NULL;
    pNode     = NULL;
    pParam    = ( taskParam_t       * ) pArg;
    pMsg      = ( MvfsMsgMountReq_t * ) pParam->pBuffer;

    /* ノード取得 */
    pNode = NodeGet( pMsg->path );

    /* 取得結果判定 */
    if ( pNode != NULL ) {
        /* 既存 */

        DEBUG_LOG_ERR( "%s(): exist. path=%s", __func__, pMsg->path );
        return STATE_INIT;
    }

    /* ノード作成 */
    pNode = NodeCreate( &( pMsg->path[ 1 ] ),
                        pMsg->path,
                        NODE_TYPE_MOUNT_FILE,
                        pParam->taskId        );

    /* 作成結果判定 */
    if ( pNode == NULL ) {
        /* 失敗 */

        DEBUG_LOG_ERR( "%s(): NodeCreate()", __func__ );

        /* Mount応答メッセージ(失敗)送信 */
        MsgSendMountResp( pParam->taskId, MVFS_RESULT_FAILURE );

        return STATE_INIT;
    }

    /* ルートノード取得 */
    pRootNode = NodeGetRoot();

    /* ノードエントリ追加 */
    retMvfs = NodeAddEntry( pRootNode, pNode );

    /* 追加結果判定 */
    if ( retMvfs != MVFS_RET_SUCCESS ) {
        /* 失敗 */

        DEBUG_LOG_ERR( "%s(): NodeAddEntry()", __func__ );

        /* Mount応答メッセージ(失敗)送信 */
        MsgSendMountResp( pParam->taskId, MVFS_RESULT_FAILURE );

        /* ノード解放 */
        NodeDelete( pNode );

        return STATE_INIT;
    }

    /* Mount応答メッセージ(成功)送信 */
    MsgSendMountResp( pParam->taskId, MVFS_RESULT_SUCCESS );

    return STATE_INIT;
}


/******************************************************************************/
/**
 * @brief       状態遷移タスク0102
 * @details     監視対象のFDがレディかどうかを判定し、どれか1つでもレディ状態の
 *              場合はSelect応答メッセージを送信する。1つもレディ状態でない場合
 *              は、VfsReady通知待ち状態へ遷移する。
 *
 * @param[in]   *pArg パラメータ
 *
 * @return      遷移先状態を返す。
 * @retval      STATE_INIT          初期状態
 * @retval      STATE_VFSREADY_WAIT VfsReady待ち状態
 *
 * @note        本関数は下記を前提とする。
 *              - 監視グローバルFDが1つ以上ある。
 *              - 不正な監視グローバルFDが無い事。
 */
/******************************************************************************/
static MLibState_t  DoTask0102( void *pArg )
{
    size_t             readFdNum;       /* 読込グローバルFD数     */
    size_t             writeFdNum;      /* 書込グローバルFD数     */
    uint32_t           idx;             /* インデックス           */
    uint32_t           *pReadFdList;    /* 読込グローバルFDリスト */
    uint32_t           *pWriteFdList;   /* 書込グローバルFDリスト */
    FdInfo_t           *pFdInfo;        /* FD情報                 */
    taskInfo_t         *pTaskInfo;      /* タスク単位情報         */
    taskParam_t        *pParam;         /* パラメータ             */
    MLibState_t        ret;             /* 遷移先状態             */
    MvfsMsgSelectReq_t *pMsg;           /* メッセージ             */

    /* 初期化 */
    readFdNum    = 0;
    writeFdNum   = 0;
    idx          = 0;
    pReadFdList  = NULL;
    pWriteFdList = NULL;
    pFdInfo      = NULL;
    pParam       = ( taskParam_t        * ) pArg;
    pTaskInfo    = ( taskInfo_t         * ) pParam->pTaskInfo;
    pMsg         = ( MvfsMsgSelectReq_t * ) pParam->pBuffer;
    ret          = STATE_INIT;

    /* 読込監視グローバルFD有無判定 */
    if ( pMsg->readFdNum != 0 ) {
        /* 有り */

        /* 読込レディグローバルFDリスト割当 */
        pReadFdList = malloc( sizeof ( uint32_t ) * pMsg->readFdNum );

        /* 割当結果判定 */
        if ( pReadFdList == NULL ) {
            /* 失敗 */

            DEBUG_LOG_ERR(
                "%s(): malloc(): size=%u",
                __func__,
                sizeof ( uint32_t ) * pMsg->readFdNum
            );

            /* Select応答メッセージ送信 */
            MsgSendSelectResp( pParam->taskId,
                               MVFS_RESULT_FAILURE,
                               NULL,
                               0,
                               NULL,
                               0                    );

            return STATE_INIT;
        }
    }

    /* 書込監視グローバルFD有無判定 */
    if ( pMsg->writeFdNum != 0 ) {
        /* 有り */

        /* 書込レディグローバルFDリスト割当 */
        pWriteFdList = malloc( sizeof ( uint32_t ) * pMsg->writeFdNum );

        /* 割当結果判定 */
        if ( pWriteFdList == NULL ) {
            /* 失敗 */

            DEBUG_LOG_ERR(
                "%s(): malloc(): size=%u",
                __func__,
                sizeof ( uint32_t ) * pMsg->writeFdNum
            );

            /* Select応答メッセージ送信 */
            MsgSendSelectResp( pParam->taskId,
                               MVFS_RESULT_FAILURE,
                               NULL,
                               0,
                               NULL,
                               0                    );

            return STATE_INIT;
        }
    }

    /* 読込監視グローバルFD毎に繰り返す */
    for ( idx = 0; idx < pMsg->readFdNum; idx++ ) {
        /* FD情報取得 */
        pFdInfo = FdGet( pMsg->fd[ idx ] );

        /* 読込レディチェック */
        if ( ( pFdInfo->pNode->ready & MVFS_READY_READ ) != 0 ) {
            /* レディ */

            /* 読込レディグローバルFD設定 */
            pReadFdList[ readFdNum++ ] = pMsg->fd[ idx ];
        }
    }

    /* 書込監視グローバルFD毎に繰り返す */
    for ( idx = 0; idx < pMsg->writeFdNum; idx++ ) {
        /* FD情報取得 */
        pFdInfo = FdGet( pMsg->fd[ pMsg->readFdNum + idx ] );

        /* 書込レディチェック */
        if ( ( pFdInfo->pNode->ready & MVFS_READY_WRITE ) != 0 ) {
            /* レディ */

            /* 書込レディグローバルFD設定 */
            pWriteFdList[ writeFdNum++ ] = pMsg->fd[ pMsg->readFdNum + idx ];
        }
    }

    /* レディ状態のグローバルFD有無判定 */
    if ( ( readFdNum  == 0 ) && ( writeFdNum == 0 ) ) {
        /* 無し */

        /* 監視グローバルFDリストコピー */
        memcpy( pReadFdList,
                &( pMsg->fd[ 0 ] ),
                sizeof ( uint32_t ) * pMsg->readFdNum );
        memcpy( pWriteFdList,
                &( pMsg->fd[ pMsg->readFdNum ] ),
                sizeof ( uint32_t ) *pMsg->writeFdNum );

        /* 監視グローバルFDリスト登録 */
        pTaskInfo->pReadFdList  = pReadFdList;
        pTaskInfo->pWriteFdList = pWriteFdList;
        pTaskInfo->readFdNum    = pMsg->readFdNum;
        pTaskInfo->writeFdNum   = pMsg->writeFdNum;

        /* 遷移先状態設定 */
        ret = STATE_VFSREADY_WAIT;

    } else {
        /* 有り */

        /* Select応答メッセージ送信 */
        MsgSendSelectResp( pParam->taskId,
                           MVFS_RESULT_SUCCESS,
                           pReadFdList,
                           readFdNum,
                           pWriteFdList,
                           writeFdNum           );

        /* グローバルFDリスト解放 */
        free( pReadFdList  );
        free( pWriteFdList );

        /* 遷移先状態設定 */
        ret = STATE_INIT;
    }

    return ret;
}


/******************************************************************************/
/**
 * @brief       状態遷移タスク0202
 * @details     受付処理中のSelect要求を破棄し、新しいSelect要求を受け付ける。
 *
 * @param[in]   *pArg パラメータ
 *
 * @return      遷移先状態を返す。
 * @retval      STATE_INIT          初期状態
 * @retval      STATE_VFSREADY_WAIT VfsReady待ち状態
 */
/******************************************************************************/
static MLibState_t DoTask0202( void *pArg )
{
    taskInfo_t  *pTaskInfo; /* タスク単位情報 */
    taskParam_t *pParam;    /* パラメータ     */

    /* 初期化 */
    pParam    = ( taskParam_t * ) pArg;
    pTaskInfo = ( taskInfo_t  * ) pParam->pTaskInfo;

    /* 監視グローバルFDリスト解放 */
    free( pTaskInfo->pReadFdList  );
    free( pTaskInfo->pWriteFdList );
    pTaskInfo->pReadFdList  = NULL;
    pTaskInfo->pWriteFdList = NULL;
    pTaskInfo->readFdNum    = 0;
    pTaskInfo->writeFdNum   = 0;

    /* Select要求処理 */
    return DoTask0102( pArg );
}


/******************************************************************************/
/**
 * @brief       状態遷移タスク0203
 * @details     読込・書込監視中のFDのノードがレディ状態になったかを判定し、レ
 *              ディ状態になった場合はSelect応答メッセージを送信する。
 *
 * @param[in]   *pArg パラメータ
 *
 * @return      遷移先状態を返す。
 * @retval      STATE_INIT          初期状態
 * @retval      STATE_VFSREADY_WAIT VfsReady待ち状態
 */
/******************************************************************************/
static MLibState_t  DoTask0203( void *pArg )
{
    size_t               readFdNum;     /* 読込レディFD数 */
    size_t               writeFdNum;    /* 書込レディFD数 */
    uint32_t             idx;           /* インデックス   */
    uint32_t             readFd;        /* 読込レディFD   */
    uint32_t             writeFd;       /* 書込レディFD   */
    FdInfo_t             *pFdInfo;      /* FD情報         */
    taskInfo_t           *pTaskInfo;    /* タスク単位情報 */
    taskParam_t          *pParam;       /* パラメータ     */
    MLibState_t          ret;           /* 遷移先状態     */
    MvfsMsgVfsReadyNtc_t *pMsg;         /* メッセージ     */

    /* 初期化 */
    readFdNum  = 0;
    writeFdNum = 0;
    idx        = 0;
    pFdInfo    = NULL;
    pParam     = ( taskParam_t          * ) pArg;
    pTaskInfo  = ( taskInfo_t           * ) pParam->pTaskInfo;
    pMsg       = ( MvfsMsgVfsReadyNtc_t * ) pParam->pBuffer;
    ret        = STATE_INIT;

    /* 読込レディ判定 */
    if ( ( pParam->pNode->ready & MVFS_READY_READ ) != 0 ) {
        /* レディ */

        /* 読込監視グローバルFD毎に繰り返す */
        for ( idx = 0;  idx < pTaskInfo->readFdNum; idx++ ) {
            /* FD情報取得 */
            pFdInfo = FdGet( pTaskInfo->pReadFdList[ idx ] );

            /* 取得結果判定 */
            if ( pFdInfo == NULL ) {
                /* 失敗 */

                DEBUG_LOG_ERR(
                    "%s(): FdGet(): idx=%u, globalFd=%u",
                    __func__,
                    idx,
                    pTaskInfo->pReadFdList[ idx ]
                );

                /* Select応答メッセージ送信 */
                MsgSendSelectResp( pTaskInfo->taskId,
                                   MVFS_RESULT_FAILURE,
                                   NULL,
                                   0,
                                   NULL,
                                   0                    );

                /* 監視グローバルFDリスト解放 */
                free( pTaskInfo->pReadFdList  );
                free( pTaskInfo->pWriteFdList );
                pTaskInfo->pReadFdList  = NULL;
                pTaskInfo->pWriteFdList = NULL;
                pTaskInfo->readFdNum    = 0;
                pTaskInfo->writeFdNum   = 0;

                return STATE_INIT;
            }

            /* ノード情報判定 */
            if ( pFdInfo->pNode == pParam->pNode ) {
                /* 一致 */

                readFd    = pTaskInfo->pReadFdList[ idx ];
                readFdNum = 1;
                break;
            }
        }
    }

    /* 書込レディ判定 */
    if ( ( pParam->pNode->ready & MVFS_READY_WRITE ) != 0 ) {
        /* レディ */

        /* 書込監視グローバルFD毎に繰り返す */
        for ( idx = 0;  idx < pTaskInfo->writeFdNum; idx++ ) {
            /* FD情報取得 */
            pFdInfo = FdGet( pTaskInfo->pWriteFdList[ idx ] );

            /* 取得結果判定 */
            if ( pFdInfo == NULL ) {
                /* 失敗 */

                DEBUG_LOG_ERR(
                    "%s(): FdGet(): idx=%u, globalFd=%u",
                    __func__,
                    idx,
                    pTaskInfo->pWriteFdList[ idx ]
                );

                /* Select応答メッセージ送信 */
                MsgSendSelectResp( pTaskInfo->taskId,
                                   MVFS_RESULT_FAILURE,
                                   NULL,
                                   0,
                                   NULL,
                                   0                    );

                /* 監視グローバルFDリスト解放 */
                free( pTaskInfo->pReadFdList  );
                free( pTaskInfo->pWriteFdList );
                pTaskInfo->pReadFdList  = NULL;
                pTaskInfo->pWriteFdList = NULL;
                pTaskInfo->readFdNum    = 0;
                pTaskInfo->writeFdNum   = 0;

                return STATE_INIT;
            }

            /* ノード情報判定 */
            if ( pFdInfo->pNode == pParam->pNode ) {
                /* 一致 */

                writeFd    = pTaskInfo->pWriteFdList[ idx ];
                writeFdNum = 1;
                break;
            }
        }
    }

    /* レディチェック */
    if ( ( writeFdNum == 0 ) && ( readFdNum == 0 ) ) {
        /* レディ無し */

        /* 遷移先状態設定 */
        ret = STATE_VFSREADY_WAIT;

    } else {
        /* レディ有り */

        /* Select応答メッセージ送信 */
        MsgSendSelectResp( pParam->pTaskInfo->taskId,
                           MVFS_RESULT_SUCCESS,
                           &readFd,
                           readFdNum,
                           &writeFd,
                           writeFdNum                 );

        /* 監視グローバルFDリスト解放 */
        free( pTaskInfo->pReadFdList  );
        free( pTaskInfo->pWriteFdList );
        pTaskInfo->pReadFdList  = NULL;
        pTaskInfo->pWriteFdList = NULL;
        pTaskInfo->readFdNum    = 0;
        pTaskInfo->writeFdNum   = 0;

        /* 遷移先状態設定 */
        ret = STATE_INIT;
    }

    return ret;
}


/******************************************************************************/
/**
 * @brief       VfsReady通知イベント実行
 * @details     タスク単位情報毎にVfsReady通知イベントを実行する。
 *
 * @param[in]   idx     タスク単位情報インデックス
 * @param[in]   *pEntry タスク単位情報
 * @param[in]   vaList  可変引数リスト
 */
/******************************************************************************/
static void ExecEventVfsReadyNtc( uint_t  idx,
                                  void    *pEntry,
                                  va_list vaList   )
{
    uint32_t    errMLib;    /* MLIBエラー要因 */
    MLibRet_t   retMLib;    /* MLIB戻り値     */
    taskInfo_t  *pTaskInfo; /* タスク単位情報 */
    MLibState_t prevState;  /* 遷移前状態     */
    MLibState_t nextState;  /* 遷移後状態     */
    taskParam_t *pParam;    /* パラメータ     */

    /* 初期化 */
    pTaskInfo = ( taskInfo_t * ) pEntry;
    errMLib   = MLIB_STATE_ERR_NONE;
    retMLib   = MLIB_FAILURE;
    prevState = MLIB_STATE_NULL;
    nextState = MLIB_STATE_NULL;
    pParam    = va_arg( vaList, taskParam_t * );

    /* パラメータ設定 */
    pParam->pTaskInfo = pTaskInfo;

    /* 状態遷移実行 */
    retMLib = MLibStateExec( &( pTaskInfo->stateHdl ),
                             EVENT_VFSREADY_NTC,
                             pParam,
                             &prevState,
                             &nextState,
                             &errMLib                  );

    /* 実行結果判定 */
    if ( retMLib != MLIB_RET_SUCCESS ) {
        /* 失敗 */
        DEBUG_LOG_ERR(
            "%s(): MLibStateExec(): ret=%d, err=%#X",
            __func__,
            retMLib,
            errMLib
        );
    }

    DEBUG_LOG_TRC(
        "%s(): exec. taskId=%#X, state=%u->%u",
        __func__,
        pTaskInfo->taskId,
        prevState,
        nextState
    );

    /* タスク単位情報解放 */
    FreeTaskInfo( pTaskInfo );

    return;
}


/******************************************************************************/
/**
 * @brief       タスク単位情報解放
 * @details     タスク単位情報の状態が初期状態の場合はタスク単位情報を解放する。
 *
 * @param[in]   *pTaskInfo タスク単位情報
 */
/******************************************************************************/
static void FreeTaskInfo( taskInfo_t *pTaskInfo )
{
    MLibState_t state;  /* 状態 */

    /* 状態取得 */
    state = MLibStateGet( &( pTaskInfo->stateHdl ), NULL );

    /* 状態判定 */
    if ( state == STATE_INIT ) {
        /* 初期状態 */

        DEBUG_LOG_TRC(
            "%s(): idx=%u, taskId=%#X",
            __func__,
            pTaskInfo->idx,
            pTaskInfo->taskId
        );

        /* タスク単位情報解放 */
        MLibDynamicArrayFree( pgMngTbl, pTaskInfo->idx, NULL );
    }

    return;
}


/******************************************************************************/
/**
 * @brief       タスク単位情報取得
 * @details     タスク単位管理テーブルから引数taskIdに該当するタスク単位情報を
 *              取得する。該当するタスク単位情報が無い場合は新たに割り当てる。
 *
 * @param[in]   taskId タスクID
 *
 * @return      タスク情報を返す。
 * @retval      NULL     失敗
 * @retval      NULL以外 成功
 */
/******************************************************************************/
static taskInfo_t *GetTaskInfo( MkTaskId_t taskId )
{
    uint32_t   idx;         /* インデックス   */
    MLibRet_t  retMLib;     /* MLIB戻り値     */
    MLibErr_t  errMLib;     /* MLIBエラー要因 */
    taskInfo_t *pTaskInfo;  /* タスク単位情報 */

    /* 初期化 */
    idx       = 0;
    retMLib   = MLIB_RET_SUCCESS;
    errMLib   = MLIB_ERR_NONE;
    pTaskInfo = NULL;

    /* タスク単位情報検索 */
    retMLib = MLibDynamicArraySearch( pgMngTbl,
                                      &idx,
                                      ( void ** ) &pTaskInfo,
                                      &errMLib,
                                      &CompareTaskId,
                                      &taskId                 );

    /* 検索結果判定 */
    if ( retMLib == MLIB_RET_SUCCESS ) {
        /* 該当有 */
        return pTaskInfo;
    }

    /* タスク単位情報割当て */
    retMLib = MLibDynamicArrayAlloc( pgMngTbl,
                                     &idx,
                                     ( void ** ) &pTaskInfo,
                                     &errMLib                );

    /* 割当て結果判定 */
    if ( retMLib != MLIB_RET_SUCCESS ) {
        /* 失敗 */

        DEBUG_LOG_ERR(
            "%s(): MLibDynamicArrayAlloc(): ret=%d, err=%#X",
            __func__,
            retMLib,
            errMLib
        );
        return NULL;
    }

    /* タスク単位情報初期化 */
    memset( pTaskInfo, 0, sizeof ( taskInfo_t ) );
    pTaskInfo->idx    = idx;
    pTaskInfo->taskId = taskId;

    /* 状態遷移初期化 */
    MLibStateInit( &( pTaskInfo->stateHdl ),
                   gStt,
                   sizeof ( gStt ),
                   STATE_INIT,
                   NULL                      );

    return pTaskInfo;
}


/******************************************************************************/
