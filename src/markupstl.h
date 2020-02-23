// MarkupSTL.h: interface for the CMarkupSTL class.
//
// Markup Release 7.0
// Copyright (C) 1999-2004 First Objective Software, Inc. All rights reserved
// Go to www.firstobject.com for the latest CMarkupSTL and EDOM documentation
// Use in commercial applications requires written permission
// This software is provided "as is", with no warranty.

#if !defined(AFX_MARKUPSTL_H__948A2705_9E68_11D2_A0BF_00105A27C570__INCLUDED_)
#define AFX_MARKUPSTL_H__948A2705_9E68_11D2_A0BF_00105A27C570__INCLUDED_

//#pragma warning(disable:4786)
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <string>

#ifdef _DEBUG
#define _DS(i) (i?&(m_strDoc.c_str())[m_aPos[i].nStart]:0)
#define MARKUP_SETDEBUGSTATE m_pMainDS=_DS(m_iPos); m_pChildDS=_DS(m_iPosChild)
#else
#define MARKUP_SETDEBUGSTATE
#endif

class CMarkupSTL  
{
public:
    CMarkupSTL() { SetDoc( NULL ); m_nFlags=0; };
    CMarkupSTL( const char* szDoc ) { SetDoc( szDoc ); m_nFlags=0; };
    CMarkupSTL( const CMarkupSTL& markup ) { *this = markup; };
    void operator=( const CMarkupSTL& markup );
    ~CMarkupSTL() {};

    // Navigate
    bool Load( const char* szFileName );
    bool SetDoc( const char* szDoc );
    bool IsWellFormed();
    bool FindElem( const char* szName=NULL );
    bool FindChildElem( const char* szName=NULL );
    bool IntoElem();
    bool OutOfElem();
    void ResetChildPos() { x_SetPos(m_iPosParent,m_iPos,0); };
    void ResetMainPos() { x_SetPos(m_iPosParent,0,0); };
    void ResetPos() { x_SetPos(0,0,0); };
    std::string GetTagName() const;
    std::string GetChildTagName() const { return x_GetTagName(m_iPosChild); };
    std::string GetData() const { return x_GetData(m_iPos); };
    std::string GetChildData() const { return x_GetData(m_iPosChild); };
    std::string GetAttrib( const char* szAttrib ) const { return x_GetAttrib(m_iPos,szAttrib); };
    std::string GetChildAttrib( const char* szAttrib ) const { return x_GetAttrib(m_iPosChild,szAttrib); };
    std::string GetAttribName( int n ) const;
    bool SavePos( const char* szPosName="" );
    bool RestorePos( const char* szPosName="" );
    const std::string& GetError() const { return m_strError; };

    enum MarkupNodeType
    {
        MNT_ELEMENT                    = 1,  // 0x01
        MNT_TEXT                    = 2,  // 0x02
        MNT_WHITESPACE                = 4,  // 0x04
        MNT_CDATA_SECTION            = 8,  // 0x08
        MNT_PROCESSING_INSTRUCTION    = 16, // 0x10
        MNT_COMMENT                    = 32, // 0x20
        MNT_DOCUMENT_TYPE            = 64, // 0x40
        MNT_EXCLUDE_WHITESPACE        = 123,// 0x7b
    };

    // Create
    bool Save( const char* szFileName );
    const std::string& GetDoc() const { return m_strDoc; };
    bool AddElem( const char* szName, const char* szData=NULL ) { return x_AddElem(szName,szData,false,false); };
    bool InsertElem( const char* szName, const char* szData=NULL ) { return x_AddElem(szName,szData,true,false); };
    bool AddChildElem( const char* szName, const char* szData=NULL ) { return x_AddElem(szName,szData,false,true); };
    bool InsertChildElem( const char* szName, const char* szData=NULL ) { return x_AddElem(szName,szData,true,true); };
    bool AddAttrib( const char* szAttrib, const char* szValue ) { return x_SetAttrib(m_iPos,szAttrib,szValue); };
    bool AddChildAttrib( const char* szAttrib, const char* szValue ) { return x_SetAttrib(m_iPosChild,szAttrib,szValue); };
    bool AddAttrib( const char* szAttrib, int nValue ) { return x_SetAttrib(m_iPos,szAttrib,nValue); };
    bool AddChildAttrib( const char* szAttrib, int nValue ) { return x_SetAttrib(m_iPosChild,szAttrib,nValue); };
    bool AddSubDoc( const char* szSubDoc ) { return x_AddSubDoc(szSubDoc,false,false); };
    bool InsertSubDoc( const char* szSubDoc ) { return x_AddSubDoc(szSubDoc,true,false); };
    std::string GetSubDoc() const { return x_GetSubDoc(m_iPos); };
    bool AddChildSubDoc( const char* szSubDoc ) { return x_AddSubDoc(szSubDoc,false,true); };
    bool InsertChildSubDoc( const char* szSubDoc ) { return x_AddSubDoc(szSubDoc,true,true); };
    std::string GetChildSubDoc() const { return x_GetSubDoc(m_iPosChild); };

    // Modify
    bool RemoveElem();
    bool RemoveChildElem();
    bool SetAttrib( const char* szAttrib, const char* szValue ) { return x_SetAttrib(m_iPos,szAttrib,szValue); };
    bool SetChildAttrib( const char* szAttrib, const char* szValue ) { return x_SetAttrib(m_iPosChild,szAttrib,szValue); };
    bool SetAttrib( const char* szAttrib, int nValue ) { return x_SetAttrib(m_iPos,szAttrib,nValue); };
    bool SetChildAttrib( const char* szAttrib, int nValue ) { return x_SetAttrib(m_iPosChild,szAttrib,nValue); };
    bool SetData( const char* szData, int nCDATA=0 ) { return x_SetData(m_iPos,szData,nCDATA); };
    bool SetChildData( const char* szData, int nCDATA=0 ) { return x_SetData(m_iPosChild,szData,nCDATA); };


protected:

#ifdef _DEBUG
    const char* m_pMainDS;
    const char* m_pChildDS;
#endif

    std::string m_strDoc;
    std::string m_strError;

    int m_iPosParent;
    int m_iPos;
    int m_iPosChild;
    int m_iPosFree;
    int m_iPosDeleted;
    int m_nNodeType;
    int m_nNodeOffset;
    int m_nNodeLength;
    int m_nFlags;

    struct ElemPos
    {
        ElemPos() { Clear(); };
        ElemPos( const ElemPos& pos ) { *this = pos; };
        enum { EP_STBITS=22, EP_STMASK=0x2fffff, EP_LEVMASK=0xffff };
        int StartTagLen() const { return (nTagLengths & EP_STMASK); };
        void SetStartTagLen( int n ) { nTagLengths = (nTagLengths & ~EP_STMASK) + n; };
        void AdjustStartTagLen( int n ) { nTagLengths += n; };
        int EndTagLen() const { return (nTagLengths >> EP_STBITS); };
        void SetEndTagLen( int n ) { nTagLengths = (nTagLengths & EP_STMASK) + (n << EP_STBITS); };
        void AdjustEndTagLen( int n ) { nTagLengths += (n << EP_STBITS); };
        int StartContent() const { return nStart + StartTagLen(); };
        int ContentLen() const { return nLength - StartTagLen() - EndTagLen(); };
        int StartAfter() const { return nStart + nLength; };
        int EndL() const { return nStart + nLength - EndTagLen(); };
        int Level() const { return nFlags & EP_LEVMASK; };
        void SetLevel( int nLev ) { nFlags = (nFlags & ~EP_LEVMASK) | nLev; };
        void Clear() { memset( this, 0, sizeof(ElemPos) ); };

        // Memory size: 8 32-bit integers == 32 bytes
        int nStart;
        int nLength;
        int nTagLengths; // 22 bits 4MB limit for start tag, 10 bits 1K limit for end tag
        int nFlags; // 16 bits flags, 16 bits level 65536 depth limit
        int iElemParent;
        int iElemChild; // first child
        int iElemNext;
        int iElemPrev; // if this is first child, iElemPrev points to last
    };

    enum MarkupNodeFlags
    {
        MNF_EMPTY   = 0x010000,
        MNF_DELETED = 0x020000,
        MNF_ERROR   = 0x040000,
        MNF_FIRST   = 0x080000,
    };

    struct NodePos
    {
        int nNodeType;
        int nStart;
        int nLength;
        int nFlags;
        std::string strName;
    };

    struct TokenPos
    {
        TokenPos( const char* sz ) { Clear(); szDoc = sz; };
        void Clear() { nL=0; nR=-1; nNext=0; bIsString=false; };
        int Length() const { return nR - nL + 1; };
        bool Match( const char* szName )
        {
            int nLen = nR - nL + 1;
        // To ignore case, define MARKUP_IGNORECASE
        #ifdef MARKUP_IGNORECASE
            return ( (strnicmp( &szDoc[nL], szName, nLen ) == 0)
        #else
            return ( (strncmp( &szDoc[nL], szName, nLen ) == 0)
        #endif
                && ( szName[nLen] == '\0' || strchr(" =/[",szName[nLen]) ) );
        };
        int nL;
        int nR;
        int nNext;
        const char* szDoc;
        bool bIsString;
    };

    struct SavedPos
    {
        SavedPos() { nSavedPosFlags=0; iPos=0; };
        std::string strName;
        int iPos;
        int nSavedPosFlags;
    };

    struct SavedPosMap
    {
        SavedPosMap() { pTable = NULL; };
        ~SavedPosMap() { RemoveAll(); };
        void RemoveAll() { if (pTable) Release(); pTable=NULL; };
        enum { SPM_SIZE = 7, SPM_MAIN = 1, SPM_CHILD = 2, SPM_USED = 4, SPM_LAST = 8 };
        void Release() { for (int n=0;n<SPM_SIZE;++n) if (pTable[n]) delete[] pTable[n]; delete[] pTable; };
        void AllocMapTable() { pTable = new SavedPos*[SPM_SIZE]; for (int n=0; n<SPM_SIZE; ++n) pTable[n]=NULL; };
        int Hash( const char* szName ) { int n=0; while (*szName) n += *szName++; return n % SPM_SIZE; };
        SavedPos** pTable;
    };
    SavedPosMap m_mapSavedPos;

    struct PosArray
    {
        PosArray() { Clear(); };
        ~PosArray() { Release(); };
        enum { PA_SEGBITS = 16, PA_SEGMASK = 0xffff };
        void RemoveAll() { Release(); Clear(); };
        void Release() { for (int n=0;n<SegsUsed();++n) delete[] (char*)pSegs[n]; if (pSegs) delete[] (char*)pSegs; };
        void Clear() { nSegs=0; nSize=0; pSegs=NULL; };
        int GetSize() const { return nSize; };
        int SegsUsed() const { return ((nSize-1)>>PA_SEGBITS) + 1; };
        ElemPos& operator[](int n) const { return pSegs[n>>PA_SEGBITS][n&PA_SEGMASK]; };
        ElemPos** pSegs;
        int nSize;
        int nSegs;
    };
    PosArray m_aPos;

    struct NodeStack
    {
        NodeStack() { nTop=-1; nSize=0; pN=NULL; };
        ~NodeStack() { if (pN) delete [] pN; };
        NodePos& Top() { return pN[nTop]; };
        void Add() { ++nTop; if (nTop==nSize) Alloc(nSize*2+6); };
        void Remove() { --nTop; };
        int TopIndex() { return nTop; };
    protected:
        void Alloc( int nNewSize ) { NodePos* pNNew = new NodePos[nNewSize]; Copy(pNNew); nSize=nNewSize; };
        void Copy( NodePos* pNNew ) { for(int n=0;n<nSize;++n) pNNew[n]=pN[n]; if (pN) delete [] pN; pN=pNNew; };
        NodePos* pN;
        int nSize;
        int nTop;
    };

    void x_SetPos( int iPosParent, int iPos, int iPosChild )
    {
        m_iPosParent = iPosParent;
        m_iPos = iPos;
        m_iPosChild = iPosChild;
        m_nNodeOffset = 0;
        m_nNodeLength = 0;
        m_nNodeType = iPos?MNT_ELEMENT:0;
        MARKUP_SETDEBUGSTATE;
    };
    int x_GetFreePos()
    {
        if ( m_iPosFree == m_aPos.GetSize() )
            x_AllocPosArray();
        return m_iPosFree++;
    };
    bool x_AllocPosArray( int nNewSize = 0 );
    bool x_ParseDoc();
    int x_ParseElem( int iPos, TokenPos& token );
    static bool x_FindChar( const char* szDoc, int& nChar, char c );
    static bool x_FindAny( const char* szDoc, int& nChar );
    static bool x_FindToken( TokenPos& token );
    static std::string x_GetToken( const TokenPos& token );
    int x_FindElem( int iPosParent, int iPos, const char* szPath );
    std::string x_GetTagName( int iPos ) const;
    std::string x_GetData( int iPos ) const;
    std::string x_GetAttrib( int iPos, const char* szAttrib ) const;
    bool x_AddElem( const char* szName, const char* szValue, bool bInsert, bool bAddChild );
    std::string x_GetSubDoc( int iPos ) const;
    bool x_AddSubDoc( const char* szSubDoc, bool bInsert, bool bAddChild );
    static bool x_FindAttrib( TokenPos& token, const char* szAttrib=NULL );
    bool x_SetAttrib( int iPos, const char* szAttrib, const char* szValue );
    bool x_SetAttrib( int iPos, const char* szAttrib, int nValue );
    static bool x_CreateNode( std::string& strNode, int nNodeType, const char* szText );
    void x_LocateNew( int iPosParent, int& iPosRel, int& nOffset, int nLength, int nFlags );
    void x_LinkElem( int iPosParent, int iPosBefore, int iPos );
    int x_UnlinkElem( int iPos );
    static int x_ParseNode( TokenPos& token, NodePos& node );
    bool x_SetData( int iPos, const char* szData, int nCDATA );
    int x_RemoveElem( int iPos );
    void x_DocChange( int nLeft, int nReplace, const std::string& strInsert );
    void x_Adjust( int iPos, int nShift, bool bAfterPos = false );
    static std::string x_TextToDoc( const char* szText, bool bAttrib = false );
    std::string x_TextFromDoc( int nLeft, int nCopy ) const;
};

#endif // !defined(AFX_MARKUPSTL_H__948A2705_9E68_11D2_A0BF_00105A27C570__INCLUDED_)
