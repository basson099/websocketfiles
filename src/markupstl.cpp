// MarkupSTL.cpp: implementation of the CMarkupSTL class.
//
// Markup Release 7.0
// Copyright (C) 1999-2004 First Objective Software, Inc. All rights reserved
// Go to www.firstobject.com for the latest CMarkup and EDOM documentation
// Use in commercial applications requires written permission
// This software is provided "as is", with no warranty.

#include <stdio.h>
#include <errno.h>

#include "markupstl.h"

using namespace std;

// Customization
#define x_EOL "\r\n" // can be \r\n or \n or empty
#define x_EOLLEN (sizeof(x_EOL)-1) // string length of x_EOL
#define x_ATTRIBQUOTE "\"" // can be double or single quote

//typedef int ssize_t;

void CMarkupSTL::operator=( const CMarkupSTL& markup )
{
    m_iPosParent = markup.m_iPosParent;
    m_iPos = markup.m_iPos;
    m_iPosChild = markup.m_iPosChild;
    m_iPosFree = markup.m_iPosFree;
    m_iPosDeleted = markup.m_iPosDeleted;
    m_nNodeType = markup.m_nNodeType;
    m_nNodeOffset = markup.m_nNodeOffset;
    m_nNodeLength = markup.m_nNodeLength;
    m_strDoc = markup.m_strDoc;
    m_strError = markup.m_strError;
    m_nFlags = markup.m_nFlags;

    // Copy used part of the index array
    m_aPos.RemoveAll();
    m_aPos.nSize = m_iPosFree;
    m_aPos.nSegs = m_aPos.SegsUsed();
    if ( m_aPos.nSegs )
    {
        m_aPos.pSegs = (ElemPos**)(new char[m_aPos.nSegs*sizeof(char*)]);
        int nSegSize = 1 << m_aPos.PA_SEGBITS;
        for ( int nSeg=0; nSeg < m_aPos.nSegs; ++nSeg )
        {
            if ( nSeg + 1 == m_aPos.nSegs )
                nSegSize = m_aPos.GetSize() - (nSeg << m_aPos.PA_SEGBITS);
            m_aPos.pSegs[nSeg] = (ElemPos*)(new char[nSegSize*sizeof(ElemPos)]);
            memcpy( m_aPos.pSegs[nSeg], markup.m_aPos.pSegs[nSeg], nSegSize*sizeof(ElemPos) );
        }
    }

    // Copy SavedPos map
    m_mapSavedPos.RemoveAll();
    if ( markup.m_mapSavedPos.pTable )
    {
        m_mapSavedPos.AllocMapTable();
        for ( int nSlot=0; nSlot < SavedPosMap::SPM_SIZE; ++nSlot )
        {
            SavedPos* pCopySavedPos = markup.m_mapSavedPos.pTable[nSlot];
            if ( pCopySavedPos )
            {
                int nCount = 0;
                while ( pCopySavedPos[nCount].nSavedPosFlags & SavedPosMap::SPM_USED )
                {
                    ++nCount;
                    if ( pCopySavedPos[nCount-1].nSavedPosFlags & SavedPosMap::SPM_LAST )
                        break;
                }
                SavedPos* pNewSavedPos = new SavedPos[nCount];
                for ( int nCopy=0; nCopy<nCount; ++nCopy )
                    pNewSavedPos[nCopy] = pCopySavedPos[nCopy];
                pNewSavedPos[nCount-1].nSavedPosFlags |= SavedPosMap::SPM_LAST;
                m_mapSavedPos.pTable[nSlot] = pNewSavedPos;
            }
        }
    }

    MARKUP_SETDEBUGSTATE;
}

bool CMarkupSTL::SetDoc( const char* szDoc )
{
    // Set document text
    if ( szDoc )
        m_strDoc = szDoc;
    else
        m_strDoc.erase();

    m_strError.erase();
    return x_ParseDoc();
};

bool CMarkupSTL::IsWellFormed()
{
    if ( m_aPos.GetSize() && m_aPos[0].iElemChild )
        return true;
    return false;
}

bool CMarkupSTL::Load( const char* szFileName )
{
    // Open file to read binary
    FILE* fp = fopen( szFileName, "rb" );
    if ( ! fp )
    {
        m_strError = strerror(errno);
        return false;
    }

    // Determine file length
    fseek( fp, 0, SEEK_END );
    int nFileByteLen = ftell(fp);
    fseek( fp, 0, SEEK_SET );

    char szResult[100];


    // Read file directly
    char* pszBuffer = new char[nFileByteLen];
    size_t ret = fread( pszBuffer, nFileByteLen, 1, fp );
    m_strDoc.assign( pszBuffer, nFileByteLen );
    delete [] pszBuffer;
    sprintf( szResult, "%d bytes", nFileByteLen );
    m_strError = szResult;

    fclose( fp );
    return x_ParseDoc();
}

bool CMarkupSTL::Save( const char* szFileName )
{
    // Open file to write binary
    bool bSuccess = false;
    FILE* fp = fopen( szFileName, "wb" );
    if ( ! fp )
    {
        m_strError = strerror(errno);
        return false;
    }

    // Get length of document
    int nLength = (int)m_strDoc.size();
    if ( ! nLength )
    {
        fclose(fp);
        return true;
    }

    char szResult[100];


    bSuccess = ( fwrite( m_strDoc.c_str(), nLength, 1, fp ) == 1 );
    sprintf( szResult, "%d bytes", nLength );
    m_strError = szResult;
    
    if ( ! bSuccess )
        m_strError = strerror(errno);
    fclose(fp);
    return bSuccess;
}

bool CMarkupSTL::FindElem( const char* szName )
{
    // Change current position only if found
    //
    if ( m_aPos.GetSize() )
    {
        int iPos = x_FindElem( m_iPosParent, m_iPos, szName );
        if ( iPos )
        {
            // Assign new position
            x_SetPos( m_aPos[iPos].iElemParent, iPos, 0 );
            return true;
        }
    }
    return false;
}

bool CMarkupSTL::FindChildElem( const char* szName )
{
    // Change current child position only if found
    //
    // Shorthand: call this with no current main position
    // means find child under root element
    if ( ! m_iPos )
        FindElem();

    int iPosChild = x_FindElem( m_iPos, m_iPosChild, szName );
    if ( iPosChild )
    {
        // Assign new position
        int iPos = m_aPos[iPosChild].iElemParent;
        x_SetPos( m_aPos[iPos].iElemParent, iPos, iPosChild );
        return true;
    }

    return false;
}


string CMarkupSTL::GetTagName() const
{
    // Return the tag name at the current main position
    string strTagName;


    if ( m_iPos )
        strTagName = x_GetTagName( m_iPos );
    return strTagName;
}

bool CMarkupSTL::IntoElem()
{
    // If there is no child position and IntoElem is called it will succeed in release 6.3
    // (A subsequent call to FindElem will find the first element)
    // The following short-hand behavior was never part of EDOM and was misleading
    // It would find a child element if there was no current child element position and go into it
    // It is removed in release 6.3, this change is NOT backwards compatible!
    // if ( ! m_iPosChild )
    //    FindChildElem();

    if ( m_iPos && m_nNodeType == MNT_ELEMENT )
    {
        x_SetPos( m_iPos, m_iPosChild, 0 );
        return true;
    }
    return false;
}

bool CMarkupSTL::OutOfElem()
{
    // Go to parent element
    if ( m_iPosParent )
    {
        x_SetPos( m_aPos[m_iPosParent].iElemParent, m_iPosParent, m_iPos );
        return true;
    }
    return false;
}

string CMarkupSTL::GetAttribName( int n ) const
{
    // Return nth attribute name of main position
    TokenPos token( m_strDoc.c_str() );
    if ( m_iPos && m_nNodeType == MNT_ELEMENT )
        token.nNext = m_aPos[m_iPos].nStart + 1;
    else if ( m_nNodeLength && m_nNodeType == MNT_PROCESSING_INSTRUCTION )
        token.nNext = m_nNodeOffset + 2;
    else
        return "";

    for ( int nAttrib=0; nAttrib<=n; ++nAttrib )
        if ( ! x_FindAttrib(token) )
            return "";

    // Return substring of document
    return x_GetToken( token );
}

bool CMarkupSTL::SavePos( const char* szPosName )
{
    // Save current element position in saved position map
    if ( szPosName )
    {
        SavedPos savedpos;
        if ( szPosName )
            savedpos.strName = szPosName;
        if ( m_iPosChild )
        {
            savedpos.iPos = m_iPosChild;
            savedpos.nSavedPosFlags |= SavedPosMap::SPM_CHILD;
        }
        else if ( m_iPos )
        {
            savedpos.iPos = m_iPos;
            savedpos.nSavedPosFlags |= SavedPosMap::SPM_MAIN;
        }
        else
        {
            savedpos.iPos = m_iPosParent;
        }
        savedpos.nSavedPosFlags |= SavedPosMap::SPM_USED;

        if ( ! m_mapSavedPos.pTable )
            m_mapSavedPos.AllocMapTable();
        int nSlot = m_mapSavedPos.Hash( szPosName );
        SavedPos* pSavedPos = m_mapSavedPos.pTable[nSlot];
        int nOffset = 0;
        if ( ! pSavedPos )
        {
            pSavedPos = new SavedPos[2];
            pSavedPos[1].nSavedPosFlags = SavedPosMap::SPM_LAST;
            m_mapSavedPos.pTable[nSlot] = pSavedPos;
        }
        else
        {
            while ( pSavedPos[nOffset].nSavedPosFlags & SavedPosMap::SPM_USED )
            {
                if ( pSavedPos[nOffset].strName == szPosName )
                    break;
                if ( pSavedPos[nOffset].nSavedPosFlags & SavedPosMap::SPM_LAST )
                {
                    int nNewSize = (nOffset + 6) * 2;
                    SavedPos* pNewSavedPos = new SavedPos[nNewSize];
                    for ( int nCopy=0; nCopy<=nOffset; ++nCopy )
                        pNewSavedPos[nCopy] = pSavedPos[nCopy];
                    pNewSavedPos[nOffset].nSavedPosFlags ^= SavedPosMap::SPM_LAST;
                    pNewSavedPos[nNewSize-1].nSavedPosFlags = SavedPosMap::SPM_LAST;
                    delete [] pSavedPos;
                    pSavedPos = pNewSavedPos;
                    m_mapSavedPos.pTable[nSlot] = pSavedPos;
                    ++nOffset;
                    break;
                }
                ++nOffset;
            }
        }
        if ( pSavedPos[nOffset].nSavedPosFlags & SavedPosMap::SPM_LAST )
            savedpos.nSavedPosFlags |= SavedPosMap::SPM_LAST;
        pSavedPos[nOffset] = savedpos;

        /*
        // To review hash table balance, uncomment and watch strBalance
        string strBalance;
        char szSlot[20];
        for ( nSlot=0; nSlot < SavedPosMap::SPM_SIZE; ++nSlot )
        {
            pSavedPos = m_mapSavedPos.pTable[nSlot];
            int nCount = 0;
            while ( pSavedPos && pSavedPos->nSavedPosFlags & SavedPosMap::SPM_USED )
            {
                ++nCount;
                if ( pSavedPos->nSavedPosFlags & SavedPosMap::SPM_LAST )
                    break;
                ++pSavedPos;
            }
            sprintf( szSlot, "%d ", nCount );
            strBalance += szSlot;
        }
        */

        return true;
    }
    return false;
}

bool CMarkupSTL::RestorePos( const char* szPosName )
{
    // Restore element position if found in saved position map
    if ( szPosName )
    {
        int nSlot = m_mapSavedPos.Hash( szPosName );
        SavedPos* pSavedPos = m_mapSavedPos.pTable[nSlot];
        if ( pSavedPos )
        {
            int nOffset = 0;
            while ( pSavedPos[nOffset].nSavedPosFlags & SavedPosMap::SPM_USED )
            {
                if ( pSavedPos[nOffset].strName == szPosName )
                {
                    int i = pSavedPos[nOffset].iPos;
                    if ( pSavedPos[nOffset].nSavedPosFlags & SavedPosMap::SPM_CHILD )
                        x_SetPos( m_aPos[m_aPos[i].iElemParent].iElemParent, m_aPos[i].iElemParent, i );
                    else if ( pSavedPos[nOffset].nSavedPosFlags & SavedPosMap::SPM_MAIN )
                        x_SetPos( m_aPos[i].iElemParent, i, 0 );
                    else
                        x_SetPos( i, 0, 0 );
                    return true;
                }
                if ( pSavedPos[nOffset].nSavedPosFlags & SavedPosMap::SPM_LAST )
                    break;
                ++nOffset;
            }
        }
    }
    return false;
}

bool CMarkupSTL::RemoveElem()
{
    // Remove current main position element
    if ( m_iPos && m_nNodeType == MNT_ELEMENT )
    {
        int iPos = x_RemoveElem( m_iPos );
        x_SetPos( m_iPosParent, iPos, 0 );
        return true;
    }
    return false;
}

bool CMarkupSTL::RemoveChildElem()
{
    // Remove current child position element
    if ( m_iPosChild )
    {
        int iPosChild = x_RemoveElem( m_iPosChild );
        x_SetPos( m_iPosParent, m_iPos, iPosChild );
        return true;
    }
    return false;
}


//////////////////////////////////////////////////////////////////////
// Private Methods
//////////////////////////////////////////////////////////////////////

bool CMarkupSTL::x_AllocPosArray( int nNewSize /*=0*/ )
{
    // Resize m_aPos when the document is created or the array is filled
    // The PosArray class is implemented using segments to reduce contiguous memory requirements
    // It reduces reallocations (copying of memory) since this only occurs within one segment
    // The "Grow By" algorithm ensures there are no reallocations after 2 segments
    //
    if ( ! nNewSize )
        nNewSize = m_iPosFree + (m_iPosFree>>1); // Grow By: multiply size by 1.5
    if ( m_aPos.GetSize() < nNewSize )
    {
        // Grow By: new size can be at most one more complete segment
        int nSeg = (m_aPos.GetSize()?m_aPos.GetSize()-1:0) >> m_aPos.PA_SEGBITS;
        int nNewSeg = (nNewSize-1) >> m_aPos.PA_SEGBITS;
        if ( nNewSeg > nSeg + 1 )
        {
            nNewSeg = nSeg + 1;
            nNewSize = (nNewSeg+1) << m_aPos.PA_SEGBITS;
        }

        // Allocate array of segments
        if ( m_aPos.nSegs <= nNewSeg )
        {
            int nNewSegments = 4 + nNewSeg * 2;
            char* pNewSegments = new char[nNewSegments*sizeof(char*)];
            if ( m_aPos.SegsUsed() )
                memcpy( pNewSegments, m_aPos.pSegs, m_aPos.SegsUsed()*sizeof(char*) );
            if ( m_aPos.pSegs )
                delete[] (char*)m_aPos.pSegs;
            m_aPos.pSegs = (ElemPos**)pNewSegments;
            m_aPos.nSegs = nNewSegments;
        }

        // Calculate segment sizes
        int nSegSize = m_aPos.GetSize() - (nSeg << m_aPos.PA_SEGBITS);
        int nNewSegSize = nNewSize - (nNewSeg << m_aPos.PA_SEGBITS);

        // Complete first segment
        int nFullSegSize = 1 << m_aPos.PA_SEGBITS;
        if ( nSeg < nNewSeg && nSegSize < nFullSegSize )
        {
            char* pNewFirstSeg = new char[ nFullSegSize * sizeof(ElemPos) ];
            if ( nSegSize )
            {
                // Reallocate
                memcpy( pNewFirstSeg, m_aPos.pSegs[nSeg], nSegSize * sizeof(ElemPos) );
                delete[] (char*)m_aPos.pSegs[nSeg];
            }
            m_aPos.pSegs[nSeg] = (ElemPos*)pNewFirstSeg;
        }

        // New segment
        char* pNewSeg = new char[ nNewSegSize * sizeof(ElemPos) ];
        if ( nNewSeg == nSeg && nSegSize )
        {
            // Reallocate
            memcpy( pNewSeg, m_aPos.pSegs[nSeg], nSegSize * sizeof(ElemPos) );
            delete[] (char*)m_aPos.pSegs[nSeg];
        }
        m_aPos.pSegs[nNewSeg] = (ElemPos*)pNewSeg;
        m_aPos.nSize = nNewSize;
    }
    return true;
}

bool CMarkupSTL::x_ParseDoc()
{
    // Preserve pre-parse result
    string strResult = m_strError;

    // Reset indexes
    ResetPos();
    m_mapSavedPos.RemoveAll();

    // Starting size of position array: 1 element per 64 bytes of document
    // Tight fit when parsing small doc, only 0 to 2 reallocs when parsing large doc
    // Start at 8 when creating new document
    m_iPosFree = 1;
    x_AllocPosArray( (int)m_strDoc.size() / 64 + 8 );
    m_iPosDeleted = 0;

    // Parse document
    bool bWellFormed = false;
    m_strError.erase();
    if ( m_strDoc.size() )
    {
        TokenPos token( m_strDoc.c_str() );
        m_aPos[0].Clear();
        int iPos = x_ParseElem( 0, token );
        if ( iPos > 0 )
        {
            m_aPos[0].iElemChild = iPos;
            m_aPos[0].nLength = (int)m_strDoc.size();
            bWellFormed = true;
        }
    }

    // Clear indexes if parse failed or empty document
    if ( ! bWellFormed )
    {
        m_aPos[0].Clear();
        m_iPosFree = 1;
    }

    ResetPos();

    // Combine preserved result with parse error
    if ( ! strResult.empty() )
    {
        if ( m_strError.empty() )
            m_strError = strResult;
        else
            m_strError = strResult + ", " + m_strError;
    }

    return bWellFormed;
};

int CMarkupSTL::x_ParseElem( int iPosParent, TokenPos& token )
{
    // This is either called by x_ParseDoc or x_AddSubDoc
    // This returns the new position if a tag is found, otherwise zero
    // In all cases we need to get a new ElemPos, but release it if unused
    //
    int iElemRoot = 0;
    int iPos = iPosParent;
    int nRootDepth = m_aPos[iPos].Level();
    token.nNext = 0;

    // Loop through the nodes of the document
    NodeStack aNodes;
    aNodes.Add();
    int nDepth = 0;
    int nTypeFound = 0;
    ElemPos* pElem;
    int iElemFirst, iElemLast;
    while ( nTypeFound >= 0 )
    {
        nTypeFound = x_ParseNode( token, aNodes.Top() );
        if ( nTypeFound == MNT_ELEMENT ) // start tag
        {
            iPos = x_GetFreePos();
            if ( ! iElemRoot )
                iElemRoot = iPos;
            else if ( nDepth == 0 )
            {
                char* szError = new char[aNodes.Top().strName.size()+100];
                sprintf( szError, "Element '%s' at offset %d is sibling to root",
                    aNodes.Top().strName.c_str(), aNodes.Top().nStart );
                m_strError = szError;
                delete [] szError;
                return -1;
            }
            pElem = &m_aPos[iPos];
            pElem->iElemParent = iPosParent;
            pElem->iElemNext = 0;
            if ( m_aPos[iPosParent].iElemChild )
            {
                iElemFirst = m_aPos[iPosParent].iElemChild;
                iElemLast = m_aPos[iElemFirst].iElemPrev;
                m_aPos[iElemLast].iElemNext = iPos;
                pElem->iElemPrev = iElemLast;
                m_aPos[iElemFirst].iElemPrev = iPos;
                pElem->nFlags = 0;
            }
            else
            {
                m_aPos[iPosParent].iElemChild = iPos;
                pElem->iElemPrev = iPos;
                pElem->nFlags = MNF_FIRST;
            }
            pElem->SetLevel( nRootDepth + nDepth );
            pElem->iElemChild = 0;
            pElem->nStart = aNodes.Top().nStart;
            pElem->SetStartTagLen( aNodes.Top().nLength );
            if ( aNodes.Top().nFlags & MNF_EMPTY )
            {
                iPos = iPosParent;
                pElem->nFlags |= MNF_EMPTY;
                pElem->SetEndTagLen( 2 );
                pElem->nLength = aNodes.Top().nLength;
            }
            else
            {
                iPosParent = iPos;
                ++nDepth;
                aNodes.Add();
            }
        }
        else if ( nTypeFound == 0 ) // end tag
        {
            if ( aNodes.TopIndex() == 0 )
            {
                char* szError = new char[token.Length()+100];
                sprintf( szError, "No start tag for end tag '%s' at offset %d",
                    x_GetToken(token).c_str(), aNodes.Top().nStart );
                m_strError = szError;
                delete [] szError;
                return -1;
            }
            pElem = &m_aPos[iPos];
            pElem->nLength = aNodes.Top().nStart - pElem->nStart + aNodes.Top().nLength;
            pElem->SetEndTagLen( aNodes.Top().nLength );
            aNodes.Remove();
            if ( ! token.Match(aNodes.Top().strName.c_str()) )
            {
                char* szError = new char[aNodes.Top().strName.size()+token.Length()+100];
                sprintf( szError, "End tag '%s' at offset %d does not match start tag '%s' at offset %d",
                    x_GetToken(token).c_str(), pElem->EndL(), aNodes.Top().strName.c_str(), pElem->nStart );
                m_strError = szError;
                delete [] szError;
                return -1;
            }
            --nDepth;
            iPosParent = pElem->iElemParent;
            iPos = iPosParent;
        }
    }

    if ( nTypeFound == -1 )
    {
        m_strError = aNodes.Top().strName;
        return -1;
    }
    else if ( nDepth > 0 )
    {
        aNodes.Remove();
        char* szError = new char[aNodes.Top().strName.size()+100];
        sprintf( szError, "Element '%s' at offset %d not ended",
            aNodes.Top().strName.c_str(), aNodes.Top().nStart );
        m_strError = szError;
        delete [] szError;
        return -1;
    }
    else if ( ! iElemRoot )
    {
        m_strError = "Root element not found";
        return 0;
    }

    // Successfully parsed element (and contained elements)
    return iElemRoot;
}


bool CMarkupSTL::x_FindChar( const char* szDoc, int& nChar, char c )
{
    // static function
    const char* pChar = &szDoc[nChar];
    while ( *pChar && *pChar != c )
        ++pChar;
    nChar = (int)( pChar - szDoc );
    if ( ! *pChar )
        return false;
    return true;
}

bool CMarkupSTL::x_FindAny( const char* szDoc, int& nChar )
{
    // Starting at nChar, find a non-whitespace char
    // return false if no non-whitespace before end of document, nChar points to end
    // otherwise return true and nChar points to non-whitespace char
    while ( szDoc[nChar] && strchr(" \t\n\r",szDoc[nChar]) )
        ++nChar;
    return szDoc[nChar] != '\0';
}

bool CMarkupSTL::x_FindToken( CMarkupSTL::TokenPos& token )
{
    // Starting at token.nNext, bypass whitespace and find the next token
    // returns true on success, members of token point to token
    // returns false on end of document, members point to end of document
    const char* szDoc = token.szDoc;
    int nChar = token.nNext;
    token.bIsString = false;

    // By-pass leading whitespace
    if ( ! x_FindAny(szDoc,nChar) )
    {
        // No token was found before end of document
        token.nL = nChar;
        token.nR = nChar - 1;
        token.nNext = nChar;
        return false;
    }

    // Is it an opening quote?
    char cFirstChar = szDoc[nChar];
    if ( cFirstChar == '\"' || cFirstChar == '\'' )
    {
        token.bIsString = true;

        // Move past opening quote
        ++nChar;
        token.nL = nChar;

        // Look for closing quote
        x_FindChar( token.szDoc, nChar, cFirstChar );

        // Set right to before closing quote
        token.nR = nChar - 1;

        // Set nChar past closing quote unless at end of document
        if ( szDoc[nChar] )
            ++nChar;
    }
    else
    {
        // Go until special char or whitespace
        token.nL = nChar;
        while ( szDoc[nChar] && ! strchr(" \t\n\r<>=\\/?!",szDoc[nChar]) )
            ++nChar;

        // Adjust end position if it is one special char
        if ( nChar == token.nL )
            ++nChar; // it is a special char
        token.nR = nChar - 1;
    }

    // nNext points to one past last char of token
    token.nNext = nChar;
    return true;
}

string CMarkupSTL::x_GetToken( const CMarkupSTL::TokenPos& token )
{
    // The token contains indexes into the document identifying a small substring
    // Build the substring from those indexes and return it
    if ( token.nL > token.nR )
        return "";
    string strToken( &token.szDoc[token.nL], token.Length() );
    return strToken;
}

int CMarkupSTL::x_FindElem( int iPosParent, int iPos, const char* szPath )
{
    // If szPath is NULL or empty, go to next sibling element
    // Otherwise go to next sibling element with matching path
    //
    if ( iPos )
        iPos = m_aPos[iPos].iElemNext;
    else
        iPos = m_aPos[iPosParent].iElemChild;

    // Finished here if szPath not specified
    if ( szPath == NULL || !szPath[0] )
        return iPos;

    // Search
    TokenPos token( m_strDoc.c_str() );
    while ( iPos )
    {
        // Compare tag name
        token.nNext = m_aPos[iPos].nStart + 1;
        x_FindToken( token ); // Locate tag name
        if ( token.Match(szPath) )
            return iPos;
        iPos = m_aPos[iPos].iElemNext;
    }
    return 0;

}

int CMarkupSTL::x_ParseNode( CMarkupSTL::TokenPos& token, CMarkupSTL::NodePos& node )
{
    // Call this with token.nNext set to the start of the node or tag
    // Upon return token.nNext points to the char after the node or tag
    // 
    // <!--...--> comment
    // <!DOCTYPE ...> dtd
    // <?target ...?> processing instruction
    // <![CDATA[...]]> cdata section
    // <NAME ...> element start tag
    // </NAME ...> element end tag
    //
    // returns the nodetype, or 0 for end tag, -1 for end of document, -2 for error
    //
    enum ParseBits
    {
        PD_OPENTAG = 1,
        PD_BANG = 2,
        PD_DASH = 4,
        PD_BRACKET = 8,
        PD_TEXTORWS = 16,
        PD_DOCTYPE = 32,
    };
    int nParseFlags = 0;

    const char* szFindEnd = NULL;
    int nNodeType = -1;
    int nEndLen = 0;
    int nName = 0;
    #define FINDNODETYPE(e,t,n) { szFindEnd=e; nEndLen=(sizeof(e)-1); nNodeType=t; if(n) nName=pDoc-token.szDoc+n-1; }
    #define ISNODEERROR(e) { char szE[100]; nNodeType=-1; sprintf(szE,"Incorrect %s at offset %d",e,nR); node.strName=szE; break; }

    node.nStart = token.nNext;
    node.nFlags = 0;

    int nR = token.nNext;
    const char* pDoc = &token.szDoc[nR];
    if ( ! *pDoc )
    {
        node.nLength = 0;
        node.nNodeType = 0;
        return -2; // end of document
    }

    while ( 1 )
    {
        if ( ! *pDoc )
        {
            nR = (int)( pDoc - token.szDoc - 1);
            if ( nNodeType != MNT_WHITESPACE && nNodeType != MNT_TEXT )
            {
                const char* szType = "tag";
                if ( (nParseFlags & PD_DOCTYPE) || nNodeType == MNT_DOCUMENT_TYPE )
                    szType = "Doctype";
                else if ( nNodeType == MNT_ELEMENT )
                    szType = "Element tag";
                else if ( nNodeType == 0 )
                    szType = "Element end tag";
                else if ( nNodeType == MNT_CDATA_SECTION )
                    szType = "CDATA Section";
                else if ( nNodeType == MNT_PROCESSING_INSTRUCTION )
                    szType = "Processing instruction";
                else if ( nNodeType == MNT_COMMENT )
                    szType = "Comment";
                nNodeType = -1;
                char szError[100];
                sprintf( szError, "%s at offset %d unterminated", szType, node.nStart );
                node.strName = szError;
            }
            break;
        }

        if ( nName )
        {
            if ( strchr(" \t\n\r/>",*pDoc) )
            {
                int nNameLen = (int)(pDoc - token.szDoc) - nName;
                if ( nNodeType == 0 )
                {
                    token.nL = nName;
                    token.nR = nName + nNameLen - 1;
                }
                else
                {
                    node.strName.assign( &token.szDoc[nName], nNameLen );
                }
                nName = 0;
            }
            else
            {
                ++pDoc;
                continue;
            }
        }

        if ( szFindEnd )
        {
            if ( *pDoc == '>' )
            {
                nR = (int)( pDoc - token.szDoc );
                if ( nEndLen == 1 )
                {
                    szFindEnd = NULL;
                    if ( nNodeType == MNT_ELEMENT && *(pDoc-1) == '/' )
                        node.nFlags |= MNF_EMPTY;
                }
                else if ( nR > nEndLen )
                {
                    // Test for end of PI or comment
                    const char* pEnd = pDoc - nEndLen + 1;
                    const char* pFindEnd = szFindEnd;
                    int nLen = nEndLen;
                    while ( --nLen && *pEnd++ == *pFindEnd++ );
                    if ( nLen == 0 )
                        szFindEnd = NULL;
                }
                if ( ! szFindEnd && ! (nParseFlags & PD_DOCTYPE) )
                    break;
            }
            else if ( *pDoc == '<' && nNodeType == MNT_TEXT )
            {
                nR = (int) ( pDoc - token.szDoc - 1 );
                break;
            }
        }
        else if ( nParseFlags )
        {
            if ( nParseFlags & PD_TEXTORWS )
            {
                if ( *pDoc == '<' )
                {
                    nR = (int)( pDoc - token.szDoc - 1 );
                    nNodeType = MNT_WHITESPACE;
                    break;
                }
                else if ( ! strchr(" \t\n\r",*pDoc) )
                {
                    nParseFlags ^= PD_TEXTORWS;
                    FINDNODETYPE( "<", MNT_TEXT, 0 )
                }
            }
            else if ( nParseFlags & PD_OPENTAG )
            {
                nParseFlags ^= PD_OPENTAG;
                if ( *pDoc > 0x60 || ( *pDoc > 0x40 && *pDoc < 0x5b ) )
                    FINDNODETYPE( ">", MNT_ELEMENT, 1 )
                else if ( *pDoc == '/' )
                    FINDNODETYPE( ">", 0, 2 )
                else if ( *pDoc == '!' )
                    nParseFlags |= PD_BANG;
                else if ( *pDoc == '?' )
                    FINDNODETYPE( "?>", MNT_PROCESSING_INSTRUCTION, 2 )
                else
                    ISNODEERROR( "tag name character" )
            }
            else if ( nParseFlags & PD_BANG )
            {
                nParseFlags ^= PD_BANG;
                if ( *pDoc == '-' )
                    nParseFlags |= PD_DASH;
                else if ( *pDoc == '[' && !(nParseFlags & PD_DOCTYPE) )
                    nParseFlags |= PD_BRACKET;
                else if ( *pDoc == 'D' && !(nParseFlags & PD_DOCTYPE) )
                    nParseFlags |= PD_DOCTYPE;
                else if ( strchr("EAN",*pDoc) ) // <!ELEMENT ATTLIST ENTITY NOTATION
                    FINDNODETYPE( ">", -1, 0 )
                else
                    ISNODEERROR( "! tag" )
            }
            else if ( nParseFlags & PD_DASH )
            {
                nParseFlags ^= PD_DASH;
                if ( *pDoc == '-' )
                    FINDNODETYPE( "-->", MNT_COMMENT, 0 )
                else
                    ISNODEERROR( "comment tag" )
            }
            else if ( nParseFlags & PD_BRACKET )
            {
                nParseFlags ^= PD_BRACKET;
                if ( *pDoc == 'C' )
                    FINDNODETYPE( "]]>", MNT_CDATA_SECTION, 0 )
                else
                    ISNODEERROR( "tag" )
            }
            else if ( nParseFlags & PD_DOCTYPE )
            {
                if ( *pDoc == '<' )
                    nParseFlags |= PD_OPENTAG;
                else if ( *pDoc == '>' )
                {
                    nR = pDoc - token.szDoc;
                    nNodeType = MNT_DOCUMENT_TYPE;
                    break;
                }
            }
        }
        else if ( *pDoc == '<' )
        {
            nParseFlags |= PD_OPENTAG;
        }
        else
        {
            nNodeType = MNT_WHITESPACE;
            if ( strchr(" \t\n\r",*pDoc) )
                nParseFlags |= PD_TEXTORWS;
            else
                FINDNODETYPE( "<", MNT_TEXT, 0 )
        }
        ++pDoc;
    }
    token.nNext = nR + 1;
    node.nLength = token.nNext - node.nStart;
    node.nNodeType = nNodeType;
    return nNodeType;
}

string CMarkupSTL::x_GetTagName( int iPos ) const
{
    // Return the tag name at specified element
    TokenPos token( m_strDoc.c_str() );
    token.nNext = m_aPos[iPos].nStart + 1;
    if ( ! iPos || ! x_FindToken( token ) )
        return "";

    // Return substring of document
    return x_GetToken( token );
}

bool CMarkupSTL::x_FindAttrib( CMarkupSTL::TokenPos& token, const char* szAttrib )
{
    // If szAttrib is NULL find next attrib, otherwise find named attrib
    // Return true if found
    int nAttrib = 0;
    for ( int nCount = 0; x_FindToken(token); ++nCount )
    {
        if ( ! token.bIsString )
        {
            // Is it the right angle bracket?
            char cChar = token.szDoc[token.nL];
            if ( cChar == '>' || cChar == '/' || cChar == '?' )
                break; // attrib not found

            // Equal sign
            if ( cChar == '=' )
                continue;

            // Potential attribute
            if ( ! nAttrib && nCount )
            {
                // Attribute name search?
                if ( ! szAttrib || ! szAttrib[0] )
                    return true; // return with token at attrib name

                // Compare szAttrib
                if ( token.Match(szAttrib) )
                    nAttrib = nCount;
            }
        }
        else if ( nAttrib && nCount == nAttrib + 2 )
        {
            return true;
        }
    }

    // Not found
    return false;
}

string CMarkupSTL::x_GetAttrib( int iPos, const char* szAttrib ) const
{
    // Return the value of the attrib
    TokenPos token( m_strDoc.c_str() );
    if ( iPos && m_nNodeType == MNT_ELEMENT )
        token.nNext = m_aPos[iPos].nStart + 1;
    else if ( iPos == m_iPos && m_nNodeLength && m_nNodeType == MNT_PROCESSING_INSTRUCTION )
        token.nNext = m_nNodeOffset + 2;
    else
        return "";

    if ( szAttrib && x_FindAttrib( token, szAttrib ) )
        return x_TextFromDoc( token.nL, token.Length() );
    return "";
}

bool CMarkupSTL::x_SetAttrib( int iPos, const char* szAttrib, int nValue )
{
    // Convert integer to string and call SetChildAttrib
    char szVal[25];
    sprintf( szVal, "%d", nValue );
    return x_SetAttrib( iPos, szAttrib, szVal );
}

bool CMarkupSTL::x_SetAttrib( int iPos, const char* szAttrib, const char* szValue )
{
    // Set attribute in iPos element
    TokenPos token( m_strDoc.c_str() );
    int nInsertAt;
    if ( iPos && m_nNodeType == MNT_ELEMENT )
    {
        token.nNext = m_aPos[iPos].nStart + 1;
        nInsertAt = m_aPos[iPos].StartContent() - ((m_aPos[iPos].nFlags & MNF_EMPTY)?2:1);
    }
    else if ( iPos == m_iPos && m_nNodeLength && m_nNodeType == MNT_PROCESSING_INSTRUCTION )
    {
        token.nNext = m_nNodeOffset + 2;
        nInsertAt = m_nNodeOffset + m_nNodeLength - 2;
    }
    else
        return false;

    // Create insertion text depending on whether attribute already exists
    int nReplace = 0;
    string strInsert;
    if ( x_FindAttrib( token, szAttrib ) )
    {
        // Replace value only
        // Decision: for empty value leaving attrib="" instead of removing attrib
        strInsert = x_TextToDoc( szValue, true );
        nInsertAt = token.nL;
        nReplace = token.Length();
    }
    else
    {
        // Insert string name value pair
        string strFormat;
        strFormat = " ";
        strFormat += szAttrib;
        strFormat += "=" x_ATTRIBQUOTE;
        strFormat += x_TextToDoc( szValue, true );
        strFormat += x_ATTRIBQUOTE;
        strInsert = strFormat;
    }

    x_DocChange( nInsertAt, nReplace, strInsert );
    int nAdjust = (int)strInsert.size() - nReplace;
    m_aPos[iPos].AdjustStartTagLen( nAdjust );
    m_aPos[iPos].nLength += nAdjust;
    x_Adjust( iPos, nAdjust );
    MARKUP_SETDEBUGSTATE;
    return true;
}


bool CMarkupSTL::x_CreateNode( string& strNode, int nNodeType, const char* szText )
{
    // Set strNode based on nNodeType and szData
    // Return false if szData would jeopardize well-formed document
    //
    switch ( nNodeType )
    {
    case MNT_CDATA_SECTION:
        if ( strstr(szText,"]]>") != NULL )
            return false;
        strNode = "<![CDATA[";
        strNode += szText;
        strNode += "]]>";
        break;
    }
    return true;
}

bool CMarkupSTL::x_SetData( int iPos, const char* szData, int nCDATA )
{
    // Set data at specified position
    // if nCDATA==1, set content of element to a CDATA Section
    string strInsert;


    // Set data in iPos element
    if ( ! iPos || m_aPos[iPos].iElemChild )
        return false;

    // Build strInsert from szData based on nCDATA
    // If CDATA section not valid, use parsed text (PCDATA) instead
    if ( nCDATA != 0 )
        if ( ! x_CreateNode(strInsert, MNT_CDATA_SECTION, szData) )
            nCDATA = 0;
    if ( nCDATA == 0 )
        strInsert = x_TextToDoc( szData );

    // Decide where to insert
    int nInsertAt, nReplace;
    if ( m_aPos[iPos].nFlags & MNF_EMPTY )
    {
        string strTagName = x_GetTagName( iPos );
        string strFormat;
        strFormat = ">";
        strFormat += strInsert;
        strFormat += "</";
        strFormat += strTagName;
        strInsert = strFormat;
        nInsertAt = m_aPos[iPos].EndL();
        nReplace = 1;
        m_aPos[iPos].AdjustStartTagLen( -1 );
        m_aPos[iPos].AdjustEndTagLen( 1 + (int)strTagName.size() );
        m_aPos[iPos].nFlags ^= MNF_EMPTY;
    }
    else
    {
        nInsertAt = m_aPos[iPos].StartContent();
        nReplace = m_aPos[iPos].ContentLen();
    }
    x_DocChange( nInsertAt, nReplace, strInsert );
    int nAdjust = (int)strInsert.size() - nReplace;
    x_Adjust( iPos, nAdjust );
    m_aPos[iPos].nLength += nAdjust;
    MARKUP_SETDEBUGSTATE;
    return true;
}

string CMarkupSTL::x_GetData( int iPos ) const
{

    // Return a string representing data between start and end tag
    // Return empty string if there are any children elements
    if ( ! m_aPos[iPos].iElemChild && ! (m_aPos[iPos].nFlags & MNF_EMPTY) )
    {
        // See if it is a CDATA section
        const char* szDoc = m_strDoc.c_str();
        int nChar = m_aPos[iPos].StartContent();
        if ( x_FindAny( szDoc, nChar ) && szDoc[nChar] == '<'
                && nChar + 11 < m_aPos[iPos].EndL()
                && strncmp( &szDoc[nChar], "<![CDATA[", 9 ) == 0 )
        {
            nChar += 9;
            int nEndCDATA = (int)m_strDoc.find( "]]>", nChar );
            if ( nEndCDATA != -1 && nEndCDATA < m_aPos[iPos].EndL() )
            {
                return m_strDoc.substr( nChar, nEndCDATA - nChar );
            }
        }
        return x_TextFromDoc( m_aPos[iPos].StartContent(), m_aPos[iPos].ContentLen() );
    }
    return "";
}

string CMarkupSTL::x_TextToDoc( const char* szText, bool bAttrib )
{
    // Convert text as seen outside XML document to XML friendly
    // replacing special characters with ampersand escape codes
    // E.g. convert "6>7" to "6&gt;7"
    //
    // &lt;   less than
    // &amp;  ampersand
    // &gt;   greater than
    //
    // and for attributes:
    //
    // &apos; apostrophe or single quote
    // &quot; double quote
    //
    static const char* szaReplace[] = { "&lt;","&amp;","&gt;","&apos;","&quot;" };
    const char* pFind = bAttrib?"<&>\'\"":"<&>";
    string strText;
    const char* pSource = szText;
    int nDestSize = (int)strlen(pSource);
    nDestSize += nDestSize / 10 + 7;
    strText.reserve( nDestSize );
    char cSource = *pSource;
    const char* pFound;
    while ( cSource )
    {
        if ( (pFound=strchr(pFind,cSource)) != NULL )
        {
            pFound = szaReplace[pFound-pFind];
            strText.append( pFound );
        }
        else
        {
            strText += cSource;
        }
        ++pSource;
        cSource = *pSource;
    }
    return strText;
}

string CMarkupSTL::x_TextFromDoc( int nLeft, int nCopy ) const
{
    // Convert XML friendly text to text as seen outside XML document
    // ampersand escape codes replaced with special characters e.g. convert "6&gt;7" to "6>7"
    // ampersand numeric codes replaced with character e.g. convert &#60; to <
    // Conveniently the result is always the same or shorter in byte length
    //
    static const char* szaCode[] = { "lt;","amp;","gt;","apos;","quot;" };
    static int anCodeLen[] = { 3,4,3,5,5 };
    static const char* szSymbol = "<&>\'\"";
    string strText;
    const char* pSource = m_strDoc.c_str();
    int nEnd = nLeft + nCopy;
    strText.reserve( nCopy );
    int nChar = nLeft;
    while ( nChar < nEnd )
    {
        if ( pSource[nChar] == '&' )
        {
            bool bCodeConverted = false;

            // Is it a numeric character reference?
            if ( pSource[nChar+1] == '#' )
            {
                // Is it a hex number?
                int nBase = 10;
                int nNumericChar = nChar + 2;
                char cChar = pSource[nNumericChar];
                if ( cChar == 'x' )
                {
                    ++nNumericChar;
                    cChar = pSource[nNumericChar];
                    nBase = 16;
                }

                // Look for terminating semi-colon within 7 characters
                int nCodeLen = 0;
                while ( nCodeLen < 7 && cChar && cChar != ';' )
                {
                    // only ASCII digits 0-9, A-F, a-f expected
                    ++nCodeLen;
                    cChar = pSource[nNumericChar + nCodeLen];
                }

                // Process unicode
                if ( cChar == ';' )
                {
                    int nUnicode = strtol( &pSource[nNumericChar], NULL, nBase );
                    /* MBCS
                    int nMBLen = wctomb( &pDest[nLen], (wchar_t)nUnicode );
                    if ( nMBLen > 0 )
                        nLen += nMBLen;
                    else
                        nUnicode = 0;
                    */
                    if ( nUnicode < 0x80 )
                        strText += (char)nUnicode;
                    else if ( nUnicode < 0x800 )
                    {
                        // Convert to 2-byte UTF-8
                        strText += (char)(((nUnicode&0x7c0)>>6) | 0xc0);
                        strText += (char)((nUnicode&0x3f) | 0x80);
                    }
                    else
                    {
                        // Convert to 3-byte UTF-8
                        strText += (char)(((nUnicode&0xf000)>>12) | 0xe0);
                        strText += (char)(((nUnicode&0xfc0)>>6) | 0x80);
                        strText += (char)((nUnicode&0x3f) | 0x80);
                    }
                    if ( nUnicode )
                    {
                        // Increment index past ampersand semi-colon
                        nChar = nNumericChar + nCodeLen + 1;
                        bCodeConverted = true;
                    }
                }
            }
            else // does not start with #
            {
                // Look for matching &code;
                for ( int nMatch = 0; nMatch < 5; ++nMatch )
                {
                    if ( nChar < nEnd - anCodeLen[nMatch]
                        && strncmp(szaCode[nMatch],&pSource[nChar+1],anCodeLen[nMatch]) == 0 )
                    {
                        // Insert symbol and increment index past ampersand semi-colon
                        strText += szSymbol[nMatch];
                        nChar += anCodeLen[nMatch] + 1;
                        bCodeConverted = true;
                        break;
                    }
                }
            }

            // If the code is not converted, leave it as is
            if ( ! bCodeConverted )
            {
                strText += '&';
                ++nChar;
            }
        }
        else // not &
        {
            strText += pSource[nChar];
            ++nChar;
        }
    }
    return strText;
}

void CMarkupSTL::x_DocChange( int nLeft, int nReplace, const string& strInsert )
{
    // Insert strInsert int m_strDoc at nLeft replacing nReplace chars
    // Do this with only one buffer reallocation if it grows
    //
    int nDocLength = (int)m_strDoc.size();
    int nInsLength = (int)strInsert.size();
    int nNewLength = nInsLength + nDocLength - nReplace;

    // When creating a document, reduce reallocs by reserving string space
    // Allow for 1.5 times the current allocation
    int nBufferLen = nNewLength;
    int nAllocLen = (int)m_strDoc.capacity();
    if ( nNewLength > nAllocLen )
    {
        nBufferLen += nBufferLen/2 + 128;
        if ( nBufferLen < nNewLength )
            nBufferLen = nNewLength;
        m_strDoc.reserve( nBufferLen );
    }

    m_strDoc.replace( nLeft, nReplace, strInsert );

}

void CMarkupSTL::x_Adjust( int iPos, int nShift, bool bAfterPos )
{
    // Loop through affected elements and adjust indexes
    // Algorithm:
    // 1. update children unless bAfterPos
    //    (if no children or bAfterPos is true, length of iPos not affected)
    // 2. update starts of next siblings and their children
    // 3. go up until there is a next sibling of a parent and update starts
    // 4. step 2
    int iPosTop = m_aPos[iPos].iElemParent;
    bool bPosFirst = bAfterPos; // mark as first to skip its children
    while ( iPos )
    {
        // Were we at containing parent of affected position?
        bool bPosTop = false;
        if ( iPos == iPosTop )
        {
            // Move iPosTop up one towards root
            iPosTop = m_aPos[iPos].iElemParent;
            bPosTop = true;
        }

        // Traverse to the next update position
        if ( ! bPosTop && ! bPosFirst && m_aPos[iPos].iElemChild )
        {
            // Depth first
            iPos = m_aPos[iPos].iElemChild;
        }
        else if ( m_aPos[iPos].iElemNext )
        {
            iPos = m_aPos[iPos].iElemNext;
        }
        else
        {
            // Look for next sibling of a parent of iPos
            // When going back up, parents have already been done except iPosTop
            while ( (iPos=m_aPos[iPos].iElemParent) != 0 && iPos != iPosTop )
                if ( m_aPos[iPos].iElemNext )
                {
                    iPos = m_aPos[iPos].iElemNext;
                    break;
                }
        }
        bPosFirst = false;

        // Shift indexes at iPos
        if ( iPos != iPosTop )
            m_aPos[iPos].nStart += nShift;
        else
            m_aPos[iPos].nLength += nShift;
    }
}

void CMarkupSTL::x_LocateNew( int iPosParent, int& iPosRel, int& nOffset, int nLength, int nFlags )
{
    // Determine where to insert new element or node
    //
    bool bInsert = (nFlags&1)?true:false;
    bool bHonorWhitespace = (nFlags&2)?true:false;

    int nStartAt;
    if ( nLength )
    {
        // Located at a non-element node
        if ( bInsert )
            nStartAt = nOffset;
        else
            nStartAt = nOffset + nLength;
    }
    else if ( iPosRel )
    {
        // Located at an element
        nStartAt = m_aPos[iPosRel].nStart;
        if ( ! bInsert ) // follow iPosRel
            nStartAt += m_aPos[iPosRel].nLength;
    }
    else if ( ! iPosParent )
    {
        // Outside of all elements
        if ( bInsert )
            nStartAt = 0;
        else
            nStartAt = (int)m_strDoc.size();
    }
    else if ( m_aPos[iPosParent].nFlags & MNF_EMPTY )
    {
        // Parent has no separate end tag, so split empty element
        nStartAt = m_aPos[iPosParent].StartContent() - 1;
    }
    else
    {
        if ( bInsert ) // after start tag
            nStartAt = m_aPos[iPosParent].StartContent();
        else // before end tag
            nStartAt = m_aPos[iPosParent].EndL();
    }

    // Go up to start of next node, unless its splitting an empty element
    if ( ! bHonorWhitespace && ! (m_aPos[iPosParent].nFlags & MNF_EMPTY) )
    {
        const char* szDoc = m_strDoc.c_str();
        int nChar = nStartAt;
        if ( ! x_FindAny(szDoc,nChar) || szDoc[nChar] == '<' )
            nStartAt = nChar;
    }

    // Determine iPosBefore
    int iPosBefore = 0;
    if ( iPosRel )
    {
        if ( bInsert )
        {
            if ( ! (m_aPos[iPosRel].nFlags & MNF_FIRST) )
                iPosBefore = m_aPos[iPosRel].iElemPrev;
        }
        else
            iPosBefore = iPosRel;
    }
    else if ( m_aPos[iPosParent].iElemChild )
    {
        if ( bInsert )
            iPosBefore = iPosRel;
        else
            iPosBefore = m_aPos[m_aPos[iPosParent].iElemChild].iElemPrev;
    }

    nOffset = nStartAt;
    iPosRel = iPosBefore;
}

bool CMarkupSTL::x_AddElem( const char* szName, const char* szValue, bool bInsert, bool bAddChild )
{
    if ( bAddChild )
    {
        // Adding a child element under main position
        if ( ! m_iPos )
            return false;
    }
    else if ( m_iPosParent == 0 )
    {
        // Adding root element
        if ( IsWellFormed() )
            return false;


        // Locate after any version and DTD
        m_aPos[0].nLength = (int)m_strDoc.size();
    }

    // Locate where to add element relative to current node
    int iPosParent, iPosBefore, nOffset = 0, nLength = 0;
    if ( bAddChild )
    {
        iPosParent = m_iPos;
        iPosBefore = m_iPosChild;
    }
    else
    {
        iPosParent = m_iPosParent;
        iPosBefore = m_iPos;
        nOffset = m_nNodeOffset;
        nLength = m_nNodeLength;
    }
    int nFlags = bInsert?1:0;
    x_LocateNew( iPosParent, iPosBefore, nOffset, nLength, nFlags );
    bool bEmptyParent = (m_aPos[iPosParent].nFlags & MNF_EMPTY)?true:false;
    bool bNoContentParent = (iPosParent && m_aPos[iPosParent].ContentLen() == 0)?true:false;
    if ( bEmptyParent || bNoContentParent )
        nOffset += x_EOLLEN;

    // Create element and modify positions of affected elements
    // If no szValue is specified, an empty element is created
    // i.e. either <NAME>value</NAME> or <NAME/>
    //
    int iPos = x_GetFreePos();
    ElemPos* pElem = &m_aPos[iPos];
    pElem->nStart = nOffset;
    pElem->iElemChild = 0;
    x_LinkElem( iPosParent, iPosBefore, iPos );

    // Create string for insert
    string strInsert;
    int nLenName = (int)strlen(szName);
    int nLenValue = szValue? (int)strlen(szValue) : 0;
    if ( ! nLenValue )
    {
        // <NAME/> empty element
        strInsert = "<";
        strInsert += szName;
        strInsert += "/>" x_EOL;
        pElem->SetStartTagLen( nLenName + 3 );
        pElem->SetEndTagLen( 2 );
        pElem->nLength = nLenName + 3;
        pElem->nFlags |= MNF_EMPTY;
    }
    else
    {
        // <NAME>value</NAME>
        string strValue = x_TextToDoc( szValue );
        nLenValue = (int)strValue.size();
        strInsert = "<";
        strInsert += szName;
        strInsert += ">";
        strInsert += strValue;
        strInsert += "</";
        strInsert += szName;
        strInsert += ">" x_EOL;
        pElem->SetStartTagLen( nLenName + 2 );
        pElem->SetEndTagLen( nLenName + 3 );
        pElem->nLength = nLenName * 2 + nLenValue + 5;
    }

    // Insert
    int nReplace = 0, nInsertAt = pElem->nStart;
    if ( bEmptyParent )
    {
        string strParentTagName = x_GetTagName(iPosParent);
        string strFormat;
        strFormat = ">" x_EOL;
        strFormat += strInsert;
        strFormat += "</";
        strFormat += strParentTagName;
        strInsert = strFormat;
        nInsertAt = m_aPos[iPosParent].StartContent() - 2;
        nReplace = 1;
        m_aPos[iPosParent].AdjustStartTagLen( -1 );
        m_aPos[iPosParent].AdjustEndTagLen( 1 + (int)strParentTagName.size() );
        m_aPos[iPosParent].nFlags ^= MNF_EMPTY;
    }
    else if ( bNoContentParent )
    {
        strInsert = x_EOL + strInsert;
        nInsertAt = m_aPos[iPosParent].StartContent();
    }
    x_DocChange( nInsertAt, nReplace, strInsert );
    x_Adjust( iPos, (int)strInsert.size() - nReplace );

    if ( bAddChild )
        x_SetPos( m_iPosParent, iPosParent, iPos );
    else
        x_SetPos( iPosParent, iPos, 0 );
    return true;
}

string CMarkupSTL::x_GetSubDoc( int iPos ) const
{
    if ( iPos )
    {
        int nStart = m_aPos[iPos].nStart;
        int nNext = nStart + m_aPos[iPos].nLength;
        const char* szDoc = m_strDoc.c_str();
        int nChar = nNext;
        if ( ! x_FindAny(szDoc,nChar) || szDoc[nChar] == '<' )
            nNext = nChar;
        return m_strDoc.substr( nStart, nNext - nStart );
    }
    return "";
}

bool CMarkupSTL::x_AddSubDoc( const char* szSubDoc, bool bInsert, bool bAddChild )
{
    // Add subdocument, parse, and modify positions of affected elements
    //
    int nOffset = 0, iPosParent, iPosBefore;
    if ( bAddChild )
    {
        // Add a subdocument under main position, before or after child
        if ( ! m_iPos )
            return false;
        iPosParent = m_iPos;
        iPosBefore = m_iPosChild;
    }
    else
    {
        // Add a subdocument under parent position, before or after main
        if ( ! m_iPosParent )
            return false;
        iPosParent = m_iPosParent;
        iPosBefore = m_iPos;
    }

    // Determine insert location
    int nFlags = bInsert?1:0;
    x_LocateNew( iPosParent, iPosBefore, nOffset, 0, nFlags );
    bool bEmptyParent = (m_aPos[iPosParent].nFlags & MNF_EMPTY)?true:false;
    if ( bEmptyParent )
        nOffset += x_EOLLEN;

    // Parse subdocument
    TokenPos token( szSubDoc );
    int iPosTempParent = x_GetFreePos();
    int iPosFreeBeforeAdd = m_iPosFree;
    m_aPos[iPosTempParent].Clear();
    m_aPos[iPosTempParent].SetLevel( m_aPos[iPosParent].Level() + 1 );
    int iPos = x_ParseElem( iPosTempParent, token );
    m_aPos[iPosTempParent].nFlags = MNF_DELETED;
    if ( iPos <= 0 )
    {
        // Abort because not well-formed
        m_iPosFree = iPosFreeBeforeAdd;
        m_aPos[iPosTempParent].iElemNext = m_iPosDeleted;
        m_iPosDeleted = iPosTempParent;
        return false;
    }

    // Extract subdocument without leading/trailing nodes
    string strInsert;
    int nExtractStart = m_aPos[iPos].nStart;
    int nExtractLength = m_aPos[iPos].nLength;
    strInsert.assign( &szSubDoc[nExtractStart], nExtractLength );
    if ( x_EOLLEN )
        strInsert += x_EOL;

    // Do an adjust here, using iPosTempParent so that it does not affect rest of document
    int nAdjust = nOffset - nExtractStart;
    m_aPos[iPos].nStart += nAdjust;
    x_Adjust( iPos, nAdjust );

    // Link in parent and siblings
    x_LinkElem( iPosParent, iPosBefore, iPos );
    m_aPos[iPosTempParent].iElemNext = m_iPosDeleted;
    m_iPosDeleted = iPosTempParent;
    if ( ! m_aPos[iPos].iElemChild )
        m_aPos[iPos].nFlags |= MNF_EMPTY;

    // Insert subdocument
    int nReplace = 0, nInsertAt = nOffset;
    if ( bEmptyParent )
    {
        string strParentTagName = x_GetTagName(iPosParent);
        ElemPos* pParent = &m_aPos[iPosParent];
        string strFormat;
        strFormat = ">" x_EOL;
        strFormat += strInsert;
        strFormat += "</";
        strFormat += strParentTagName;
        strInsert = strFormat;
        nInsertAt = pParent->StartContent() - 2;
        nReplace = 1;
        pParent->AdjustStartTagLen( -1 );
        pParent->AdjustEndTagLen( 1 + (int)strParentTagName.size() );
        pParent->nFlags ^= MNF_EMPTY;
    }
    x_DocChange( nInsertAt, nReplace, strInsert );
    x_Adjust( iPos, (int)strInsert.size() - nReplace, true );

    // Set position to top element of subdocument
    if ( bAddChild )
        x_SetPos( m_iPosParent, iPosParent, iPos );
    else // Main
        x_SetPos( m_iPosParent, iPos, 0 );
    return true;
}

int CMarkupSTL::x_RemoveElem( int iPos )
{
    // Remove element and all contained elements
    // Return new position
    //
    if ( ! iPos )
        return 0;

    // Determine whether any whitespace up to next tag
    int nAfterEnd = m_aPos[iPos].StartAfter();
    const char* szDoc = m_strDoc.c_str();
    int nChar = nAfterEnd;
    if ( ! x_FindAny(szDoc,nChar) || szDoc[nChar] == '<' )
        nAfterEnd = nChar;

    // Remove from document, adjust affected indexes, and unlink
    int nLen = nAfterEnd - m_aPos[iPos].nStart;
    x_DocChange( m_aPos[iPos].nStart, nLen, string() );
    x_Adjust( iPos, - nLen, true );
    return x_UnlinkElem( iPos );
}

void CMarkupSTL::x_LinkElem( int iPosParent, int iPosBefore, int iPos )
{
    // Link in element, and initialize nFlags, and iElem indexes
    ElemPos* pElem = &m_aPos[iPos];
    pElem->iElemParent = iPosParent;
    if ( iPosBefore )
    {
        // Link in after iPosBefore
        pElem->nFlags = 0;
        pElem->iElemNext = m_aPos[iPosBefore].iElemNext;
        if ( ! pElem->iElemNext )
            m_aPos[m_aPos[iPosParent].iElemChild].iElemPrev = iPos;
        m_aPos[iPosBefore].iElemNext = iPos;
        pElem->iElemPrev = iPosBefore;
    }
    else
    {
        // Link in as first child
        pElem->nFlags = MNF_FIRST;
        if ( m_aPos[iPosParent].iElemChild )
        {
            pElem->iElemNext = m_aPos[iPosParent].iElemChild;
            pElem->iElemPrev = m_aPos[pElem->iElemNext].iElemPrev;
            m_aPos[pElem->iElemNext].iElemPrev = iPos;
            m_aPos[pElem->iElemNext].nFlags ^= MNF_FIRST;
        }
        else
        {
            pElem->iElemNext = 0;
            pElem->iElemPrev = iPos;
        }
        m_aPos[iPosParent].iElemChild = iPos;
    }
    if ( iPosParent )
        pElem->SetLevel( m_aPos[iPosParent].Level() + 1 );
}

int CMarkupSTL::x_UnlinkElem( int iPos )
{
    // Fix links to remove element and mark as deleted
    // return previous position or zero if none
    ElemPos* pElem = &m_aPos[iPos];

    // Find previous sibling and bypass removed element
    int iPosPrev = 0;
    if ( pElem->nFlags & MNF_FIRST )
    {
        if ( pElem->iElemNext ) // set next as first child
        {
            m_aPos[pElem->iElemParent].iElemChild = pElem->iElemNext;
            m_aPos[pElem->iElemNext].iElemPrev = pElem->iElemPrev;
            m_aPos[pElem->iElemNext].nFlags |= MNF_FIRST;
        }
        else // no children remaining
            m_aPos[pElem->iElemParent].iElemChild = 0;
    }
    else
    {
        iPosPrev = pElem->iElemPrev;
        m_aPos[iPosPrev].iElemNext = pElem->iElemNext;
        if ( pElem->iElemNext )
            m_aPos[pElem->iElemNext].iElemPrev = iPosPrev;
        else
            m_aPos[m_aPos[pElem->iElemParent].iElemChild].iElemPrev = iPosPrev;
    }
    pElem->nFlags = MNF_DELETED;
    pElem->iElemNext = m_iPosDeleted;
    m_iPosDeleted = iPos;
    return iPosPrev;
}


