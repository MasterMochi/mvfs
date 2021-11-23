/******************************************************************************/
/*                                                                            */
/* src/libmvfs/Reopen.c                                                       */
/*                                                                 2021/11/07 */
/* Copyright (C) 2021 Mochi.                                                  */
/*                                                                            */
/******************************************************************************/
/******************************************************************************/
/* インクルード                                                               */
/******************************************************************************/
/* 標準ヘッダ */
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

/* ライブラリヘッダ */
#include <MLib/MLib.h>
#include <libmvfs.h>

/* モジュール内ヘッダ */
#include "Fd.h"
#include "Open.h"


/******************************************************************************/
/* 定義                                                                       */
/******************************************************************************/
/** 再オープンデータ */
typedef struct {
    bool       failure; /**< 失敗有無     */
    uint32_t   *pErrNo; /**< エラー番号   */
    MkTaskId_t taskId;  /**< MVFSタスクID */
} ReopenData_t;


/******************************************************************************/
/* ローカル関数宣言                                                           */
/******************************************************************************/
/* ファイル再オープン */
static void Reopen( FdInfo_t *pFdInfo,
                    void     *pParam   );


/******************************************************************************/
/* ライブラリ関数定義                                                         */
/******************************************************************************/
/******************************************************************************/
/**
 * @brief       全ファイル再オープン
 * @details     オープン済みの全てのファイルを再オープンする。
 *
 * @param[out]  *pErrNo エラー番号
 *
 * @return      処理結果を返す。
 * @retval      LIBMVFS_RET_SUCCESS 成功
 * @retval      LIBMVFS_RET_FAILURE 失敗
 */
/******************************************************************************/
LibMvfsRet_t LibMvfsReopen( uint32_t *pErrNo )
{
    ReopenData_t data;  /* 再オープンデータ */
    LibMvfsRet_t ret;   /* 戻り値           */

    /* 初期化 */
    memset( &data, 0, sizeof ( ReopenData_t ) );
    data.failure = false;
    data.pErrNo  = pErrNo;
    data.taskId  = MK_TASKID_NULL;
    ret          = LIBMVFS_RET_FAILURE;

    /* タスクID取得 */
    ret = LibMvfsGetTaskId( &( data.taskId ), pErrNo );

    /* 取得結果判定 */
    if ( ret != LIBMVFS_RET_SUCCESS ) {
        /* 失敗 */

        return LIBMVFS_RET_FAILURE;
    }

    /* 全ファイル再オープン */
    FdForeach( Reopen, &data );

    /* 結果判定 */
    if ( data.failure != false ) {
        /* 失敗 */

        return LIBMVFS_RET_FAILURE;
    }

    return LIBMVFS_RET_SUCCESS;
}


/******************************************************************************/
/* ローカル関数                                                               */
/******************************************************************************/
/******************************************************************************/
/**
 * @brief           ファイル再オープン
 * @details         オープン済みのファイルを再オープンする。
 *
 * @param[in]       *pFdInfo FD情報
 * @param[in, out]  *pParam  再オープンデータ
 */
/******************************************************************************/
static void Reopen( FdInfo_t *pFdInfo,
                    void     *pParam   )
{
    LibMvfsRet_t ret;       /* 戻り値           */
    ReopenData_t *pData;    /* 再オープンデータ */

    /* 初期化 */
    ret   = LIBMVFS_RET_FAILURE;
    pData = ( ReopenData_t * ) pParam;

    /* ファイルオープン */
    ret = OpenDo( pData->taskId,
                  pFdInfo->path,
                  pFdInfo->localFd,
                  &( pFdInfo->globalFd ),
                  pData->pErrNo           );

    /* 結果判定 */
    if ( ret != LIBMVFS_RET_SUCCESS ) {
        /* 失敗 */

        /* ローカルFD解放 */
        FdFree( pFdInfo->localFd );

        /* エラー設定 */
        pData->failure = true;
    }

    return;
}


/******************************************************************************/
