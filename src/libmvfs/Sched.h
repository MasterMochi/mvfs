/******************************************************************************/
/*                                                                            */
/* src/libmvfs/Sched.h                                                        */
/*                                                                 2021/11/14 */
/* Copyright (C) 2021 Mochi.                                                  */
/*                                                                            */
/******************************************************************************/
#ifndef SCHED_H
#define SCHED_H
/******************************************************************************/
/* インクルード                                                               */
/******************************************************************************/
/* ライブラリヘッダ */
#include <libmk.h>
#include <MLib/MLibList.h>


/******************************************************************************/
/* 定義                                                                       */
/******************************************************************************/
/* メッセージバッファ */
typedef struct {
    MLibListNode_t nodeInfo;                    /**< ノード情報 */
    char           buffer[ MK_MSG_SIZE_MAX ];   /**< メッセージ */
} SchedMsgBuf_t;


/******************************************************************************/
/* ライブラリ内グローバル関数定義                                             */
/******************************************************************************/
/* メッセージバッファ追加 */
extern void SchedAddMsgBuffer( SchedMsgBuf_t *pMsgBuf );


/******************************************************************************/
#endif
