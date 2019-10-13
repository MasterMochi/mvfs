/******************************************************************************/
/*                                                                            */
/* src/include/Fn.h                                                           */
/*                                                                 2019/10/07 */
/* Copyright (C) 2019 Mochi.                                                  */
/*                                                                            */
/******************************************************************************/
#ifndef FN_H
#define FN_H
/******************************************************************************/
/* インクルード                                                               */
/******************************************************************************/
/* 標準ヘッダ */
#include <stddef.h>

/* カーネルヘッダ */
#include <kernel/types.h>


/******************************************************************************/
/* 外部モジュール向けグローバル関数宣言                                       */
/******************************************************************************/
/* 機能初期化 */
extern void FnInit( void );
/* Closeメッセージ受信 */
extern void FnMainRecvCloseReq( MkTaskId_t taskId,
                                void       *pBuffer,
                                size_t     size      );
/* Openメッセージ受信 */
extern void FnMainRecvOpenReq( MkTaskId_t taskId,
                               void       *pBuffer,
                               size_t     size      );
/* Read要求メッセージ受信 */
extern void FnMainRecvReadReq( MkTaskId_t taskId,
                               void       *pBuffer,
                               size_t     size      );
/* VfsClose応答メッセージ受信 */
extern void FnMainRecvVfsCloseResp( MkTaskId_t taskId,
                                    void       *pBuffer,
                                    size_t     size      );
/* VfsOpen応答メッセージ受信 */
extern void FnMainRecvVfsOpenResp( MkTaskId_t taskId,
                                   void       *pBuffer,
                                   size_t     size      );
/* VfsRead応答メッセージ受信 */
extern void FnMainRecvVfsReadResp( MkTaskId_t taskId,
                                   void       *pBuffer,
                                   size_t     size      );
/* VfsWrite応答メッセージ受信 */
extern void FnMainRecvVfsWriteResp( MkTaskId_t taskId,
                                    void       *pBuffer,
                                    size_t     size      );
/* Write要求メッセージ受信 */
extern void FnMainRecvWriteReq( MkTaskId_t taskId,
                                void       *pBuffer,
                                size_t     size      );
/* Mount要求メッセージ受信 */
extern void FnTaskRecvMountReq( MkTaskId_t taskId,
                                void       *pBuffer,
                                size_t     size      );
/* Select要求メッセージ受信 */
extern void FnTaskRecvSelectReq( MkTaskId_t taskId,
                                 void       *pBuffer,
                                 size_t     size      );
/* VfsReady通知メッセージ受信 */
extern void FnTaskRecvVfsReadyNtc( MkTaskId_t taskId,
                                   void       *pBuffer,
                                   size_t     size      );


/******************************************************************************/
#endif
