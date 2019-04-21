/******************************************************************************/
/*                                                                            */
/* src/mvfs/Node.c                                                            */
/*                                                                 2019/03/31 */
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
#include <kernel/config.h>
#include <kernel/types.h>

/* ライブラリヘッダ */
#include <MLib/MLibList.h>

/* モジュール共通ヘッダ */
#include <mvfs.h>

/* モジュールヘッダ */
#include "Node.h"


/******************************************************************************/
/* 定義                                                                       */
/******************************************************************************/


/******************************************************************************/
/* ローカル関数宣言                                                           */
/******************************************************************************/
/* エントリリスト作成 */
static NodeList_t *CreateList( NodeInfo_t *pNode );
/* エントリリスト削除 */
static void DeleteList( NodeList_t *pList );


/******************************************************************************/
/* 静的グローバル変数定義                                                     */
/******************************************************************************/
/** ルートノード情報 */
static NodeInfo_t *gpRootNode;


/******************************************************************************/
/* グローバル関数定義                                                         */
/******************************************************************************/
/******************************************************************************/
/**
 * @brief       ノードエントリ追加
 * @details     ノードのエントリにノードを追加する。
 *
 * @param[in]   *pNode    ノード
 * @param[in]   *pAddNode 追加ノード
 *
 * @return      処理結果を返す。
 * @retval      MVFS_OK 成功
 * @retval      MVFS_NG 失敗
 */
/******************************************************************************/
int32_t NodeAddEntry( NodeInfo_t *pNode,
                      NodeInfo_t *pAddEntry )
{
    uint32_t   idx;     /* インデックス   */
    NodeList_t *pList;  /* エントリリスト */

    /* 先頭エントリリスト取得 */
    pList = ( NodeList_t * )
            MLibListGetNextNode( &( pNode->entryList ), NULL );

    /* 取得結果判定 */
    if ( pList == NULL ) {
        /* エントリリスト無し */

        /* エントリリスト作成 */
        pList = CreateList( pNode );
    }

    /* リスト毎に繰り返す */
    while ( pList != NULL ) {
        /* リスト内のエントリ毎に繰り返す */
        for ( idx = 0; idx < NODE_ENTRY_NUM; idx++ ) {
            /* 未使用エントリ判定 */
            if ( pList->pEntry[ idx ] == NULL ) {
                /* 未使用 */

                /* エントリ設定 */
                pList->pEntry[ idx ] = pAddEntry;
                return MVFS_OK;
            }
        }

        /* 次エントリリスト取得 */
        pList = ( NodeList_t * )
                MLibListGetNextNode( &( pNode->entryList ),
                                     ( MLibListNode_t * ) pList );

        /* 取得結果判定 */
        if ( pList == NULL ) {
            /* エントリリスト無し */

            /* エントリリスト作成 */
            pList = CreateList( pNode );
        }
    }

    return MVFS_NG;
}


/******************************************************************************/
/**
 * @brief       ノード作成
 * @details     新しくノードを作成する。
 *
 * @param[in]   *pPath      パス
 * @param[in]   type        タイプ
 *                  - NODE_TYPE_NORMAL_DIR 通常ディレクトリ
 *                  - NODE_TYPE_MOUNT_FILE マウントディレクトリ
 * @param[in]   mountTaskId マウントタスクID
 *
 * @return      作成したノードを返す。
 * @retval      NULL     失敗
 * @retval      NULL以外 成功(作成したノード)
 */
/******************************************************************************/
NodeInfo_t *NodeCreate( const char *pPath,
                        uint32_t   type,
                        MkTaskId_t mountTaskId )
{
    NodeInfo_t *pNode;  /* ノード */

    /* ノード割当 */
    pNode = ( NodeInfo_t * ) malloc( sizeof ( NodeInfo_t ) );

    /* 割当結果判定 */
    if ( pNode == NULL ) {
        /* 失敗 */

        return NULL;
    }

    /* ノード初期化 */
    memset( pNode, 0, sizeof ( NodeInfo_t ) );

    /* パス名設定 */
    strncpy( pNode->path, pPath, MVFS_PATH_MAXLEN );

    /* タイプ設定 */
    pNode->type = type;

    /* マウントタスクID設定 */
    pNode->mountTaskId = mountTaskId;

    /* エントリリスト初期化 */
    MLibListInit( &( pNode->entryList ) );

    return pNode;
}


/******************************************************************************/
/**
 * @brief       ノード削除
 * @details     ノード情報を削除する。
 *
 * @param[in]   *pNode ノード情報
 */
/******************************************************************************/
void NodeDelete( NodeInfo_t *pNode )
{
    NodeList_t *pList;      /* エントリリスト   */
    NodeList_t *pNextList;  /* 次エントリリスト */

    /* エントリリスト取得 */
    pList = ( NodeList_t * ) MLibListGetNextNode( &( pNode->entryList ), NULL );

    /* エントリリスト毎に繰り返す */
    while ( pList != NULL ) {
        /* 次エントリリスト取得 */
        pNextList = ( NodeList_t * )
                    MLibListGetNextNode( &( pNode->entryList ),
                                         ( MLibListNode_t * ) pList );

        /* エントリリスト解放 */
        DeleteList( pList );

        /* 次エントリリストへ */
        pList = pNextList;
    }

    /* ノード情報解放 */
    free( pNode );

    return;
}


/******************************************************************************/
/**
 * @brief       ルートノード取得
 * @details     ルートディレクトリのノード情報を取得する。
 *
 * @return      ルートディレクトリのノード情報を返す。
 */
/******************************************************************************/
NodeInfo_t *NodeGetRoot( void )
{
    return gpRootNode;
}


/******************************************************************************/
/**
 * @brief   ノード初期化
 * @details ルートディレクトリノード情報を作成する。
 */
/******************************************************************************/
void NodeInit( void )
{
    /* ルートノード作成 */
    gpRootNode = NodeCreate( "/",
                             NODE_TYPE_NORMAL_DIR,
                             MK_CONFIG_TASKID_NULL );

    return;
}


/******************************************************************************/
/* ローカル関数定義                                                           */
/******************************************************************************/
/******************************************************************************/
/**
 * @brief       エントリリスト作成
 * @details     ノードに新しいエントリリストを作成する。
 *
 * @param[in]   *pNode ノード
 *
 * @return      作成したエントリリストを返す。
 * @retval      NULL     失敗
 * @retval      NULL以外 成功(作成したエントリリスト)
 */
/******************************************************************************/
static NodeList_t *CreateList( NodeInfo_t *pNode )
{
    MLibRet_t  retMLib; /* MLib関数戻り値 */
    NodeList_t *pList;  /* エントリリスト */

    /* エントリリスト作成 */
    pList = ( NodeList_t * ) malloc( sizeof ( NodeList_t ) );

    /* 作成結果判定 */
    if ( pList == NULL ) {
        /* 失敗 */

        return NULL;
    }

    /* エントリリスト初期化 */
    memset( pList, 0, sizeof ( NodeList_t ) );

    /* エントリリスト追加 */
    retMLib = MLibListInsertTail( &( pNode->entryList ),
                                  ( MLibListNode_t * ) pList );

    /* 追加結果判定 */
    if ( retMLib != MLIB_SUCCESS ) {
        /* 失敗 */

        /* エントリリスト解放 */
        free( pList );

        return NULL;
    }

    return pList;
}


/******************************************************************************/
/**
 * @brief       エントリリスト削除
 * @details     エントリリストを削除する。
 *
 * @param[in]   *pList エントリリスト
 */
/******************************************************************************/
static void DeleteList( NodeList_t *pList )
{
    /* エントリリスト解放 */
    free( pList );

    return;
}


/******************************************************************************/
