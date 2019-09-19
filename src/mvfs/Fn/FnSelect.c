/******************************************************************************/
/*                                                                            */
/* src/mvfs/Select.c                                                          */
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
#define STATE_INI               ( 1 )   /**< 初期状態         */
#define STATE_VFSREADY_NTC_WAIT ( 2 )   /**< VfsReady通知待ち */

/* イベント */
#define EVENT_SELECT_REQ   ( 1 )    /**< Select要求イベント   */
#define EVENT_VFSREADY_NTC ( 2 )    /**< VfsReady通知イベント */

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

static MkTaskId_t gSelectTaskId;


/******************************************************************************/
/* グローバル変数定義                                                         */
/******************************************************************************/
/* 状態遷移表 */
static const MLibStateTransition_t gStt[] = {
    { STATE_INI              , EVENT_SELECT_REQ  , Task0101, { STATE_INI, STATE_VFSREADY_NTC_WAIT, 0 } },
    { STATE_INI              , EVENT_VFSREADY_NTC, NULL    , { STATE_INI, 0                          } },
    { STATE_VFSREADY_NTC_WAIT, EVENT_VFSREADY_NTC, Task0202, { STATE_INI, STATE_VFSREADY_NTC_WAIT, 0 } }  };

/** 状態遷移ハンドル */
static MLibStateHandle_t gStateHdl;


/******************************************************************************/
/* 外部モジュール向けグローバル関数定義                                       */
/******************************************************************************/
/******************************************************************************/
/**
 * @brief       Select要求メッセージ受信
 * @details     メッセージ送信元タスクID(taskId)からのSelect要求メッセージを処
 *              理する。
 *
 * @param[in]   taskId   メッセージ送信元タスクID
 * @param[in]   *pBuffer メッセージバッファ
 */
/******************************************************************************/
void FnSelectRecvSelectReq( MkTaskId_t taskId,
                            void       *pBuffer )
{
    uint32_t           err;         /* エラー                   */
    MLibRet_t          retMLib;     /* MLib戻り値               */
    MLibState_t        prevState;   /* 遷移前状態               */
    MLibState_t        nextState;   /* 遷移後状態               */
    StateTaskParam_t   param;       /* 状態遷移タスクパラメータ */
    MvfsMsgSelectReq_t *pMsg;       /* メッセージ               */

    DEBUG_LOG_TRC(
        "%s(): start. taskId=0x%X, pBuffer=%p",
        __func__,
        taskId,
        pBuffer
    );

    /* 初期化 */
    err       = MLIB_STATE_ERR_NONE;
    retMLib   = MLIB_FAILURE;
    prevState = MLIB_STATE_NULL;
    nextState = MLIB_STATE_NULL;
    pMsg      = ( MvfsMsgSelectReq_t * ) pBuffer;
    memset( &param, 0, sizeof ( param ) );

    /* 状態遷移パラメータ設定 */
    param.taskId  = taskId;
    param.pBuffer = pBuffer;

    /* 状態遷移実行 */
    retMLib = MLibStateExec( &gStateHdl,
                             EVENT_SELECT_REQ,
                             &param,
                             &prevState,
                             &nextState,
                             &err              );

    /* 実行結果判定 */
    if ( retMLib != MLIB_SUCCESS ) {
        /* 失敗 */
        DEBUG_LOG_ERR( "MLibStateExec(): ret=%d, err=0x%X", retMLib, err );

        /* Select応答メッセージ送信 */
        MsgSendSelectResp( taskId, MVFS_RESULT_FAILURE, NULL, 0, NULL, 0 );

        DEBUG_LOG_TRC( "%s(): end.", __func__ );
        return;
    }

    DEBUG_LOG_TRC( "state: %u->%u", prevState, nextState );
    DEBUG_LOG_TRC( "%s(): end.", __func__ );
    return;
}


/******************************************************************************/
/**
 * @brief       VfsReady通知メッセージ受信
 * @details     メッセージ送信元タスクID(taskId)からのVfsReady通知メッセージを
 *              処理する。
 *
 * @param[in]   taskId   メッセージ送信元タスクID
 * @param[in]   *pBuffer メッセージバッファ
 */
/******************************************************************************/
void FnSelectRecvVfsReadyNtc( MkTaskId_t taskId,
                              void       *pBuffer )
{
    uint32_t             err;       /* エラー                   */
    MLibRet_t            retMLib;   /* MLib戻り値               */
    MLibState_t          prevState; /* 遷移前状態               */
    MLibState_t          nextState; /* 遷移後状態               */
    StateTaskParam_t     param;     /* 状態遷移タスクパラメータ */
    MvfsMsgVfsReadyNtc_t *pMsg;     /* メッセージ               */

    DEBUG_LOG_TRC(
        "%s(): start. taskId=0x%X, pBuffer=%p",
        __func__,
        taskId,
        pBuffer
    );

    /* 初期化 */
    err       = MLIB_STATE_ERR_NONE;
    retMLib   = MLIB_FAILURE;
    prevState = MLIB_STATE_NULL;
    nextState = MLIB_STATE_NULL;
    pMsg      = ( MvfsMsgVfsReadyNtc_t * ) pBuffer;
    memset( &param, 0, sizeof ( param ) );

    DEBUG_LOG_TRC( "ready=0x%X", pMsg->ready );

    /* 状態遷移パラメータ設定 */
    param.taskId   = taskId;
    param.pBuffer  = pBuffer;

    /* 状態遷移実行 */
    retMLib = MLibStateExec( &gStateHdl,
                             EVENT_VFSREADY_NTC,
                             &param,
                             &prevState,
                             &nextState,
                             &err                );

    /* 実行結果判定 */
    if ( retMLib != MLIB_SUCCESS ) {
        /* 失敗 */
        DEBUG_LOG_ERR( "MLibStateExec(): ret=%d, err=0x%X", retMLib, err );
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
 * @brief       Select機能初期化
 * @details     状態遷移を初期化する。
 */
/******************************************************************************/
void SelectInit( void )
{
    uint32_t  err;  /* エラー     */
    MLibRet_t ret;  /* MLib戻り値 */

    DEBUG_LOG_FNC( "%s(): start.", __func__ );

    /* 初期化 */
    err = MLIB_STATE_ERR_NONE;
    ret = MLIB_FAILURE;

    /* 状態遷移初期化 */
    ret = MLibStateInit( &gStateHdl,
                         gStt,
                         sizeof ( gStt ),
                         STATE_INI,
                         &err             );

    /* 初期化結果判定 */
    if ( ret != MLIB_SUCCESS ) {
        /* 失敗 */
        DEBUG_LOG_ERR( "MLibStateInit(): ret=%d, err=0x%X", ret, err );
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
 * @details     監視グローバルFDの読込/書込レディをチェックし、レディ状態の場合
 *              はSelect応答メッセージを送信する。レディ状態でない場合は
 *              VfsReady通知待ち合わせ状態に遷移する。
 *
 * @param[in]   *pArg 状態遷移パラメータ
 *
 * @return      遷移先状態を返す。
 * @retval      STATE_INI               初期状態
 * @retval      STATE_VFSREADY_NTC_WAIT VfsReady通知待ち
 */
/******************************************************************************/
static MLibState_t Task0101( void *pArg )
{
    bool               valid;           /* 有効FD有無                   */
    size_t             idx;             /* インデックス                 */
    size_t             readFdNum;       /* 読込レディグローバルFD数     */
    size_t             writeFdNum;      /* 書込レディグローバルFD数     */
    uint32_t           *pReadFdList;    /* 読込レディグローバルFDリスト */
    uint32_t           *pWriteFdList;   /* 書込レディグローバルFDリスト */
    FdInfo_t           *pFdInfo;        /* ファイルディスクリプタ情報   */
    MLibState_t        ret;             /* 関数戻り値                   */
    StateTaskParam_t   *pParam;         /* 状態遷移パラメータ           */
    MvfsMsgSelectReq_t *pMsg;           /* Select要求メッセージ         */

    /* 初期化 */
    valid        = false;
    readFdNum    = 0;
    writeFdNum   = 0;
    pReadFdList  = NULL;
    pWriteFdList = NULL;
    pFdInfo      = NULL;
    ret          = STATE_INI;
    pParam       = ( StateTaskParam_t   * ) pArg;
    pMsg         = ( MvfsMsgSelectReq_t * ) pParam->pBuffer;

    /* 読込監視グローバルFD数判定 */
    if ( pMsg->readFdNum != 0 ) {
        /* 0でない */

        /* 読込レディグローバルFDリスト割当 */
        pReadFdList = malloc( sizeof ( uint32_t ) * pMsg->readFdNum );

        /* 割当結果判定 */
        if ( pReadFdList == NULL ) {
            /* 失敗 */

            /* Select応答メッセージ送信 */
            MsgSendSelectResp( pParam->taskId,
                               MVFS_RESULT_FAILURE,
                               NULL,
                               0,
                               NULL,
                               0                    );

            return STATE_INI;
        }
    }

    /* 書込監視グローバルFD数判定 */
    if ( pMsg->writeFdNum != 0 ) {
        /* 0でない */

        /* 書込レディグローバルFDリスト割当 */
        pWriteFdList = malloc( sizeof ( uint32_t ) * pMsg->writeFdNum );

        /* 割当結果判定 */
        if ( pReadFdList == NULL ) {
            /* 失敗 */

            /* Select応答メッセージ送信 */
            MsgSendSelectResp( pParam->taskId,
                               MVFS_RESULT_FAILURE,
                               NULL,
                               0,
                               NULL,
                               0                    );

            /* 読込みレディグローバルFDリスト解放 */
            free( pReadFdList );

            return STATE_INI;
        }
    }

    /* 読込監視グローバルFD毎に繰り返し */
    for ( idx = 0; idx < pMsg->readFdNum; idx ++ ) {
        /* FD情報取得 */
        pFdInfo = FdGet( pMsg->fd[ idx ] );

        /* 取得結果判定 */
        if ( pFdInfo == NULL ) {
            /* 失敗 */
            continue;
        }

        /* Openチェック */
        if ( pFdInfo->pid != MK_TASKID_TO_PID( pParam->taskId ) ) {
            /* 未Open */
            continue;
        }

        /* 有効FD有設定 */
        valid = true;

        /* 読込レディチェック */
        if ( ( pFdInfo->pNode->ready & MVFS_READY_READ ) == 0 ) {
            /* 非レディ */

            /* 監視状態設定 */
            pFdInfo->select |= MVFS_READY_READ;
            continue;
        }

        /* 読込レディグローバルFD設定 */
        pReadFdList[ readFdNum++ ] = pMsg->fd[ idx ];
    }

    /* 書込監視グローバルFD毎に繰り返し */
    for ( idx = 0; idx < pMsg->writeFdNum; idx++ ) {
        /* FD情報取得 */
        pFdInfo = FdGet( pMsg->fd[ pMsg->readFdNum + idx ] );

        /* 取得結果判定 */
        if ( pFdInfo == NULL ) {
            /* 失敗 */
            continue;
        }

        /* Openチェック */
        if ( pFdInfo->pid != MK_TASKID_TO_PID( pParam->taskId ) ) {
            /* 未Open */
            continue;
        }

        /* 有効FD有設定 */
        valid = true;

        /* 書込レディチェック */
        if ( ( pFdInfo->pNode->ready & MVFS_READY_WRITE ) == 0 ) {
            /* 非レディ */

            /* 監視状態設定 */
            pFdInfo->select |= MVFS_READY_WRITE;
            continue;
        }

        /* 書込レディグローバルFD設定 */
        pWriteFdList[ writeFdNum++ ] = pMsg->fd[ pMsg->readFdNum + idx ];
    }

    /* レディ数・有効判定 */
    if ( ( readFdNum == 0 ) && ( writeFdNum == 0 ) && ( valid != false ) ) {
        /* レディ無し・有効 */

        /* 待ち合わせ中タスクID設定 */
        gSelectTaskId = pParam->taskId;

        /* 状態遷移先設定 */
        ret = STATE_VFSREADY_NTC_WAIT;

    } else {
        /* レディ有り・有効、または、無効 */

        /* Select応答メッセージ送信 */
        MsgSendSelectResp( pParam->taskId,
                           MVFS_RESULT_SUCCESS,
                           pReadFdList,
                           readFdNum,
                           pWriteFdList,
                           writeFdNum           );

        /* 状態遷移先設定 */
        ret = STATE_INI;
    }

    /* FDリスト解放 */
    free( pReadFdList  );   /* 読込レディグローバルFDリスト */
    free( pWriteFdList );   /* 書込レディグローバルFDリスト */

    return ret;
}


/******************************************************************************/
/**
 * @brief       状態遷移タスク0202
 * @details     Select応答メッセージを送信する。
 *
 * @param[in]   *pArg 状態遷移パラメータ
 *
 * @return      状態遷移を返す。
 * @retval      STATE_INI               初期状態
 * @retval      STATE_VFSREADY_NTC_WAIT VfsReady通知待ち
 */
/******************************************************************************/
static MLibState_t Task0202( void *pArg )
{
    size_t               readFdNum;     /* 読込レディグローバルFD数   */
    size_t               writeFdNum;    /* 書込レディグローバルFD数   */
    FdInfo_t             *pFdInfo;      /* ファイルディスクリプタ情報 */
    MLibState_t          ret;           /* 戻り値(状態遷移先)         */
    StateTaskParam_t     *pParam;       /* 状態遷移パラメータ         */
    MvfsMsgVfsReadyNtc_t *pMsg;         /* VfsReady応答メッセージ     */

    /* 初期化 */
    readFdNum  = 0;
    writeFdNum = 0;
    ret        = STATE_VFSREADY_NTC_WAIT;
    pParam     = ( StateTaskParam_t     * ) pArg;
    pMsg       = ( MvfsMsgVfsReadyNtc_t * ) pParam->pBuffer;

    /* ファイルディスクリプタ情報取得 */
    pFdInfo = FdGet( pMsg->globalFd );

    /* 取得結果判定 */
    if ( pFdInfo == NULL ) {
        /* 失敗 */

        DEBUG_LOG_FNC( "%s(): end. ret=%u", __func__, STATE_VFSREADY_NTC_WAIT );
        return STATE_VFSREADY_NTC_WAIT;
    }

    /* レディ状態設定 */
    pFdInfo->pNode->ready = pMsg->ready;

    /* 読込レディ状態・監視状態判定 */
    if ( ( ( pMsg->ready     & MVFS_READY_READ ) != 0 ) &&
         ( ( pFdInfo->select & MVFS_READY_READ ) != 0 )    ){
        /* 読込レディ、かつ、監視対象 */

        /* 読込レディグローバルFD数設定 */
        readFdNum = 1;
    }

    /* 書込レディ状態・監視状態判定 */
    if ( ( ( pMsg->ready     & MVFS_READY_WRITE ) != 0 ) &&
         ( ( pFdInfo->select & MVFS_READY_READ  ) != 0 )    ) {
        /* 書込レディ */

        /* 書込レディグローバルFD数設定 */
        writeFdNum = 1;
    }

    /* Select応答要否判定 */
    if ( ( readFdNum != 0 ) || ( writeFdNum != 0 ) ) {
        /* 必要 */

        /* Select応答メッセージ送信 */
        MsgSendSelectResp( gSelectTaskId,
                           MVFS_RESULT_SUCCESS,
                           &( pMsg->globalFd ),
                           readFdNum,
                           &( pMsg->globalFd ),
                           writeFdNum           );

        /* 状態遷移先設定 */
        ret = STATE_INI;

    } else {
        /* 不要 */

        /* 状態遷移先設定 */
        ret = STATE_VFSREADY_NTC_WAIT;
    }

    DEBUG_LOG_FNC( "%s(): end. ret=%u", __func__, ret );
    return ret;
}


/******************************************************************************/
