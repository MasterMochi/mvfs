/******************************************************************************/
/*                                                                            */
/* src/mvfs/Fd.c                                                              */
/*                                                                 2019/09/03 */
/* Copyright (C) 2019 Mochi.                                                  */
/*                                                                            */
/******************************************************************************/
/******************************************************************************/
/* インクルード                                                               */
/******************************************************************************/
/* 標準ヘッダ */
#include <stdlib.h>
#include <string.h>

/* カーネルヘッダ */
#include <kernel/types.h>

/* ライブラリヘッダ */
#include <MLib/MLibList.h>

/* モジュール内ヘッダ */
#include "Debug.h"
#include "Fd.h"


/******************************************************************************/
/* 定義                                                                       */
/******************************************************************************/
/** FDテーブルエントリ最小インデックス */
#define FDTABLE_ENTRY_IDX_MIN (   0 )
/** FDテーブルエントリ最大インデックス */
#define FDTABLE_ENTRY_IDX_MAX ( 254 )
/** FDテーブルエントリ数 */
#define FDTABLE_ENTRY_NUM     ( FDTABLE_ENTRY_IDX_MAX + 1 )

/** FDテーブル */
typedef struct {
    MLibListNode_t link;                        /**< リンクリスト情報 */
    FdInfo_t       fdInfo[ FDTABLE_ENTRY_NUM ]; /**< FD情報           */
} FdTable_t;


/******************************************************************************/
/* ローカル関数宣言                                                           */
/******************************************************************************/
/* FDテーブル追加 */
static FdTable_t *AddFdTable( void );


/******************************************************************************/
/* 静的グローバル変数定義                                                     */
/******************************************************************************/
/** FDテーブルリスト */
static MLibList_t gFdTableList;


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
    uint32_t  idx;          /* インデックス */
    FdTable_t *pFdTable;    /* FDテーブル   */

    DEBUG_LOG_FNC(
        "%s(): start. localFd=%u, pid=%u, pNode=%p",
        __func__,
        localFd,
        pid,
        pNode
    );

    /* 初期化 */
    pFdTable = NULL;

    while ( true ) {
        /* FDテーブル取得 */
        pFdTable = ( FdTable_t * )
                   MLibListGetNextNode( &gFdTableList,
                                        ( MLibListNode_t * ) pFdTable );

        /* 取得結果判定 */
        if ( pFdTable == NULL ) {
            /* FDテーブル無し */

            /* FDテーブル追加 */
            pFdTable = AddFdTable();

            /* 追加結果判定 */
            if ( pFdTable == NULL ) {
                /* 失敗 */

                DEBUG_LOG_ERR( "AddFdTable()" );
                break;
            }
        }

        /* FDテーブルエントリ毎に繰り返し */
        for ( idx  = FDTABLE_ENTRY_IDX_MIN;
              idx <= FDTABLE_ENTRY_IDX_MAX;
              idx++                         ) {
            /* エントリ未使用チェック */
            if ( pFdTable->fdInfo[ idx ].used == false ) {
                /* 未使用 */

                /* エントリ割当て */
                pFdTable->fdInfo[ idx ].used    = true;
                pFdTable->fdInfo[ idx ].localFd = localFd;
                pFdTable->fdInfo[ idx ].pid     = pid;
                pFdTable->fdInfo[ idx ].pNode   = pNode;

                DEBUG_LOG_FNC(
                    "%s(): end. ret=%u",
                    __func__,
                    pFdTable->fdInfo[ idx ].globalFd
                );
                return pFdTable->fdInfo[ idx ].globalFd;
            }
        }
    }

    DEBUG_LOG_FNC( "%s(): end. ret=%u", __func__, MVFS_FD_NULL );
    return MVFS_FD_NULL;
}


/******************************************************************************/
/**
 * @brief       FD解放
 * @details     FDを解放する。
 *
 * @param[in]   *pFdInfo FD情報
 */
/******************************************************************************/
void FdFree( FdInfo_t *pFdInfo )
{
    DEBUG_LOG_FNC( "%s(): start. pFdInfo=%p", __func__, pFdInfo );

    /* FD情報初期化 */
    pFdInfo->used    = false;
    pFdInfo->localFd = MVFS_FD_NULL;
    pFdInfo->pid     = MK_PID_NULL;
    pFdInfo->pNode   = NULL;

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
    uint32_t  idx;          /* インデックス */
    FdTable_t *pFdTable;    /* FDテーブル   */

    DEBUG_LOG_FNC( "%s(): start. globalFd=%u", __func__, globalFd );

    /* 初期化 */
    pFdTable = NULL;

    /* FDテーブルエントリ毎に繰り返し */
    while ( true ) {
        /* FDテーブル取得 */
        pFdTable = ( FdTable_t * )
                   MLibListGetNextNode( &gFdTableList,
                                        ( MLibListNode_t * ) pFdTable );

        /* 取得結果判定 */
        if ( pFdTable == NULL ) {
            /* FDテーブル無し */

            break;
        }

        /* FD情報毎に繰り返し */
        for ( idx  = FDTABLE_ENTRY_IDX_MIN;
              idx <= FDTABLE_ENTRY_IDX_MAX;
              idx++                         ) {
            /* グローバルFD比較 */
            if ( pFdTable->fdInfo[ idx ].globalFd == globalFd ) {
                /* 該当 */

                DEBUG_LOG_FNC(
                    "%s(): end. ret=%p",
                    __func__,
                    &( pFdTable->fdInfo[ idx ] )
                );
                return &( pFdTable->fdInfo[ idx ] );
            }
        }
    }

    DEBUG_LOG_FNC( "%s(): end. ret=%p", __func__, NULL );
    return NULL;
}


/******************************************************************************/
/**
 * @brief       FD管理初期化
 * @details     FD管理テーブルを初期化する。
 */
/******************************************************************************/
void FdInit( void )
{
    /* FDテーブルリスト初期化 */
    MLibListInit( &gFdTableList );

    return;
}


/******************************************************************************/
/* ローカル関数定義                                                           */
/******************************************************************************/
/******************************************************************************/
/**
 * @brief       FDテーブル追加
 * @details     FDテーブルリストに新しいFDテーブルを追加する。
 *
 * @return      追加したFDテーブルへのポインタを返す。
 * @retval      NULL     追加失敗
 * @retval      NULL以外 追加成功
 */
/******************************************************************************/
static FdTable_t *AddFdTable( void )
{
    uint32_t  globalFd; /* グローバルFD     */
    uint32_t  idx;      /* インデックス     */
    FdTable_t *pLast;   /* 最後尾FDテーブル */
    FdTable_t *pAdd;    /* 追加FDテーブル   */

    DEBUG_LOG_FNC( "%s(): start.", __func__ );

    /* 初期化 */
    globalFd = 0;
    idx      = 0;
    pLast    = NULL;
    pAdd     = NULL;

    /* リスト最後尾FDテーブル取得 */
    pLast = ( FdTable_t * ) MLibListGetPrevNode( &gFdTableList, NULL );

    /* 取得結果判定 */
    if ( pLast == NULL ) {
        /* FDテーブル無し */

        globalFd = MVFS_FD_NULL;

    } else {
        /* FDテーブル有り */

        /* 最老番グローバルFD取得 */
        globalFd = pLast->fdInfo[ FDTABLE_ENTRY_IDX_MAX ].globalFd;
    }

    /* FDテーブル割当 */
    pAdd = malloc( sizeof ( FdTable_t ) );

    /* 割当結果判定 */
    if ( pAdd == NULL ) {
        /* 失敗 */

        DEBUG_LOG_ERR( "malloc(): %d", sizeof ( FdTable_t ) );
        DEBUG_LOG_FNC( "%s(): end. ret=%p", __func__, NULL );
        return NULL;
    }

    /* FDテーブル初期化 */
    memset( pAdd, 0, sizeof ( FdTable_t ) );

    /* FDテーブルエントリ毎に繰り返し */
    for ( idx = FDTABLE_ENTRY_IDX_MIN; idx <= FDTABLE_ENTRY_IDX_MAX; idx++ ) {
        pAdd->fdInfo[ idx ].used     = false;
        pAdd->fdInfo[ idx ].globalFd = ++globalFd;
    }

    /* リスト最後尾挿入 */
    MLibListInsertTail( &gFdTableList, ( MLibListNode_t * ) pAdd );

    DEBUG_LOG_FNC( "%s(): end. ret=%p", __func__, pAdd );
    return pAdd;
}


/******************************************************************************/
