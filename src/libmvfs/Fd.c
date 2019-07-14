/******************************************************************************/
/*                                                                            */
/* src/libmvfs/Fd.c                                                           */
/*                                                                 2019/07/11 */
/* Copyright (C) 2019 Mochi.                                                  */
/*                                                                            */
/******************************************************************************/
/******************************************************************************/
/* インクルード                                                               */
/******************************************************************************/
/* 標準ヘッダ */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* ライブラリヘッダ */
#include <MLib/MLibList.h>

/* モジュール共通ヘッダ */
#include <mvfs.h>

/* モジュール内ヘッダ */
#include "Fd.h"


/******************************************************************************/
/* 定義                                                                       */
/******************************************************************************/
/** FDテーブルエントリ最小インデックス */
#define FDTABLE_ENTRY_IDX_MIN (  0 )
/** FDテーブルエントリ最大インデックス */
#define FDTABLE_ENTRY_IDX_MAX ( 32 )
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
static MLibList_t gFdTableList = { 0 };


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
    uint32_t  idx;          /* インデックス */
    FdTable_t *pFdTable;    /* FDテーブル   */

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
                pFdTable->fdInfo[ idx ].used     = true;
                pFdTable->fdInfo[ idx ].writeIdx = 0;
                pFdTable->fdInfo[ idx ].readIdx  = 0;

                return &( pFdTable->fdInfo[ idx ] );
            }
        }
    }

    return NULL;
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
    /* 未使用フラグ設定 */
    pFdInfo->used = false;

    return;
}


/******************************************************************************/
/**
 * @brief       FD情報取得
 * @details     FD管理テーブルから指定したFDに該当するFD情報を取得する。
 *
 * @param[in]   localFd ローカルファイルディスクリプタ
 *
 * @return      FD情報へのポインタを返す。
 * @retval      NULL     該当FD情報無し
 * @retval      NULL以外 該当FD情報
 */
/******************************************************************************/
FdInfo_t *FdGet( uint32_t localFd )
{
    uint32_t  idx;          /* インデックス */
    FdTable_t *pFdTable;    /* FDテーブル   */

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
            if ( pFdTable->fdInfo[ idx ].localFd == localFd ) {
                /* 該当 */

                return &( pFdTable->fdInfo[ idx ] );
            }
        }
    }

    return NULL;
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
    uint32_t  localFd;  /* ローカルFD       */
    uint32_t  idx;      /* インデックス     */
    FdTable_t *pLast;   /* 最後尾FDテーブル */
    FdTable_t *pAdd;    /* 追加FDテーブル   */

    /* 初期化 */
    localFd = 0;
    idx     = 0;
    pLast   = NULL;
    pAdd    = NULL;

    /* リスト最後尾FDテーブル取得 */
    pLast = ( FdTable_t * ) MLibListGetPrevNode( &gFdTableList, NULL );

    /* 取得結果判定 */
    if ( pLast == NULL ) {
        /* FDテーブル無し */

        /* ローカルFD初期化 */
        localFd = MVFS_FD_NULL;

    } else {
        /* FDテーブル有り */

        /* 最老番ローカルFD取得 */
        localFd = pLast->fdInfo[ FDTABLE_ENTRY_IDX_MAX ].globalFd;
    }

    /* FDテーブル割当 */
    pAdd = malloc( sizeof ( FdTable_t ) );

    /* 割当結果判定 */
    if ( pAdd == NULL ) {
        /* 失敗 */

        return NULL;
    }

    /* FDテーブル初期化 */
    memset( pAdd, 0, sizeof ( FdTable_t ) );

    /* FDテーブルエントリ毎に繰り返し */
    for ( idx = FDTABLE_ENTRY_IDX_MIN; idx <= FDTABLE_ENTRY_IDX_MAX; idx++ ) {
        pAdd->fdInfo[ idx ].used    = false;
        pAdd->fdInfo[ idx ].localFd = ++localFd;
    }

    /* リスト最後尾挿入 */
    MLibListInsertTail( &gFdTableList, ( MLibListNode_t * ) pAdd );

    return pAdd;
}


/******************************************************************************/

