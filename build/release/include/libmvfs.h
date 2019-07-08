/******************************************************************************/
/*                                                                            */
/* libmvfs.h                                                                  */
/*                                                                 2019/07/07 */
/* Copyright (C) 2019 Mochi.                                                  */
/*                                                                            */
/******************************************************************************/
#ifndef __LIB_MVFS_H__
#define __LIB_MVFS_H__
/******************************************************************************/
/* インクルード                                                               */
/******************************************************************************/
/* 標準ヘッダ */
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
#define LIBMVFS_ERR_NONE      ( 0x00000000 )    /**< エラー無し               */
#define LIBMVFS_ERR_PARAM     ( 0x00000001 )    /**< パラメータ不正           */
#define LIBMVFS_ERR_NOT_FOUND ( 0x00000002 )    /**< 仮想ファイルサーバ不明   */
#define LIBMVFS_ERR_NOT_RESP  ( 0x00000003 )    /**< 応答無し                 */
#define LIBMVFS_ERR_NO_MEMORY ( 0x00000004 )    /**< メモリ不足               */
#define LIBMVFS_ERR_SERVER    ( 0x00000005 )    /**< 仮想ファイルサーバエラー */
#define LIBMVFS_ERR_OTHER     ( 0x0000FFFF )    /**< その他エラー             */

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

/** スケジューラコールバック関数リスト */
typedef struct {
    LibMvfsFuncVfsOpen_t pVfsOpen;  /**< vfsOpen要求      */
    LibMvfsFuncOther_t   pOther;    /**< その他メッセージ */
} LibMvfsSchedCallBack_t;

/** スケジューラ情報 */
typedef struct {
    LibMvfsSchedCallBack_t callBack;
} LibMvfsSchedInfo_t;


/******************************************************************************/
/* ライブラリ関数定義                                                         */
/******************************************************************************/
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
/* スケジュール開始 */
extern LibMvfsRet_t LibMvfsSchedStart( LibMvfsSchedInfo_t *pInfo,
                                       uint32_t           *pErrNo );

/* vfsOpen応答メッセージ送信 */
extern LibMvfsRet_t LibMvfsSendVfsOpenResp( uint32_t result,
                                            uint32_t *pErrNo );


/******************************************************************************/
#endif
