/******************************************************************************/
/*                                                                            */
/* src/include/mvfs.h                                                         */
/*                                                                 2019/07/06 */
/* Copyright (C) 2018-2019 Mochi.                                             */
/*                                                                            */
/******************************************************************************/
#ifndef MVFS_H
#define MVFS_H
/******************************************************************************/
/* インクルード                                                               */
/******************************************************************************/
/* 標準ヘッダ */
#include <stddef.h>
#include <stdint.h>

/* カーネルヘッダ */
#include <kernel/types.h>


/******************************************************************************/
/* 定義                                                                       */
/******************************************************************************/
/* 機能ID */
#define MVFS_FUNCID_MOUNT   ( 0 )  /**< mount        */
#define MVFS_FUNCID_OPEN    ( 1 )  /**< open         */
#define MVFS_FUNCID_VFSOPEN ( 2 )  /**< vfsOpen      */
#define MVFS_FUNCID_MAX     ( 2 )  /**< 機能ID最大値 */
#define MVFS_FUNCID_NUM     \
    ( MVFS_FUNCID_MAX + 1 )        /**< 機能数       */

/* タイプ */
#define MVFS_TYPE_REQ  ( 0 )    /**< 要求 */
#define MVFS_TYPE_RESP ( 1 )    /**< 応答 */

/** パス長(NULL文字含まず) */
#define MVFS_PATH_MAXLEN ( 1023 )

/** ファイル名長(NULL文字含まず) */
#define MVFS_NAME_MAXLEN ( 255 )

/* 処理結果 */
#define MVFS_RESULT_SUCCESS ( 0 )   /**< 成功 */
#define MVFS_RESULT_FAILURE ( 1 )   /**< 失敗 */

/* 戻り値 */
#define MVFS_OK (  0 )  /**< 正常終了 */
#define MVFS_NG ( -1 )  /**< 異常終了 */

/** FD無効値 */
#define MVFS_FD_NULL ( 0xFFFFFFFF )

/** メッセージヘッダ */
typedef struct {
    uint32_t funcId;    /**< 機能ID */
    uint32_t type;      /**< タイプ */
} MvfsMsgHdr_t;

/** mount要求メッセージ */
typedef struct {
    MvfsMsgHdr_t header;                        /**< メッセージヘッダ */
    char         path[ MVFS_PATH_MAXLEN + 1 ];  /**< パス             */
} MvfsMsgMountReq_t;


/** mount応答メッセージ */
typedef struct {
    MvfsMsgHdr_t header;    /**< メッセージヘッダ */
    uint32_t     result;    /**< 処理結果         */
} MvfsMsgMountResp_t;


/** open要求メッセージ */
typedef struct {
    MvfsMsgHdr_t header;                        /**< メッセージヘッダ */
    uint32_t     localFd;                       /**< ローカルFD       */
    char         path[ MVFS_PATH_MAXLEN + 1 ];  /**< 絶対パス(\0含む) */
} MvfsMsgOpenReq_t;

/** open応答メッセージ */
typedef struct {
    MvfsMsgHdr_t header;    /**< メッセージヘッダ */
    uint32_t     result;    /**< 処理結果         */
    uint32_t     globalFd;  /**< グローバルFD     */
} MvfsMsgOpenResp_t;

/** vfsOpen要求メッセージ */
typedef struct {
    MvfsMsgHdr_t header;                        /**< メッセージヘッダ */
    MkPid_t      pid;                           /**< プロセスID       */
    uint32_t     globalFd;                      /**< グローバルFD     */
    char         path[ MVFS_PATH_MAXLEN + 1 ];  /**< 絶対パス(\0含む) */
} MvfsMsgVfsOpenReq_t;

/** vfsOpen応答メッセージ */
typedef struct {
    MvfsMsgHdr_t header;    /**< メッセージヘッダ */
    uint32_t     result;    /**< 処理結果         */
} MvfsMsgVfsOpenResp_t;


/******************************************************************************/
#endif
