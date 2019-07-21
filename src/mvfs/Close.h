/******************************************************************************/
/*                                                                            */
/* src/mvfs/Close.h                                                           */
/*                                                                 2019/07/20 */
/* Copyright (C) 2019 Mochi.                                                  */
/*                                                                            */
/******************************************************************************/
#ifndef CLOSE_H
#define CLOSE_H
/******************************************************************************/
/* インクルード                                                               */
/******************************************************************************/
/* 標準ヘッダ */
#include <kernel/types.h>


/******************************************************************************/
/* グローバル関数宣言                                                         */
/******************************************************************************/
/* Close機能初期化 */
extern void CloseInit( void );
/* vfsClose応答メッセージ受信 */
extern void CloseRcvMsgVfsCloseResp( MkTaskId_t taskId,
                                     void       *pBuffer );
/* closeメッセージ受信 */
extern void CloseRcvMsgCloseReq( MkTaskId_t taskId,
                                 void       *pBuffer );


/******************************************************************************/
#endif
