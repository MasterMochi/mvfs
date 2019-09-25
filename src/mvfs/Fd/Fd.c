/******************************************************************************/
/*                                                                            */
/* src/mvfs/Fd/Fd.c                                                           */
/*                                                                 2019/09/24 */
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
 * @param[in]   pid     割当PID
 * @param[in]   *pNode  ノード
 *
 * @return      割り当てたグローバルFDを返す。
 * @retval      MVFS_FD_NULL     割当失敗
 * @retval      MVFS_FD_NULL以外 割当成功(グローバルFD)
 */
/******************************************************************************/
uint32_t FdAlloc( uint32_t   localFd,
                  MkPid_t    pid,
                  NodeInfo_t *pNode   )
{
    uint32_t  globalFd;     /* グローバルFD       */
    MLibErr_t err;          /* MLIB関数エラー要因 */
    MLibRet_t retMLib;      /* MLIB関数戻り値     */
    FdInfo_t  *pFdInfo;     /* FD情報             */

    DEBUG_LOG_FNC(
        "%s(): start. localFd=%u, pid=%u, pNode=%p",
        __func__,
        localFd,
        pid,
        pNode
    );

    /* FDエントリ割当 */
    retMLib = MLibDynamicArrayAlloc( pgFdTable,
                                     &globalFd,
                                     ( void ** ) &pFdInfo,
                                     &err                  );

    /* 割当結果判定 */
    if ( retMLib != MLIB_RET_SUCCESS ) {
        /* 失敗 */
        DEBUG_LOG_ERR( "MLibDynamicArrayAlloc() error: 0x%X", err );
        DEBUG_LOG_FNC( "%s(): end. ret=%u", __func__, MVFS_FD_NULL );
        return MVFS_FD_NULL;
    }

    /* エントリ割当て */
    pFdInfo->localFd = localFd;
    pFdInfo->pid     = pid;
    pFdInfo->pNode   = pNode;

    DEBUG_LOG_FNC( "%s(): end. ret=%u", __func__, globalFd );
    return globalFd;
}


/******************************************************************************/
/**
 * @brief       FD解放
 * @details     FDを解放する。
 *
 * @param[in]   globalFd グローバルFD
 */
/******************************************************************************/
void FdFree( uint32_t globalFd )
{
    DEBUG_LOG_FNC( "%s(): start. globalFd=0x%X", __func__, globalFd );

    /* FD情報解放 */
    MLibDynamicArrayFree( pgFdTable, globalFd, NULL );

    DEBUG_LOG_FNC( "%s(): end.", __func__ );
    return;
}


/******************************************************************************/
/**
 * @brief       FD情報取得
 * @details     FD管理テーブルから指定したFDに該当するFD情報を取得する。
 *
 * @param[in]   fd ファイルディスクリプタ
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
    MLibErr_t err;      /* MLIBエラー要因 */

    DEBUG_LOG_FNC( "%s(): start. globalFd=%u", __func__, globalFd );

    /* 初期化 */
    pFdInfo = NULL;
    retMLib = MLIB_RET_FAILURE;
    err     = MLIB_ERR_NONE;

    /* FD情報取得 */
    retMLib = MLibDynamicArrayGet( pgFdTable,
                                   globalFd,
                                   ( void ** ) &pFdInfo,
                                   &err                  );

    /* 取得結果判定 */
    if ( retMLib != MLIB_RET_SUCCESS ) {
        /* 失敗 */
        DEBUG_LOG_ERR( "MLibDynamicArrayGet() error: 0x%X", err );
    }

    DEBUG_LOG_FNC( "%s(): end. ret=%p", __func__, pFdInfo );
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
    MLibErr_t err;      /* MLIBエラー要因 */

    /* 初期化 */
    retMLib = MLIB_RET_FAILURE;
    err     = MLIB_ERR_NONE;

    /* FDテーブル初期化 */
    retMLib = MLibDynamicArrayInit( &pgFdTable,
                                    FDTABLE_CHUNK_SIZE,
                                    sizeof ( FdInfo_t ),
                                    FDTABLE_ENTRY_NUM,
                                    &err                 );

    /* 初期化結果判定 */
    if ( retMLib != MLIB_RET_SUCCESS ) {
        /* 失敗 */
        DEBUG_LOG_ERR( "MLibDynamicArrayInit() error: 0x%X", err );
        DEBUG_ABORT();
    }

    return;
}


/******************************************************************************/
