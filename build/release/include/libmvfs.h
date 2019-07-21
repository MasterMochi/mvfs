/******************************************************************************/
/*                                                                            */
/* libmvfs.h                                                                  */
/*                                                                 2019/07/20 */
/* Copyright (C) 2019 Mochi.                                                  */
/*                                                                            */
/******************************************************************************/
#ifndef __LIB_MVFS_H__
#define __LIB_MVFS_H__
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
/* 戻り値 */
#define LIBMVFS_RET_SUCCESS ( 0 )   /**< 正常終了 */
#define LIBMVFS_RET_FAILURE ( 1 )   /**< 異常終了 */

/* エラー番号 */
#define LIBMVFS_ERR_NONE       ( 0x00000000 )   /**< エラー無し               */
#define LIBMVFS_ERR_PARAM      ( 0x00000001 )   /**< パラメータ不正           */
#define LIBMVFS_ERR_NOT_FOUND  ( 0x00000002 )   /**< 仮想ファイルサーバ不明   */
#define LIBMVFS_ERR_NOT_RESP   ( 0x00000003 )   /**< 応答無し                 */
#define LIBMVFS_ERR_NO_MEMORY  ( 0x00000004 )   /**< メモリ不足               */
#define LIBMVFS_ERR_SERVER     ( 0x00000005 )   /**< 仮想ファイルサーバエラー */
#define LIBMVFS_ERR_INVALID_FD ( 0x00000006 )   /**< FD無効                   */
#define LIBMVFS_ERR_OTHER      ( 0x0000FFFF )   /**< その他エラー             */

/** 処理結果 */
typedef uint32_t LibMvfsRet_t;

/** その他メッセージコールバック関数型 */
typedef
    void
    ( *LibMvfsFuncOther_t )
    ( MkTaskId_t src,           /* 送信元タスクID   */
      void       *pMsg,         /* 受信メッセージ   */
      size_t     size   );      /* 受信メッセージ長 */

/** vfsOpen要求コールバック関数型 */
typedef
    void
    ( *LibMvfsFuncVfsOpen_t )
    ( MkPid_t    pid,           /* PID          */
      uint32_t   globalFd,      /* グローバルFD */
      const char *pPath    );   /* ファイルパス */

/** vfsWrite要求コールバック関数型 */
typedef
    void
    ( *LibMvfsFuncVfsWrite_t )
    ( uint32_t globalFd,    /* グローバルFD       */
      uint64_t writeIdx,    /* 書込みインデックス */
      void     *pBuffer,    /* 書込みバッファ     */
      size_t   size      ); /* 書込みサイズ       */

/** vfsRead要求コールバック関数型 */
typedef
    void
    ( *LibMvfsFuncVfsRead_t )
    ( uint32_t globalFd,    /* グローバルFD       */
      uint64_t readIdx,     /* 書込みインデックス */
      size_t   size      ); /* 書込みサイズ       */

/** vfsClose要求コールバック関数型 */
typedef
    void
    ( *LibMvfsFuncVfsClose_t )
    ( uint32_t globalFd );  /* グローバルFD */

/** スケジューラコールバック関数リスト */
typedef struct {
    LibMvfsFuncVfsOpen_t  pVfsOpen;     /**< vfsOpen要求      */
    LibMvfsFuncVfsWrite_t pVfsWrite;    /**< vfsWrite要求     */
    LibMvfsFuncVfsRead_t  pVfsRead;     /**< vfsRead要求      */
    LibMvfsFuncVfsClose_t pVfsClose;    /**< vfsClose要求     */
    LibMvfsFuncOther_t    pOther;       /**< その他メッセージ */
} LibMvfsSchedCallBack_t;

/** スケジューラ情報 */
typedef struct {
    LibMvfsSchedCallBack_t callBack;
} LibMvfsSchedInfo_t;


/******************************************************************************/
/* ライブラリ関数定義                                                         */
/******************************************************************************/
/* Close */
extern LibMvfsRet_t LibMvfsClose( uint32_t fd,
                                  uint32_t *pErrNo );
/* TaskID取得 */
extern LibMvfsRet_t LibMvfsGetTaskId( MkTaskId_t *pTaskId,
                                      uint32_t   *pErrNo   );
/* Mount */
extern LibMvfsRet_t LibMvfsMount( const char *pPath,
                                  uint32_t   *pErrNo );
/* Open */
extern LibMvfsRet_t LibMvfsOpen( uint32_t   *pLocalFd,
                                 const char *pPath,
                                 uint32_t   *pErrNo    );
/* Read */
extern LibMvfsRet_t LibMvfsRead( uint32_t fd,
                                 void     *pBuffer,
                                 size_t   bufferSize,
                                 size_t   *pReadSize,
                                 uint32_t *pErrNo     );
/* スケジュール開始 */
extern LibMvfsRet_t LibMvfsSchedStart( LibMvfsSchedInfo_t *pInfo,
                                       uint32_t           *pErrNo );
/* vfsClose応答メッセージ送信 */
extern LibMvfsRet_t LibMvfsSendVfsCloseResp( uint32_t globalFd,
                                             uint32_t result,
                                             uint32_t *pErrNo   );
/* vfsOpen応答メッセージ送信 */
extern LibMvfsRet_t LibMvfsSendVfsOpenResp( uint32_t result,
                                            uint32_t *pErrNo );
/* vfsRead応答メッセージ送信 */
extern LibMvfsRet_t LibMvfsSendVfsReadResp( uint32_t globalFd,
                                            uint32_t result,
                                            void     *pBuffer,
                                            size_t   size,
                                            uint32_t *pErrNo   );
/* vfsWrite応答メッセージ送信 */
extern LibMvfsRet_t LibMvfsSendVfsWriteResp( uint32_t globalFd,
                                             uint32_t result,
                                             size_t   size,
                                             uint32_t *pErrNo );
/* Write */
extern LibMvfsRet_t LibMvfsWrite( uint32_t fd,
                                  void     *pBuffer,
                                  size_t   bufferSize,
                                  size_t   *pWriteSize,
                                  uint32_t *pErrNo      );


/******************************************************************************/
#endif
