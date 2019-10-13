/******************************************************************************/
/*                                                                            */
/* src/include/Msg.h                                                          */
/*                                                                 2019/10/07 */
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

/* 外部モジュールヘッダ */
#include <mvfs.h>
#include <Fd.h>
#include <Node.h>


/******************************************************************************/
/* 外部モジュール向けグローバル関数宣言                                       */
/******************************************************************************/
/* Close要求メッセージチェック */
extern MvfsRet_t MsgCheckCloseReq( MkTaskId_t        taskId,
                                   MvfsMsgCloseReq_t *pMsg,
                                   size_t            size,
                                   FdInfo_t          **ppFdInfo );
/* Mount要求メッセージチェック */
extern MvfsRet_t MsgCheckMountReq( MvfsMsgMountReq_t *pMsg,
                                   size_t            size   );
/* Open要求メッセージチェック */
extern MvfsRet_t MsgCheckOpenReq( MvfsMsgOpenReq_t *pMsg,
                                  size_t           size   );
/* Read要求メッセージチェック */
extern MvfsRet_t MsgCheckReadReq( MkTaskId_t       taskId,
                                  MvfsMsgReadReq_t *pMsg,
                                  size_t           size,
                                  FdInfo_t         **ppFdInfo );
/* Select要求メッセージチェック */
extern MvfsRet_t MsgCheckSelectReq( MkTaskId_t         taskId,
                                    MvfsMsgSelectReq_t *pMsg,
                                    size_t             size    );
/* VfsClose応答メッセージチェック */
extern MvfsRet_t MsgCheckVfsCloseResp( MkTaskId_t            taskId,
                                       MvfsMsgVfsCloseResp_t *pMsg,
                                       size_t                size,
                                       FdInfo_t              **ppFdInfo );
/* VfsOpen応答メッセージチェック */
extern MvfsRet_t MsgCheckVfsOpenResp( MkTaskId_t           taskId,
                                      MvfsMsgVfsOpenResp_t *pMsg,
                                      size_t               size,
                                      FdInfo_t             **ppFdInfo );
/* VfsRead応答メッセージチェック */
extern MvfsRet_t MsgCheckVfsReadResp( MkTaskId_t           taskId,
                                      MvfsMsgVfsReadResp_t *pMsg,
                                      size_t               size,
                                      FdInfo_t             **ppFdInfo );
/* VfsReady通知メッセージチェック */
extern MvfsRet_t MsgCheckVfsReadyNtc( MkTaskId_t           taskId,
                                      MvfsMsgVfsReadyNtc_t *pMsg,
                                      size_t               size,
                                      NodeInfo_t           **ppNode );
/* VfsWrite応答メッセージチェック */
extern MvfsRet_t MsgCheckVfsWriteResp( MkTaskId_t            taskId,
                                       MvfsMsgVfsWriteResp_t *pMsg,
                                       size_t                size,
                                       FdInfo_t              **ppFdInfo );
/* Write応答メッセージチェック */
extern MvfsRet_t MsgCheckWriteReq( MkTaskId_t        taskId,
                                   MvfsMsgWriteReq_t *pMsg,
                                   size_t            size,
                                   FdInfo_t          **ppFdInfo );
/* Close応答メッセージ送信 */
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
