/******************************************************************************/
/*                                                                            */
/* src/mvfs/Fd/Fd.c                                                           */
/*                                                                 2019/10/13 */
/* Copyright (C) 2019 Mochi.                                                  */
/*                                                                            */
/******************************************************************************/
/******************************************************************************/
/* インクルード                                                               */
/******************************************************************************/
/* 標準ヘッダ */
#include <limits.h>
#include <stdint.h>
#include <string.h>

/* カーネルヘッダ */
#include <kernel/types.h>

/* ライブラリヘッダ */
#include <MLib/MLibDynamicArray.h>

/* 外部モジュールヘッダ */
#include <Debug.h>
#include <Fd.h>


/******************************************************************************/
/* 定義                                                                       */
/******************************************************************************/
/** FDテーブルチャンク内エントリ数 */
#define FDTABLE_CHUNK_SIZE ( 255 )
/** FDテーブルエントリ数 */
#define FDTABLE_ENTRY_NUM  ( UINT_MAX )


/******************************************************************************/
/* 静的グローバル変数定義                                                     */
/******************************************************************************/
/** FDテーブル */
static MLibDynamicArray_t *pgFdTable;


/******************************************************************************/
/* グローバル関数定義                                                         */
/******************************************************************************/
/******************************************************************************/
/**
 * @brief       FD割当て
 * @details     FDテーブルから未使用エントリを探してそのエントリを割当てる。
 *
 * @param[in]   localFd ローカルFD
 * @param[in]   *pNode  ノード
 *
 * @return      割り当てたグローバルFD情報を返す。
 * @retval      MVFS_FD_NULL     割当失敗
 * @retval      MVFS_FD_NULL以外 割当成功(グローバルFD)
 */
/******************************************************************************/
FdInfo_t *FdAlloc( uint32_t   localFd,
                   NodeInfo_t *pNode   )
{
    uint32_t  globalFd;     /* グローバルFD       */
    MLibErr_t errMLib;      /* MLIB関数エラー要因 */
    MLibRet_t retMLib;      /* MLIB関数戻り値     */
    FdInfo_t  *pFdInfo;     /* FD情報             */

    /* 初期化 */
    retMLib = MLIB_RET_FAILURE;
    errMLib = MLIB_ERR_NONE;
    pFdInfo = NULL;

    /* FDエントリ割当 */
    retMLib = MLibDynamicArrayAlloc( pgFdTable,
                                     &globalFd,
                                     ( void ** ) &pFdInfo,
                                     &errMLib              );

    /* 割当結果判定 */
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

    /* エントリ割当て */
    pFdInfo->globalFd = globalFd;
    pFdInfo->localFd  = localFd;
    pFdInfo->pNode    = pNode;

    return pFdInfo;
}


/******************************************************************************/
/**
 * @brief       FD解放
 * @details     FDを解放する。
 *
 * @param[in]   globalFd グローバルファイルディスクリプタ
 */
/******************************************************************************/
void FdFree( uint32_t globalFd )
{
    /* FD情報解放 */
    MLibDynamicArrayFree( pgFdTable, globalFd, NULL );

    return;
}


/******************************************************************************/
/**
 * @brief       FD情報取得
 * @details     FD管理テーブルから指定したFDに該当するFD情報を取得する。
 *
 * @param[in]   globalFd グローバルファイルディスクリプタ
 *
 * @return      FD情報へのポインタを返す。
 * @retval      NULL     該当FD情報無し
 * @retval      NULL以外 該当FD情報
 */
/******************************************************************************/
FdInfo_t *FdGet( uint32_t globalFd )
{
    FdInfo_t  *pFdInfo; /* FD情報         */
    MLibRet_t retMLib;  /* MLIB戻り値     */
    MLibErr_t errMLib;  /* MLIBエラー要因 */

    /* 初期化 */
    pFdInfo = NULL;
    retMLib = MLIB_RET_FAILURE;
    errMLib = MLIB_ERR_NONE;

    /* FD情報取得 */
    retMLib = MLibDynamicArrayGet( pgFdTable,
                                   globalFd,
                                   ( void ** ) &pFdInfo,
                                   &errMLib              );

    /* 取得結果判定 */
    if ( retMLib != MLIB_RET_SUCCESS ) {
        /* 失敗 */
        DEBUG_LOG_ERR(
            "%s(): MLibDynamicArrayGet(): ret=%d, err=%#X",
            retMLib,
            errMLib
        );
    }

    return pFdInfo;
}


/******************************************************************************/
/**
 * @brief       FD管理初期化
 * @details     FD管理テーブルを初期化する。
 */
/******************************************************************************/
void FdInit( void )
{
    MLibRet_t retMLib;  /* MLIB戻り値     */
    MLibErr_t errMLib;  /* MLIBエラー要因 */

    /* 初期化 */
    retMLib = MLIB_RET_FAILURE;
    errMLib = MLIB_ERR_NONE;

    /* FDテーブル初期化 */
    retMLib = MLibDynamicArrayInit( &pgFdTable,
                                    FDTABLE_CHUNK_SIZE,
                                    sizeof ( FdInfo_t ),
                                    FDTABLE_ENTRY_NUM,
                                    &errMLib             );

    /* 初期化結果判定 */
    if ( retMLib != MLIB_RET_SUCCESS ) {
        /* 失敗 */

        DEBUG_LOG_ERR(
            "%(): MLibDynamicArrayInit(): ret=%d, err=%#X",
            __func__,
            retMLib,
            errMLib
        );
        DEBUG_ABORT();
    }

    return;
}


/******************************************************************************/
