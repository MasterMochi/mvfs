/******************************************************************************/
/*                                                                            */
/* src/include/Fn.h                                                           */
/*                                                                 2019/09/18 */
/* Copyright (C) 2019 Mochi.                                                  */
/*                                                                            */
/******************************************************************************/
#ifndef FN_H
#define FN_H
/******************************************************************************/
/* インクルード                                                               */
/******************************************************************************/
/* カーネルヘッダ */
#include <kernel/types.h>


/******************************************************************************/
/* 外部モジュール向けグローバル関数宣言                                       */
/******************************************************************************/
/* Closeメッセージ受信 */
extern void FnCloseRecvCloseReq( MkTaskId_t taskId,
                                 void       *pBuffer );
/* VfsClose応答メッセージ受信 */
extern void FnCloseRecvVfsCloseResp( MkTaskId_t taskId,
                                     void       *pBuffer );
/* 機能初期化 */
extern void FnInit( void );
/* Mount要求メッセージ受信 */
extern void FnMountRecvMountReq( MkTaskId_t taskId,
                                 void       *pBuffer );
/* Openメッセージ受信 */
extern void FnOpenRecvOpenReq( MkTaskId_t taskId,
                               void       *pBuffer );
/* VfsOpen応答メッセージ受信 */
extern void FnOpenRecvVfsOpenResp( MkTaskId_t taskId,
                                   void       *pBuffer );
/* Read要求メッセージ受信 */
extern void FnReadRecvReadReq( MkTaskId_t taskId,
                               void       *pBuffer );
/* VfsRead応答メッセージ受信 */
extern void FnReadRecvVfsReadResp( MkTaskId_t taskId,
                                   void       *pBuffer );
/* Select要求メッセージ受信 */
extern void FnSelectRecvSelectReq( MkTaskId_t taskId,
                                   void       *pBuffer );
/* VfsReady通知メッセージ受信 */
extern void FnSelectRecvVfsReadyNtc( MkTaskId_t taskId,
                                     void       *pBuffer );
/* VfsWrite応答メッセージ受信 */
extern void FnWriteRecvVfsWriteResp( MkTaskId_t taskId,
                                     void       *pBuffer );
/* Write要求メッセージ受信 */
extern void FnWriteRecvWriteReq( MkTaskId_t taskId,
                                 void       *pBuffer );


/******************************************************************************/
#endif
