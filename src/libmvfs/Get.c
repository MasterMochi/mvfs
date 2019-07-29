/******************************************************************************/
/*                                                                            */
/* src/libmvfs/Get.c                                                          */
/*                                                                 2019/07/28 */
/* Copyright (C) 2019 Mochi.                                                  */
/*                                                                            */
/******************************************************************************/
/******************************************************************************/
/* インクルード                                                               */
/******************************************************************************/
/* 標準ヘッダ */
#include <stdbool.h>
#include <stdint.h>

/* カーネルヘッダ */
#include <kernel/types.h>

/* ライブラリヘッダ */
#include <libmk.h>
#include <libmvfs.h>
#include <MLib/MLib.h>

/* 共通ヘッダ */
#include <mvfs.h>


/******************************************************************************/
/* 定義                                                                       */
/******************************************************************************/
/* リトライ */
#define RETRY_MAX  (    10 )    /**< リトライ上限回数             */
#define RETRY_WAIT ( 10000 )    /**< リトライウェイト(マイクロ秒) */


/******************************************************************************/
/* ライブラリ関数定義                                                         */
/******************************************************************************/
/******************************************************************************/
/**
 * @brief       mvfsタスクID取得
 * @details     仮想ファイルサーバのメッセージ受信タスクIDを取得する。
 *
 * @param[out]  *pTaskId タスクID
 * @param[out]  *pErrNo  エラー番号
 *                  - LIBMVFS_ERR_NONE      エラー無し
 *                  - LIBMVFS_ERR_PARAM     パラメータエラー
 *                  - LIBMVFS_ERR_NOT_FOUND 仮想ファイルサーバが不明
 *
 * @return      取得結果を返す。
 * @retval      LIBMVFS_RET_SUCCESS 正常終了
 * @retval      LIBMVFS_RET_FAILURE 異常終了
 */
/******************************************************************************/
LibMvfsRet_t LibMvfsGetTaskId( MkTaskId_t *pTaskId,
                               uint32_t   *pErrNo   )
{
    MkRet_t  ret;   /* 関数戻り値       */
    MkErr_t  err;   /* エラー内容       */
    uint32_t retry; /* リトライカウンタ */

    /* 初期化 */
    ret   = MK_RET_FAILURE;
    err   = MK_ERR_NONE;
    retry = 0;
    MLIB_SET_IFNOT_NULL( pErrNo, LIBMVFS_ERR_NONE );

    /* パラメータチェック */
    if ( pTaskId == NULL ) {
        /* 不正 */

        /* エラー番号設定 */
        MLIB_SET_IFNOT_NULL( pErrNo, LIBMVFS_ERR_PARAM );

        return LIBMVFS_RET_FAILURE;
    }

    *pTaskId = MVFS_FD_NULL;

    /* リトライ上限まで繰り返す */
    do {
        /* タスクID取得 */
        ret = LibMkTaskNameGet( "VFS", pTaskId, &err );

        /* 取得結果判定 */
        if ( ret == MK_RET_SUCCESS ) {
            /* 成功 */

            break;
        }

        /* リトライ判定 */
        if ( ( err   == MK_ERR_NO_REGISTERED ) &&
             ( retry <= RETRY_MAX            )    ) {
            /* リトライ要 */

            /* ビジーウェイト */
            LibMkTimerSleep( RETRY_WAIT, NULL );
            /* リトライ回数更新 */
            retry++;
            /* リトライ */
            continue;

        } else {
            /* リトライ不要 */

            /* エラー番号設定 */
            MLIB_SET_IFNOT_NULL( pErrNo, LIBMVFS_ERR_NOT_FOUND );

            return LIBMVFS_RET_FAILURE;
        }

    } while ( true );

    return LIBMVFS_RET_SUCCESS;
}


/******************************************************************************/
