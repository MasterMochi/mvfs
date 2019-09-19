/******************************************************************************/
/*                                                                            */
/* src/include/Msg.h                                                          */
/*                                                                 2019/09/18 */
/* Copyright (C) 2019 Mochi.                                                  */
/*                                                                            */
/******************************************************************************/
#ifndef MSG_H
#define MSG_H
/******************************************************************************/
/* インクルード                                                               */
/******************************************************************************/
/* 標準ヘッダ */
#include <stddef.h>
#include <stdint.h>

/* カーネルヘッダ */
#include <kernel/types.h>


/******************************************************************************/
/* 外部モジュール向けグローバル関数宣言                                       */
/******************************************************************************/
/* Close応答めせーじ送信 */
extern void MsgSendCloseResp( MkTaskId_t dst,
                              uint32_t   result );
/* Mount応答メッセージ送信 */
extern void MsgSendMountResp( MkTaskId_t dst,
                              uint32_t   result );
/* Open応答メッセージ送信 */
extern void MsgSendOpenResp( MkTaskId_t taskId,
                             uint32_t   result,
                             uint32_t   globalFd );
/* Read応答メッセージ送信 */
extern void MsgSendReadResp( MkTaskId_t taskId,
                             uint32_t   result,
                             void       *pBuffer,
                             size_t     size      );
/* Select応答メッセージ送信 */
extern void MsgSendSelectResp( MkTaskId_t taskId,
                               uint32_t   result,
                               uint32_t   *pReadFdList,
                               size_t     readFdNum,
                               uint32_t   *pWriteFdList,
                               size_t     writeFdNum     );
/* VfsClose要求メッセージ送信 */
extern void MsgSendVfsCloseReq( MkTaskId_t dst,
                                uint32_t   globalFd );
/* VfsOpen要求メッセージ送信 */
extern void MsgSendVfsOpenReq( MkTaskId_t dst,
                               MkPid_t    pid,
                               uint32_t   fd,
                               const char *pPath );
/* VfsRead要求メッセージ送信 */
extern void MsgSendVfsReadReq( MkTaskId_t dst,
                               uint32_t   globalFd,
                               uint64_t   readIdx,
                               size_t     size      );
/* VfsWrite要求メッセージ送信 */
extern void MsgSendVfsWriteReq( MkTaskId_t dst,
                                uint32_t   globalFd,
                                uint64_t   writeIdx,
                                const char *pBuffer,
                                size_t     size      );
/* Write応答メッセージ送信 */
extern void MsgSendWriteResp( MkTaskId_t taskId,
                              uint32_t   result,
                              size_t     size    );


/******************************************************************************/
#endif
