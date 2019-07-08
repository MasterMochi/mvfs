/******************************************************************************/
/*                                                                            */
/* src/mvfs/Node.h                                                            */
/*                                                                 2019/06/24 */
/* Copyright (C) 2019 Mochi.                                                  */
/*                                                                            */
/******************************************************************************/
#ifndef NODE_H
#define NODE_H
/******************************************************************************/
/* インクルード                                                               */
/******************************************************************************/
/* 標準ヘッダ */
#include <stdint.h>

/* カーネルヘッダ */
#include <kernel/types.h>

/* ライブラリヘッダ */
#include <MLib/MLibList.h>

/* モジュール共通ヘッダ */
#include <mvfs.h>


/******************************************************************************/
/* 定義                                                                       */
/******************************************************************************/
/* ノードタイプ */
#define NODE_TYPE_NORMAL_DIR ( 1 )  /**< 通常ディレクトリ */
#define NODE_TYPE_MOUNT_FILE ( 2 )  /**< マウントファイル */

/** ノードリストエントリ数 */
#define NODE_ENTRY_NUM ( 128 )

/** ノード情報 */
typedef struct {
    char       name[ MVFS_NAME_MAXLEN + 1 ];    /**< 名前             */
    char       path[ MVFS_PATH_MAXLEN + 1 ];    /**< パス             */
    uint32_t   type;                            /**< タイプ           */
    MLibList_t entryList;                       /**< エントリリスト   */
    MkTaskId_t mountTaskId;                     /**< マウントタスクID */
} NodeInfo_t;

/** ノードリスト */
typedef struct {
    MLibListNode_t link;                            /**< リンクリスト情報 */
    NodeInfo_t     *pEntry[ NODE_ENTRY_NUM + 1 ];   /**< エントリ         */
} NodeList_t;


/******************************************************************************/
/* グローバル関数宣言                                                         */
/******************************************************************************/
/* ノードエントリ追加 */
extern int32_t NodeAddEntry( NodeInfo_t *pNode,
                             NodeInfo_t *pAddEntry );
/* ノード作成 */
extern NodeInfo_t *NodeCreate( const char *pName,
                               const char *pPath,
                               uint32_t   type,
                               MkTaskId_t mountTaskId );
/* ノード解放 */
extern void NodeDelete( NodeInfo_t *pNode );
/* ノード取得 */
extern NodeInfo_t *NodeGet( const char *pPath );
/* ルートノード取得 */
extern NodeInfo_t *NodeGetRoot( void );
/* ノード初期化 */
extern void NodeInit( void );


/******************************************************************************/
#endif
