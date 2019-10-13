/******************************************************************************/
/*                                                                            */
/* src/include/mvfs.h                                                         */
/*                                                                 2019/10/07 */
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
#define MVFS_FUNCID_MOUNT    ( 0x00000000 ) /**< Mount        */
#define MVFS_FUNCID_OPEN     ( 0x00000001 ) /**< Open         */
#define MVFS_FUNCID_VFSOPEN  ( 0x00000002 ) /**< VfsOpen      */
#define MVFS_FUNCID_WRITE    ( 0x00000003 ) /**< Write        */
#define MVFS_FUNCID_VFSWRITE ( 0x00000004 ) /**< VfsWrite     */
#define MVFS_FUNCID_READ     ( 0x00000005 ) /**< Read         */
#define MVFS_FUNCID_VFSREAD  ( 0x00000006 ) /**< VfsRead      */
#define MVFS_FUNCID_CLOSE    ( 0x00000007 ) /**< Close        */
#define MVFS_FUNCID_VFSCLOSE ( 0x00000008 ) /**< VfsClose     */
#define MVFS_FUNCID_SELECT   ( 0x00000009 ) /**< Select       */
#define MVFS_FUNCID_VFSREADY ( 0x0000000A ) /**< VfsReady     */
#define MVFS_FUNCID_MAX      ( 0x0000000A ) /**< 機能ID最大値 */
#define MVFS_FUNCID_NUM      \
    ( MVFS_FUNCID_MAX + 1 )         /**< 機能数       */

/* タイプ */
#define MVFS_TYPE_REQ  ( 0 )    /**< 要求 */
#define MVFS_TYPE_RESP ( 1 )    /**< 応答 */
#define MVFS_TYPE_NTC  ( 2 )    /**< 通知 */

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

/** バッファ最大サイズ */
#define MVFS_BUFFER_SIZE_MAX ( 24064 )

/* レディ状態 */
#define MVFS_READY_READ  ( 1 )  /**< 読込レディ */
#define MVFS_READY_WRITE ( 2 )  /**< 書込レディ */

/** メッセージヘッダ */
typedef struct {
    uint32_t funcId;    /**< 機能ID */
    uint32_t type;      /**< タイプ */
} MvfsMsgHdr_t;


/*-------*/
/* Close */
/*-------*/
/** Close 応答メッセージ */
typedef struct {
    MvfsMsgHdr_t header;        /**< メッセージヘッダ */
    uint32_t     result;        /**< 処理結果         */
} MvfsMsgCloseResp_t;

/** Close要求メッセージ */
typedef struct {
    MvfsMsgHdr_t header;        /**< メッセージヘッダ */
    uint32_t     globalFd;      /**< グローバルFD     */
} MvfsMsgCloseReq_t;


/*-------*/
/* Mount */
/*-------*/
/** Mount応答メッセージ */
typedef struct {
    MvfsMsgHdr_t header;    /**< メッセージヘッダ */
    uint32_t     result;    /**< 処理結果         */
} MvfsMsgMountResp_t;

/** Mount要求メッセージ */
typedef struct {
    MvfsMsgHdr_t header;                        /**< メッセージヘッダ */
    char         path[ MVFS_PATH_MAXLEN + 1 ];  /**< パス             */
} MvfsMsgMountReq_t;


/*------*/
/* Open */
/*------*/
/** Open応答メッセージ */
typedef struct {
    MvfsMsgHdr_t header;    /**< メッセージヘッダ */
    uint32_t     result;    /**< 処理結果         */
    uint32_t     globalFd;  /**< グローバルFD     */
} MvfsMsgOpenResp_t;

/** Open要求メッセージ */
typedef struct {
    MvfsMsgHdr_t header;                        /**< メッセージヘッダ */
    uint32_t     localFd;                       /**< ローカルFD       */
    char         path[ MVFS_PATH_MAXLEN + 1 ];  /**< 絶対パス(\0含む) */
} MvfsMsgOpenReq_t;


/*------*/
/* Read */
/*------*/
/** Read応答メッセージ */
typedef struct {
    MvfsMsgHdr_t header;        /**< メッセージヘッダ */
    uint32_t     result;        /**< 処理結果         */
    size_t       size;          /**< 読込み実施サイズ */
    char         pBuffer[];     /**< 読込みバッファ   */
} MvfsMsgReadResp_t;

/** Read要求メッセージ */
typedef struct {
    MvfsMsgHdr_t header;        /**< メッセージヘッダ   */
    uint32_t     globalFd;      /**< グローバルFD       */
    uint64_t     readIdx;       /**< 読込みインデックス */
    size_t       size;          /**< 読込みサイズ       */
} MvfsMsgReadReq_t;


/*--------*/
/* Select */
/*--------*/
/** Select応答 */
typedef struct {
    MvfsMsgHdr_t header;        /**< メッセージヘッダ         */
    uint32_t     result;        /**< 処理結果                 */
    size_t       readFdNum;     /**< 読込レディグローバルFD数 */
    size_t       writeFdNum;    /**< 書込レディグローバルFD数 */
    uint32_t     fd[ 0 ];       /**< レディグローバルFD       */
} MvfsMsgSelectResp_t;

/** Select要求 */
typedef struct {
    MvfsMsgHdr_t header;        /**< メッセージヘッダ       */
    size_t       readFdNum;     /**< 読込監視グローバルFD数 */
    size_t       writeFdNum;    /**< 書込監視グローバルFD数 */
    uint32_t     fd[ 0 ];       /**< 監視グローバルFD       */
} MvfsMsgSelectReq_t;


/*----------*/
/* VfsClose */
/*----------*/
/** VfsClose応答メッセージ */
typedef struct {
    MvfsMsgHdr_t header;        /**< メッセージヘッダ */
    uint32_t     globalFd;      /**< グローバルFD     */
    uint32_t     result;        /**< 処理結果         */
} MvfsMsgVfsCloseResp_t;

/** VfsClose要求メッセージ */
typedef struct {
    MvfsMsgHdr_t header;        /**< メッセージヘッダ */
    uint32_t     globalFd;      /**< グローバルFD     */
} MvfsMsgVfsCloseReq_t;


/*---------*/
/* VfsOpen */
/*---------*/
/** VfsOpen応答メッセージ */
typedef struct {
    MvfsMsgHdr_t header;    /**< メッセージヘッダ */
    uint32_t     globalFd;  /**< グローバルFD     */
    uint32_t     result;    /**< 処理結果         */
} MvfsMsgVfsOpenResp_t;

/** VfsOpen要求メッセージ */
typedef struct {
    MvfsMsgHdr_t header;                        /**< メッセージヘッダ */
    MkPid_t      pid;                           /**< プロセスID       */
    uint32_t     globalFd;                      /**< グローバルFD     */
    char         path[ MVFS_PATH_MAXLEN + 1 ];  /**< 絶対パス(\0含む) */
} MvfsMsgVfsOpenReq_t;


/*---------*/
/* VfsRead */
/*---------*/
/** VfsRead応答メッセージ */
typedef struct {
    MvfsMsgHdr_t header;        /**< メッセージヘッダ */
    uint32_t     globalFd;      /**< グローバルFD     */
    uint32_t     result;        /**< 処理結果         */
    uint32_t     ready;         /**< 読込レディ状態   */
    size_t       size;          /**< 読込み実施サイズ */
    char         pBuffer[];     /**< 読込みバッファ   */
} MvfsMsgVfsReadResp_t;

/** VfsRead要求メッセージ */
typedef struct {
    MvfsMsgHdr_t header;        /**< メッセージヘッダ   */
    uint32_t     globalFd;      /**< グローバルFD       */
    uint64_t     readIdx;       /**< 読込みインデックス */
    size_t       size;          /**< 読込みサイズ       */
} MvfsMsgVfsReadReq_t;


/*----------*/
/* VfsReady */
/*----------*/
/** VfsReady通知メッセージ */
typedef struct {
    MvfsMsgHdr_t header;                        /**< メッセージヘッダ */
    char         path[ MVFS_PATH_MAXLEN + 1 ];  /**< パス             */
    uint32_t     ready;                         /**< レディ状態       */
} MvfsMsgVfsReadyNtc_t;


/*----------*/
/* VfsWrite */
/*----------*/
/** VfsWrite応答メッセージ */
typedef struct {
    MvfsMsgHdr_t header;        /**< メッセージヘッダ */
    uint32_t     globalFd;      /**< グローバルFD     */
    uint32_t     result;        /**< 処理結果         */
    uint32_t     ready;         /**< 書込みレディ状態 */
    size_t       size;          /**< 書込み実施サイズ */
} MvfsMsgVfsWriteResp_t;

/** VfsWrite要求メッセージ */
typedef struct {
    MvfsMsgHdr_t header;        /**< メッセージヘッダ   */
    uint32_t     globalFd;      /**< グローバルFD       */
    uint64_t     writeIdx;      /**< 書込みインデックス */
    size_t       size;          /**< 書込みサイズ       */
    char         pBuffer[];     /**< 書込みバッファ     */
} MvfsMsgVfsWriteReq_t;


/*-------*/
/* Write */
/*-------*/
/** Write応答メッセージ */
typedef struct {
    MvfsMsgHdr_t header;        /**< メッセージヘッダ */
    uint32_t     result;        /**< 処理結果         */
    size_t       size;          /**< 書込み実施サイズ */
} MvfsMsgWriteResp_t;

/** Write要求メッセージ */
typedef struct {
    MvfsMsgHdr_t header;        /**< メッセージヘッダ   */
    uint32_t     globalFd;      /**< グローバルFD       */
    uint64_t     writeIdx;      /**< 書込みインデックス */
    size_t       size;          /**< 書込みサイズ       */
    char         pBuffer[];     /**< 書込みバッファ     */
} MvfsMsgWriteReq_t;


/* 戻り値 */
#define MVFS_RET_SUCCESS ( 0x00000000 ) /**< 正常終了 */
#define MVFS_RET_FAILURE ( 0x00000001 ) /**< 異常終了 */

/** 戻り値型 */
typedef uint32_t MvfsRet_t;


/******************************************************************************/
#endif
