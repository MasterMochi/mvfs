/******************************************************************************/
/*                                                                            */
/* src/mvfs/Node/Node.c                                                       */
/*                                                                 2020/05/28 */
/* Copyright (C) 2019-2020 Mochi.                                             */
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
#include <MLib/MLibSplit.h>

/* 外部モジュールヘッダ */
#include <mvfs.h>
#include <Debug.h>
#include <Node.h>


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
/* ノードリスト内ノード取得 */
static NodeInfo_t *GetInNode( NodeInfo_t *pNode,
                              const char *pName  );


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
 * @param[in]   *pName      ファイル名
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
NodeInfo_t *NodeCreate( const char *pName,
                        const char *pPath,
                        uint32_t   type,
                        MkTaskId_t mountTaskId )
{
    NodeInfo_t *pNode;  /* ノード */

    /* ノード割当 */
    pNode = ( NodeInfo_t * ) malloc( sizeof ( NodeInfo_t ) );

    /* 割当結果判定 */
    if ( pNode == NULL ) {
        /* 失敗 */

        DEBUG_LOG_ERR(
            "%s(): malloc(): size=%u",
            __func__,
            sizeof ( NodeInfo_t )
        );
        return NULL;
    }

    /* ノード初期化 */
    memset( pNode, 0, sizeof ( NodeInfo_t ) );

    /* ファイル名設定 */
    strncpy( pNode->name, pName, MVFS_NAME_MAXLEN );

    /* パス名設定 */
    strncpy( pNode->path, pPath, MVFS_PATH_MAXLEN );

    /* タイプ設定 */
    pNode->type = type;

    /* ready状態設定 */
    pNode->ready = MVFS_READY_READ | MVFS_READY_WRITE;

    /* マウントタスクID設定 */
    pNode->mountTaskId = mountTaskId;

    /* エントリリスト初期化 */
    MLibListInit( &( pNode->entryList ) );

    DEBUG_LOG_TRC(
        "%s(): pName=%s, pPath=%s, type=%#X, mountTaskId=%#X",
        __func__,
        pName,
        pPath,
        type,
        mountTaskId
    );

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
    pList = ( NodeList_t * )
            MLibListGetNextNode( &( pNode->entryList ), NULL );

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
 * @brief       ノード取得
 * @details     絶対パス(*pPath)に該当するノード情報を取得する。
 *
 * @param[in]   *pPath パス
 *
 * @return      ノード情報を返す。
 * @retval      NULL     該当ノード無し
 * @retval      NULL以外 ノード情報
 */
/******************************************************************************/
NodeInfo_t *NodeGet( const char *pPath )
{
    char              *pName;       /* ファイル名         */
    size_t            num;          /* パス分割数         */
    uint32_t          i;            /* カウンタ           */
    uint32_t          errNo;        /* エラー番号         */
    MLibRet_t         retMLib;      /* MLib戻り値         */
    NodeInfo_t        *pNode;       /* ノード    　       */
    MLibSplitHandle_t *pHandle;     /* 文字列分割ハンドル */

    /* 初期化 */
    pName   = NULL;
    num     = 0;
    i       = 0;
    errNo   = MLIB_SPLIT_ERR_NONE;
    retMLib = MLIB_FAILURE;
    pNode   = NULL;
    pHandle = NULL;

    /* 絶対パス判定 */
    if ( pPath[ 0 ] != '/' ) {
        /* 絶対パスでない */

        DEBUG_LOG_ERR( "%s(): invalid path: %s", __func__, pPath );
        return NULL;
    }

    /* ルートノード取得 */
    pNode = NodeGetRoot();

    /* パス文字列分割 */
    retMLib = MLibSplitInitByDelimiter( &pHandle,
                                        &pPath[ 1 ],
                                        strlen( pPath ),
                                        '/',
                                        &errNo           );

    /* 分割結果判定 */
    if ( retMLib != MLIB_SUCCESS ) {
        /* 失敗 */

        DEBUG_LOG_ERR(
            "%s(): MLibSplitInitByDelimiter(): ret=%d, err=%#X, pPath=%s",
            __func__,
            retMLib,
            errNo,
            pPath
        );
        return NULL;
    }

    /* 分割数取得 */
    retMLib = MLibSplitGetNum( pHandle, &num, &errNo );

    for ( i = 0; i < num; i++ ) {
        /* 文字列取得 */
        MLibSplitGet( pHandle, i, &pName, &errNo );

        /* 親ノードからノード取得 */
        pNode = GetInNode( pNode, pName );
    }

    /* 文字列分割ハンドル解放 */
    retMLib = MLibSplitTerm( &pHandle, &errNo );

    /* 解放結果判定 */
    if ( retMLib != MLIB_SUCCESS ) {
        /* 失敗 */

        DEBUG_LOG_ERR(
            "%s(): MLibSplitTerm(): ret=%d, err=%#X",
            __func__,
            retMLib,
            errNo
        );
    }

    return pNode;
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
    gpRootNode = NodeCreate( "", "/", NODE_TYPE_NORMAL_DIR, MK_TASKID_NULL );

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

        DEBUG_LOG_ERR(
            "%s(): malloc(): size=%u",
            __func__,
            sizeof ( NodeList_t )
        );
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

        DEBUG_LOG_ERR(
            "%s(): MLibListInsertTail(): ret=%d",
            __func__,
            retMLib
        );

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
/**
 * @brief       ノード内ノード取得
 * @details     ノードが持つノードリストからファイル名(pName)に該当するノードを
 *              取得する。
 *
 * @param[in]   *pName ファイル名
 *
 * @return      ノードを返す。
 * @retval      NULL     該当ノード無し
 * @retval      NULL以外 該当ノード
 */
/******************************************************************************/
static NodeInfo_t *GetInNode( NodeInfo_t *pNode,
                              const char *pName  )
{
    int        retCmp;  /* ファイル名比較結果 */
    uint32_t   i;       /* カウンタ           */
    NodeList_t *pList;  /* ノードリスト       */

    /* 初期化 */
    pList = NULL;

    /* リスト取得 */
    pList = ( NodeList_t * )
            MLibListGetNextNode( &( pNode->entryList ),
                                 ( MLibListNode_t * ) pList );

    while ( pList != NULL ) {
        /* リストエントリ毎に繰り返し */
        for ( i = 0; i < NODE_ENTRY_NUM; i++ ) {
            /* ノード有無判定 */
            if ( pList->pEntry[ i ] == NULL ) {
                /* ノード無し */

                continue;
            }

            /* ファイル名比較 */
            retCmp = strncmp( pName,
                              pList->pEntry[ i ]->name,
                              MVFS_NAME_MAXLEN          );

            /* 判定 */
            if ( retCmp == 0 ) {
                /* 一致 */

                return pList->pEntry[ i ];
            }
        }

        /* 次リスト取得 */
        pList = ( NodeList_t * )
                MLibListGetNextNode( &( pNode->entryList ),
                                     ( MLibListNode_t * ) pList );
    }

    return NULL;
}


/******************************************************************************/
