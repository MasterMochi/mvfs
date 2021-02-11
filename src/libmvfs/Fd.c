/******************************************************************************/
/*                                                                            */
/* src/libmvfs/Fd.c                                                           */
/*                                                                 2021/02/01 */
/* Copyright (C) 2019-2021 Mochi.                                             */
/*                                                                            */
/******************************************************************************/
/******************************************************************************/
/* インクルード                                                               */
/******************************************************************************/
/* 標準ヘッダ */
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* ライブラリヘッダ */
#include <libmvfs.h>
#include <MLib/MLib.h>
#include <MLib/MLibDynamicArray.h>

/* モジュール共通ヘッダ */
#include <mvfs.h>

/* モジュール内ヘッダ */
#include "Fd.h"


/******************************************************************************/
/* 定義                                                                       */
/******************************************************************************/
/** FDテーブルチャンク内エントリ数 */
#define FDTABLE_CHUNK_SIZE ( 32 )


/******************************************************************************/
/* ローカル関数宣言                                                           */
/******************************************************************************/
/* グローバル情報判定 */
static bool CheckGlobalFdInfo( uint_t  idx,
                               void    *pEntry,
                               va_list vaList   );
/* FDテーブル初期化 */
static void InitFdTable( void );


/******************************************************************************/
/* グローバル変数定義                                                         */
/******************************************************************************/
/** FDテーブル */
static MLibDynamicArray_t gFdTable;

/** 初期化済フラグ */
static bool gInit = false;


/******************************************************************************/
/* グローバル関数定義                                                         */
/******************************************************************************/
/******************************************************************************/
/**
 * @brief       FD割当て
 * @details     FDテーブルから未使用エントリを探してそのエントリを割当てる。
 *
 * @return      割り当てたFDテーブルエントリへのポインタを返す。
 * @retval      NULL     割当失敗
 * @retval      NULL以外 割当成功
 */
/******************************************************************************/
FdInfo_t *FdAlloc( void )
{
    uint32_t  localFd;  /* ローカルFD */
    FdInfo_t  *pFdInfo; /* FD情報     */
    MLibRet_t retMLib;  /* MLIB戻り値 */
    MLibErr_t errMLib;  /* MLIBエラー */

    /* 初期化 */
    localFd = 0;
    pFdInfo = NULL;
    retMLib = MLIB_RET_FAILURE;
    errMLib = MLIB_ERR_NONE;

    /* FDテーブル初期化判定 */
    if ( gInit == false  ) {
        /* 未初期化 */

        /* 初期化 */
        InitFdTable();
    }

    /* FD情報割当 */
    retMLib = MLibDynamicArrayAlloc( &gFdTable,
                                     &localFd,
                                     ( void ** ) &pFdInfo,
                                     &errMLib              );

    /* 割当結果判定 */
    if ( retMLib != MLIB_RET_SUCCESS ) {
        /* 失敗 */
        return NULL;
    }

    /* FD情報初期化 */
    memset( pFdInfo, 0, sizeof ( FdInfo_t ) );
    pFdInfo->localFd  = localFd;
    pFdInfo->writeIdx = 0;
    pFdInfo->readIdx  = 0;

    return pFdInfo;
}


/******************************************************************************/
/**
 * @brief       FD解放
 * @details     FDを解放する。
 *
 * @param[in]   localFd ローカルファイルディスクリプタ
 */
/******************************************************************************/
void FdFree( uint32_t localFd )
{
    /* FDテーブル初期化判定 */
    if ( gInit == false ) {
        /* 未初期化 */

        return;
    }

    /* FD情報解放 */
    MLibDynamicArrayFree( &gFdTable, localFd, NULL );

    return;
}


/******************************************************************************/
/**
 * @brief       グローバルFD情報取得
 * @details     FD管理テーブルからグローバルFDに該当するFD情報を取得する。
 *
 * @param[in]   globalFd グローバルファイルディスクリプタ
 *
 * @return      FD情報へのポインタを返す。
 * @retval      NULL     該当FD情報無し
 * @retval      NULL以外 該当FD情報
 */
/******************************************************************************/
FdInfo_t *FdGetGlobalFdInfo( uint32_t globalFd )
{
    uint32_t  localFd;  /* ローカルFD     */
    FdInfo_t  *pFdInfo; /* FD情報         */
    MLibRet_t retMLib;  /* MLIB戻り値     */
    MLibErr_t errMLib;  /* MLIBエラー要因 */

    /* 初期化 */
    localFd = 0;
    pFdInfo = NULL;
    retMLib = MLIB_RET_FAILURE;
    errMLib = MLIB_ERR_NONE;

    /* FDテーブル初期化判定 */
    if ( gInit == false ) {
        /* 未初期化 */

        return NULL;
    }

    /* FDテーブル検索 */
    retMLib = MLibDynamicArraySearch( &gFdTable,
                                      &localFd,
                                      ( void ** ) &pFdInfo,
                                      &errMLib,
                                      CheckGlobalFdInfo,
                                      globalFd              );

    /* 検索結果判定 */
    if ( retMLib != MLIB_RET_SUCCESS ) {
        /* 失敗 */

        return NULL;
    }

    return pFdInfo;
}


/******************************************************************************/
/**
 * @brief       FD情報取得
 * @details     FD管理テーブルからローカルFDに該当するFD情報を取得する。
 *
 * @param[in]   localFd ローカルファイルディスクリプタ
 *
 * @return      FD情報へのポインタを返す。
 * @retval      NULL     該当FD情報無し
 * @retval      NULL以外 該当FD情報
 */
/******************************************************************************/
FdInfo_t *FdGetLocalFdInfo( uint32_t localFd )
{
    FdInfo_t *pFdInfo;  /* FD情報         */
    MLibRet_t retMLib;  /* MLIB戻り値     */
    MLibErr_t errMLib;  /* MLIBエラー要因 */

    /* 初期化 */
    pFdInfo = NULL;
    retMLib = MLIB_RET_FAILURE;
    errMLib = MLIB_ERR_NONE;

    /* FDテーブル初期化判定 */
    if ( gInit == false ) {
        /* 未初期化 */

        return NULL;
    }

    /* FD情報取得 */
    retMLib = MLibDynamicArrayGet( &gFdTable,
                                   localFd,
                                   ( void ** ) &pFdInfo,
                                   &errMLib              );

    /* 取得結果判定 */
    if ( retMLib != MLIB_RET_SUCCESS ) {
        /* 失敗 */
        return NULL;
    }

    return pFdInfo;;
}


/******************************************************************************/
/* ローカル関数定義                                                           */
/******************************************************************************/
/******************************************************************************/
/**
 * @brief       グローバルFD情報判定
 * @details     FD情報が引数globalFdに一致するか判定する。
 *
 * @param[in]   idx     エントリインデックス
 * @param[in]   *pEntry エントリアドレス
 * @param[in]   vaList  可変長変数リスト
 *
 * @return      判定結果を返す。
 * @retval      true  一致
 * @retval      false 不一致
 */
/******************************************************************************/
static bool CheckGlobalFdInfo( uint_t  idx,
                               void    *pEntry,
                               va_list vaList   )
{
    uint32_t globalFd;  /* グローバルFD */
    FdInfo_t *pFdInfo;  /* FD情報       */

    /* 初期化 */
    globalFd = va_arg( vaList, uint32_t );
    pFdInfo  = ( FdInfo_t * ) pEntry;

    /* グローバルFD判定 */
    if ( pFdInfo->globalFd == globalFd ) {
        /* 一致 */

        return true;
    }

    return false;
}


/******************************************************************************/
/**
 * @brief       FDテーブル初期化
 * @details     FDテーブルを初期化する。
 */
/******************************************************************************/
static void InitFdTable( void )
{
    /* FDテーブル初期化 */
    MLibDynamicArrayInit( &gFdTable,
                          FDTABLE_CHUNK_SIZE,
                          sizeof ( FdInfo_t ),
                          LIBMVFS_FD_MAXNUM,
                          NULL                 );

    /* 初期化済 */
    gInit = true;

    return;
}


/******************************************************************************/
