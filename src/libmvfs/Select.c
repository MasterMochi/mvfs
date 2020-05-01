/******************************************************************************/
/*                                                                            */
/* src/libmvfs/Select.c                                                       */
/*                                                                 2020/04/30 */
/* Copyright (C) 2019-2020 Mochi.                                             */
/*                                                                            */
/******************************************************************************/
/******************************************************************************/
/* インクルード                                                               */
/******************************************************************************/
/* 標準ヘッダ */
#include <stdint.h>
#include <stdlib.h>

/* ライブラリヘッダ */
#include <libmk.h>
#include <libmvfs.h>
#include <MLib/MLib.h>

/* モジュール共通ヘッダ */
#include <mvfs.h>

/* モジュール内ヘッダ */
#include "Fd.h"


/******************************************************************************/
/* ローカル関数定義                                                           */
/******************************************************************************/
/* FDビットリスト変換 */
static void ConvertFdsToList( uint32_t     *pList,
                              LibMvfsFds_t *pFds   );
/* FDリスト変換 */
static void ConvertListToFds( LibMvfsFds_t *pFds,
                              uint32_t     *pList,
                              size_t       fdNum   );
/* FD数取得 */
static size_t GetFdsNum( LibMvfsFds_t *pFds );
/* Select応答受信 */
static LibMvfsRet_t ReceiveSelectResp( MkTaskId_t   taskId,
                                       LibMvfsFds_t *pReadFds,
                                       LibMvfsFds_t *pWriteFds,
                                       uint32_t     timeout,
                                       uint32_t     *pErr       );
/* Select要求送信 */
static LibMvfsRet_t SendSelectReq( MkTaskId_t   taskId,
                                   LibMvfsFds_t *pReadFds,
                                   LibMvfsFds_t *pWriteFds,
                                   uint32_t     *pErr       );


/******************************************************************************/
/* ライブラリ関数定義                                                         */
/******************************************************************************/
/******************************************************************************/
/**
 * @brief       ファイルレディ監視
 * @details     仮想ファイルサーバにselect要求メッセージを送信して、ファイルの
 *              読込み書込みレディ監視を要求し、その応答を待ち合わせる。
 *
 * @param[in]   *pReadFds  読込監視ローカルFDビットリスト
 * @param[in]   *pWriteFds 書込監視ローカルFDビットリスト
 * @param[out]  *pReadyFds レディFDビットリスト
 * @param[in]   timeout    タイムアウト時間[us]
 * @param[out]  *pErr      エラー
 *                  - LIBMVFS_ERR_NONE エラー無し
 *
 * @return      処理結果を返す。
 * @retval      LIBMVFS_RET_SUCCESS 正常終了
 * @retval      LIBMVFS_RET_FAILURE 異常終了
 */
/******************************************************************************/
LibMvfsRet_t LibMvfsSelect( LibMvfsFds_t *pReadFds,
                            LibMvfsFds_t *pWriteFds,
                            uint32_t     timeout,
                            uint32_t     *pErr       )
{
    MkTaskId_t   taskId;    /* タスクID */
    LibMvfsRet_t ret;       /* 戻り値 */

    /* 初期化 */
    taskId = MK_TASKID_NULL;
    ret    = LIBMVFS_RET_FAILURE;

    /* タスクID取得 */
    ret = LibMvfsGetTaskId( &taskId, pErr );

    /* 取得結果判定 */
    if ( ret != LIBMVFS_RET_SUCCESS ) {
        /* 失敗 */

        return LIBMVFS_RET_FAILURE;
    }

    /* Select要求メッセージ送信 */
    ret = SendSelectReq( taskId, pReadFds, pWriteFds, pErr );

    /* 送信結果判定 */
    if ( ret != LIBMVFS_RET_SUCCESS ) {
        /* 失敗 */

        return ret;
    }

    /* Select応答メッセージ受信 */
    ret = ReceiveSelectResp( taskId, pReadFds, pWriteFds, timeout, pErr );

    /* 受信結果判定 */
    if ( ret != LIBMVFS_RET_SUCCESS ) {
        /* 失敗 */

        return ret;
    }

    return LIBMVFS_RET_SUCCESS;
}


/******************************************************************************/
/* ローカル関数定義                                                           */
/******************************************************************************/
/******************************************************************************/
/**
 * @brief       FDビットリスト変換
 * @details     ローカルFDビットリストをグローバルFDリストに変換する。
 *
 * @param[out]  *pList グローバルFDリスト
 * @param[in]   *pFds  ローカルFDビットリスト
 */
/******************************************************************************/
static void ConvertFdsToList( uint32_t     *pList,
                              LibMvfsFds_t *pFds   )
{
    uint32_t fd;        /* FD             */
    uint32_t no;        /* FD数           */
    FdInfo_t *pFdInfo;  /* ローカルFD情報 */

    /* 初期化 */
    fd      = 0;
    no      = 0;
    pFdInfo = NULL;

    /* 引数判定 */
    if ( pFds == NULL ) {
        /* 不正 */
        return;
    }

    /* FD毎に繰り返す */
    for ( fd = 0; fd < LIBMVFS_FD_MAXNUM; fd++ ) {
        /* リスト内有無判定 */
        if ( LIBMVFS_FDS_CHECK( pFds, fd ) == 0 ) {
            /* 無 */
            continue;
        }

        /* ローカルFD情報取得 */
        pFdInfo = FdGetLocalFdInfo( fd );

        /* 取得結果判定 */
        if ( pFdInfo == NULL ) {
            /* 失敗 */
            continue;
        }

        /* リスト設定 */
        pList[ no++ ] = pFdInfo->globalFd;
    }

    return;
}


/******************************************************************************/
/**
 * @brief       FDリスト変換
 * @details     グローバルFDリストをローカルFDビットリストに変換する。
 *
 * @param[out]  *pFds  ローカルFDビットリスト
 * @param[in]   *pList グローバルFDリスト
 * @param[in]   fdNum  グローバルFD数
 */
/******************************************************************************/
static void ConvertListToFds( LibMvfsFds_t *pFds,
                              uint32_t     *pList,
                              size_t       fdNum   )
{
    uint32_t idx;       /* リストインデックス */
    FdInfo_t *pFdInfo;  /* グローバルFD情報   */

    /* 初期化 */
    idx     = 0;
    pFdInfo = NULL;

    /* 引数判定 */
    if ( pFds == NULL ) {
        /* 不正 */
        return;
    }

    /* リストエントリ毎に繰り返す */
    for ( idx = 0; idx < fdNum; idx++ ) {
        /* グローバルFD情報取得 */
        pFdInfo = FdGetGlobalFdInfo( pList[ idx ] );

        /* 取得結果判定 */
        if ( pFdInfo == NULL ) {
            /* 失敗 */
            continue;
        }

        /* ローカルFDビットリスト設定 */
        LIBMVFS_FDS_SET( pFds, pFdInfo->localFd );
    }

    return;
}


/******************************************************************************/
/**
 * @brief       FD数取得
 * @details     FDビットリスト*pFdsに設定されているFD数を取得する。
 *
 * @param[in]   *pFds FDビットリスト
 *
 * @return      FD数を返す。
 */
/******************************************************************************/
static size_t GetFdsNum( LibMvfsFds_t *pFds )
{
    size_t   ret;   /* FD数 */
    uint32_t fd;    /* FD   */

    /* 初期化 */
    ret = 0;
    fd  = 0;

    /* 引数判定 */
    if ( pFds == NULL ) {
        /* 不正 */

        return 0;
    }

    /* FD毎に繰り返す */
    for ( fd = 0; fd < LIBMVFS_FD_MAXNUM; fd++ ) {
        /* リスト内有無判定 */
        if ( LIBMVFS_FDS_CHECK( pFds, fd ) == 0 ) {
            /* 無 */
            continue;
        }

        /* FD数更新 */
        ret++;
    }

    return ret;
}


/******************************************************************************/
/**
 * @brief       Select応答受信
 * @details     仮想ファイルサーバからSelect応答メッセージの受信を待ち合わせる。
 *
 * @param[in]   taskId     仮想ファイルサーバタスクID
 * @param[out]  *pReadFds  読込レディFDビットリスト
 * @param[out]  *pWriteFds 書込レディFDビットリスト
 * @param[int]  timeout    タイムアウト時間[us]
 * @param[out]  *pErr      エラー内容
 *                  - LIBMVFS_ERR_NONE      エラー無し
 *                  - LIBMVFS_ERR_NO_MEMORY メモリ不足
 *                  - LIBMVFS_ERR_NOT_RESP  応答無し
 *                  - LIBMVFS_ERR_TIMEOUT   タイムアウト
 *                  - LIBMVFS_ERR_ERR_OTHER その他エラー
 *
 * @return      処理結果を返す。
 * @retval      LIBMVFS_RET_SUCCESS 正常終了
 * @retval      LIBMVFS_RET_FAILURE 異常終了
 */
/******************************************************************************/
static LibMvfsRet_t ReceiveSelectResp( MkTaskId_t   taskId,
                                       LibMvfsFds_t *pReadFds,
                                       LibMvfsFds_t *pWriteFds,
                                       uint32_t     timeout,
                                       uint32_t     *pErr       )
{
    size_t              size;   /* 受信メッセージサイズ  */
    MkRet_t             retMk;  /* カーネル戻り値        */
    MkErr_t             errMk;  /* カーネルエラー        */
    MvfsMsgSelectResp_t *pMsg;  /* Select応答メッセージ　*/

    /* 初期化 */
    size  = 0;
    retMk = MK_RET_FAILURE;
    errMk = MK_ERR_NONE;
    pMsg  = NULL;

    /* メッセージバッファ割当 */
    pMsg = malloc( MK_MSG_SIZE_MAX );

    /* 割当結果判定 */
    if ( pMsg == NULL ) {
        /* 失敗 */
        MLIB_SET_IFNOT_NULL( pErr, LIBMVFS_ERR_NO_MEMORY );
        return LIBMVFS_RET_FAILURE;
    }

    /* メッセージ受信 */
    retMk = LibMkMsgReceive( taskId,                /* 受信タスクID   */
                             pMsg,                  /* バッファ       */
                             MK_MSG_SIZE_MAX,       /* バッファサイズ */
                             NULL,                  /* 送信元タスクID */
                             &size,                 /* 受信サイズ     */
                             timeout,               /* タイムアウト値 */
                             &errMk           );    /* エラー要因     */

    /* 受信結果判定 */
    if ( retMk != MK_RET_SUCCESS ) {
        /* 失敗 */

        /* エラー判定 */
        if ( errMk == MK_ERR_NO_EXIST ) {
            /* 存在しないタスクID */
            MLIB_SET_IFNOT_NULL( pErr, LIBMVFS_ERR_NOT_FOUND );

        } else if ( errMk == MK_ERR_NO_MEMORY ) {
            /* メモリ不足 */
            MLIB_SET_IFNOT_NULL( pErr, LIBMVFS_ERR_NO_MEMORY );

        } else if ( errMk == MK_ERR_TIMEOUT ) {
            /* タイムアウト */
            MLIB_SET_IFNOT_NULL( pErr, LIBMVFS_ERR_TIMEOUT );

        } else {
            /* その他エラー */
            MLIB_SET_IFNOT_NULL( pErr, LIBMVFS_ERR_OTHER );
        }

        return LIBMVFS_RET_FAILURE;
    }

    /* メッセージチェック */
    if ( ( pMsg->header.funcId != MVFS_FUNCID_SELECT ) &&
         ( pMsg->header.type   != MVFS_TYPE_RESP     )    ) {
        /* メッセージ不正 */
        MLIB_SET_IFNOT_NULL( pErr, LIBMVFS_ERR_NOT_RESP );
        return LIBMVFS_RET_FAILURE;
    }

    /* 読込レディFDビットリスト設定 */
    ConvertListToFds( pReadFds,
                      &( pMsg->fd[ 0 ] ),
                      pMsg->readFdNum     );

    /* 書込レディFDビットリスト設定 */
    ConvertListToFds( pWriteFds,
                      &( pMsg->fd[ pMsg->readFdNum ] ),
                      pMsg->writeFdNum                  );

    return LIBMVFS_RET_SUCCESS;
}


/******************************************************************************/
/**
 * @brief       Select要求送信
 * @details     仮想ファイルサーバにSelect要求メッセージを送信する。
 *
 * @param[in]   taskId     仮想ファイルサーバのタスクID
 * @param[in]   *pReadFds  読込監視ローカルFDビットリスト
 * @param[in]   *pWriteFds 書込監視ローカルFDビットリスト
 * @param[out]  *pErr      エラー番号
 *                  - LIBMVFS_ERR_NONE      エラー無し
 *                  - LIBMVFS_ERR_NOT_FOUND 仮想ファイルサーバ不正
 *                  - LIBMVFS_ERR_NO_MEMORY メモリ不足
 *                  - LIBMVFS_ERR_OTHER     その他エラー
 *
 * @return      送信結果を返す。
 * @retval      LIBMVFS_RET_SUCCESS 正常終了
 * @retval      LIBMVFS_RET_FAILURE 異常終了
 */
/******************************************************************************/
static LibMvfsRet_t SendSelectReq( MkTaskId_t   taskId,
                                   LibMvfsFds_t *pReadFds,
                                   LibMvfsFds_t *pWriteFds,
                                   uint32_t     *pErr       )
{
    size_t             size;        /* メッセージサイズ */
    size_t             readFdNum;   /* 読込監視数       */
    size_t             writeFdNum;  /* 書込監視数       */
    MkRet_t            retMk;       /* カーネル戻り値   */
    MkErr_t            errMk;       /* カーネルエラー   */
    LibMvfsRet_t       ret;         /* 戻り値           */
    MvfsMsgSelectReq_t *pMsg;       /* 要求メッセージ   */

    /* 初期化 */
    retMk = MK_RET_FAILURE;
    errMk = MK_ERR_NONE;
    ret   = LIBMVFS_RET_SUCCESS;

    /* 監視数取得 */
    readFdNum  = GetFdsNum( pReadFds );    /* 読込み */
    writeFdNum = GetFdsNum( pWriteFds );   /* 書込み */

    size = sizeof ( MvfsMsgSelectReq_t )    +
           readFdNum  * sizeof ( uint32_t ) +
           writeFdNum * sizeof ( uint32_t );

    /* メッセージ領域割当 */
    pMsg = malloc( size );

    /* 割当結果判定 */
    if ( pMsg == NULL ) {
        /* 失敗 */

        /* エラー設定 */
        MLIB_SET_IFNOT_NULL( pErr, LIBMVFS_ERR_NO_MEMORY );
        return LIBMVFS_RET_FAILURE;
    }

    /* メッセージ作成 */
    pMsg->header.funcId = MVFS_FUNCID_SELECT;
    pMsg->header.type   = MVFS_TYPE_REQ;
    pMsg->readFdNum     = readFdNum;
    pMsg->writeFdNum    = writeFdNum;

    /* リスト変換 */
    ConvertFdsToList( &( pMsg->fd[         0 ] ), pReadFds  );
    ConvertFdsToList( &( pMsg->fd[ readFdNum ] ), pWriteFds );

    /* メッセージ送信 */
    retMk = LibMkMsgSend( taskId,       /* 送信先タスクID   */
                          pMsg,         /* 送信メッセージ   */
                          size,         /* 送信メッセージ長 */
                          &errMk  );    /* エラー           */

    /* 送信結果判定 */
    if ( retMk != MK_RET_SUCCESS ) {
        /* 失敗 */

        /* エラー判定 */
        if ( errMk == MK_ERR_NO_EXIST ) {
            /* 送信先不明 */

            /* エラー設定 */
            MLIB_SET_IFNOT_NULL( pErr, LIBMVFS_ERR_NOT_FOUND );

        } else if ( errMk == MK_ERR_NO_MEMORY ) {
            /* メモリ不足 */

            /* エラー設定 */
            MLIB_SET_IFNOT_NULL( pErr, LIBMVFS_ERR_NO_MEMORY );

        } else {
            /* その他 */

            /* エラー設定 */
            MLIB_SET_IFNOT_NULL( pErr, LIBMVFS_ERR_OTHER ) ;
        }

        ret = LIBMVFS_RET_FAILURE;
    }

    /* メッセージ領域解放 */
    free( pMsg );

    return ret;
}


/******************************************************************************/
