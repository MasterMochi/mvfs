/******************************************************************************/
/*                                                                            */
/* src/mvfs/Read.h                                                            */
/*                                                                 2019/07/14 */
/* Copyright (C) 2019 Mochi.                                                  */
/*                                                                            */
/******************************************************************************/
#ifndef READ_H
#define READ_H
/******************************************************************************/
/* インクルード                                                               */
/******************************************************************************/
/* 標準ヘッダ */
#include <kernel/types.h>


/******************************************************************************/
/* グローバル関数宣言                                                         */
/******************************************************************************/
/* Read機能初期化 */
extern void ReadInit( void );
/* vfsRead応答メッセージ受信 */
extern void ReadRcvMsgVfsReadResp( MkTaskId_t taskId,
                                   void       *pBuffer );
/* readメッセージ受信 */
extern void ReadRcvMsgReadReq( MkTaskId_t taskId,
                               void       *pBuffer );


/******************************************************************************/
#endif
