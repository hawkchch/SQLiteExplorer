/*
** A utility for printing all or part of an SQLite database file.
*/
#include <stdio.h>
#include <ctype.h>
#define ISDIGIT(X) isdigit((unsigned char)(X))
#define ISPRINT(X) isprint((unsigned char)(X))
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#if !defined(_MSC_VER)
#include <unistd.h>
#else
#include <io.h>
#endif

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "sqlite3.h"

#include "SQLite3DB.h"
#include <algorithm>
#include <QDebug>
#include "utils.h"

/* Print a line of decode output showing a 4-byte integer.
*/
int decode_number(unsigned char *aData,      /* Content being decoded */
                              int ofst, int nByte        /* Start and size of decode */)
{
    int i;
    int val = aData[ofst];
    for(i=1; i<nByte; i++){
        val = val*256 + aData[ofst+i];
    }
    return val;
}

/*
** Convert the var-int format into i64.  Return the number of bytes
** in the var-int.  Write the var-int value into *pVal.
*/
int decodeVarint(const unsigned char *z, int64_t *pVal){
    int64_t v = 0;
    int i;
    for(i=0; i<8; i++){
        v = (v<<7) + (z[i]&0x7f);
        if( (z[i]&0x80)==0 ){ *pVal = v; return i+1; }
    }
    v = (v<<8) + (z[i]&0xff);
    *pVal = v;
    return 9;
}

/*
** Extract a big-endian 32-bit integer
*/
unsigned int decodeInt32(const unsigned char *z){
    return (z[0]<<24) + (z[1]<<16) + (z[2]<<8) + z[3];
}

CSQLite3DB::CSQLite3DB(const string& path)
: m_pagesize(4096)
, m_dbfd(-1)
, m_mxPage(0)
, m_bRaw(0)
, m_pFd(0)
, m_path(path)
, m_bTableInfoHasLoad(false)
, m_pSqlite3Page(NULL)
, m_pSqlite3Payload(NULL)
{
    FileOpen();

    int64_t szFile;
    unsigned char *zPgSz;
    szFile = FileGetsize();

    zPgSz = FileRead(16, 2);
    m_pagesize = decode_number(zPgSz, 0, 2);
    if( m_pagesize==0 ) m_pagesize = 1024;
    sqlite3_free(zPgSz);

    m_mxPage = (int)((szFile+m_pagesize-1)/m_pagesize);

    m_pSqlite3Page = new CSQLite3Page(this);
    m_pSqlite3Payload = new CSQLite3Payload(m_pSqlite3Page);
}

CSQLite3DB::~CSQLite3DB(void)
{

}

vector<string> CSQLite3DB::GetAllTableNames()
{
    vector<string> names;
    LoadSqliteMaster();

    for (map<string, TableSchema>::iterator it=m_mapTableSchema.begin(); it!=m_mapTableSchema.end(); ++it)
    {
        const TableSchema& tableInfo = it->second;
        if (tableInfo.type == "table")
        {
            names.push_back(tableInfo.name);
        }
        
    }

    return names;
}

vector<pair<int, PageType> > CSQLite3DB::GetAllPageIdsAndType(const string &cname)
{
    LoadSqliteMaster();
    m_pageUsageInfo.clear();

    vector<pair<int, PageType>> ids;
    string name = StrLower(cname);
    map<string, TableSchema>::iterator it = m_mapTableSchema.find(name);
    if (it != m_mapTableSchema.end())
    {
        PageUsageBtree(it->second.rootpage, 0, 0, name.c_str());
    }
    else if (name == "sqlite_master")
    {
        PageUsageBtree(1, 0, 0, name.c_str());
    }

    for (vector<PageUsageInfo>::iterator it=m_pageUsageInfo.begin();
        it != m_pageUsageInfo.end();
        ++ it)
    {
        ids.push_back(make_pair(it->pgno, it->type));
    }

    std::sort(ids.begin(), ids.end());
    return ids;
}

std::string CSQLite3DB::LoadPage( int pgno, bool decode )
{
    return m_pSqlite3Page->LoadPage(pgno, decode);
}

int CSQLite3DB::GetPageSize()
{
    return m_pagesize;
}

string CSQLite3DB::ExecuteCmd(const string& sql, table_content& table , cell_content &headers)
{
    string errmsg;
    sqlite3_stmt* stmt = NULL;
    do{
        if(SQLITE_OK != sqlite3_prepare(mpDB, sql.c_str(), -1, &stmt, NULL)){
            errmsg = sqlite3_errmsg(mpDB);
            break;
        }
        while(SQLITE_ROW == sqlite3_step(stmt)){
            cell_content content;
            for(int i = 0, iend = sqlite3_column_count(stmt); i < iend; ++i){
                string item;
                switch(sqlite3_column_type(stmt, i))
                {
                case SQLITE_INTEGER:
                case SQLITE_FLOAT:
                case SQLITE_TEXT:
                    item = (char*)sqlite3_column_text(stmt, i);
                    break;
                case SQLITE_BLOB:
                    item.assign((const char*)sqlite3_column_blob(stmt, i), sqlite3_column_bytes(stmt, i));
                    break;
                case SQLITE_NULL:
                    break;
                default:
                    break;
                }
                content.push_back(item);
            }
            table.push_back(content);

            if(headers.empty())
            {
                for(int i = 0, iend = sqlite3_column_count(stmt); i < iend; ++i)
                {
                    headers.push_back(sqlite3_column_name(stmt, i));
                }
            }
        }
    }while(SQLITE_SCHEMA == sqlite3_finalize(stmt));

    return errmsg;
}

bool CSQLite3DB::GetTablePrimaryKey(const string& tableName, vector<string> &pkFieldName, vector<string> &pkType, vector<int> &pkIdx, bool& withoutRowid)
{
    bool bGetPkIsOk = false;
    if(tableName.empty())
    {
        return bGetPkIsOk;
    }

    bGetPkIsOk = true;
    table_content tb;
    if(GetTableInfo(tableName, tb))
    {
        while(!tb.empty())
        {
            cell_content cc = tb.front();
            tb.pop_front();
            if(cc[5] == "1") // pk
            {
                pkIdx.push_back(StrToInt(cc[0].c_str()));
                pkFieldName.push_back(cc[1]);
                pkType.push_back(cc[2]);
            }
        }
    }

    withoutRowid = false;
    string sql = "SELECT sql FROM sqlite_master WHERE type='table' AND tbl_name='" + tableName + "'";
    CppSQLite3Query q = execQuery(sql.c_str());
    if(!q.eof())
    {
        string txt = q.getStringField(0);
        txt = txt.substr(txt.find(')'));
        if(txt.size() > 0)
        {
            txt = StrUpper(txt);
            if(txt.find("WITHOUT") != string::npos && txt.find("ROWID") != string::npos)
            {
                withoutRowid = true;
            }
        }
    }

    return bGetPkIsOk;
}

bool CSQLite3DB::GetTableInfo(const string &tableName, table_content &tb)
{
    string sql = "PRAGMA table_info(" + tableName + ")";
    cell_content cc;
    ExecuteCmd(sql, tb, cc);
    return tb.size() > 0;
}

vector<PageUsageInfo> CSQLite3DB::GetPageUsageInfos(bool freelist)
{
    if(freelist) GetFreeList();
    return m_pageUsageInfo;
}

map<string, string> CSQLite3DB::GetDatabaseInfo()
{
    if(m_pragmaInfos.size())
        return m_pragmaInfos;

    vector<string> keys;
    keys.push_back("application_id");
    keys.push_back("auto_vacuum");
    keys.push_back("automatic_index");
    keys.push_back("busy_timeout");
    keys.push_back("cache_size");
    keys.push_back("case_sensitive_like");
    keys.push_back("checkpoint_fullfsync");
    keys.push_back("collation_list");
    keys.push_back("compile_options");
    keys.push_back("encoding");
    keys.push_back("foreign_keys");
    keys.push_back("freelist_count");
    keys.push_back("fullfsync");
    keys.push_back("ignore_check_constraints");
    keys.push_back("journal_mode");
    keys.push_back("journal_size_limit");
    keys.push_back("legacy_file_format");
    keys.push_back("locking_mode");
    keys.push_back("max_page_count");
    keys.push_back("mmap_size");
    keys.push_back("page_count");
    keys.push_back("page_size");
    keys.push_back("parser_trace");
    keys.push_back("read_uncommitted");
    keys.push_back("recursive_triggers");
    keys.push_back("reverse_unordered_selects");
    keys.push_back("schema_version");
    keys.push_back("secure_delete");
    keys.push_back("synchronous");
    keys.push_back("temp_store");
    keys.push_back("user_version");
    keys.push_back("vdbe_addoptrace");
    keys.push_back("vdbe_listing");
    keys.push_back("vdbe_trace");
    keys.push_back("wal_autocheckpoint");
    keys.push_back("writable_schema");


    for(auto it=keys.begin(); it!=keys.end(); ++it)
    {
        m_pragmaInfos[*it] = Pragma(*it);
    }

    return m_pragmaInfos;
}

void CSQLite3DB::SetDatabaseInfo(const string &key, const string &val)
{

}

vector<PageUsageInfo> CSQLite3DB::GetFreeList(bool useCache)
{
    if(!useCache)
    {
        m_pageUsageInfo.clear();

        unsigned char* a = FileRead(0, 100);
        int pgno = decodeInt32(a+32);
        sqlite3_free(a);

        int cnt = 0;
        int i;
        int n;
        int iNext;
        int parent = 1;
        PageUsageInfo info;

        while( pgno>0 && pgno<=m_mxPage && (cnt++)<m_mxPage )
        {
            //page_usage_msg(pgno, "freelist trunk #%d child of %d", cnt, parent);
            a = FileRead((pgno-1)*m_pagesize, m_pagesize);
            iNext = decodeInt32(a);
            n = decodeInt32(a+4);

            info.pgno = pgno;
            info.parent = parent;
            info.type = PAGE_TYPE_FREELIST_TRUNK;
            info.ncell = n;
            m_pageUsageInfo.push_back(info);
            for(i=0; i<n; i++){
                int child = decodeInt32(a + (i*4+8));
                //page_usage_msg(child, "freelist leaf, child %d of trunk page %d", i, pgno);
                info.pgno = child;
                info.parent = pgno;
                info.type = PAGE_TYPE_FREELIST_LEAF;
                info.ncell = 1;
                m_pageUsageInfo.push_back(info);
            }
            sqlite3_free(a);
            parent = pgno;
            pgno = iNext;
        }
    }

    return m_pageUsageInfo;
}

void CSQLite3DB::DecodeFreeListTrunkPage(int pgno,
                                         ContentArea &sNextTrunkPageNo, int &nNextTrunkPageNo,
                                         ContentArea &sLeafPageCounts, int &nLeafPageCounts,
                                         vector<ContentArea> &sLeafPageNos, vector<int> &nLeafPageNos,
                                         ContentArea &sUnused)
{
    string pg = LoadPage(pgno, false);
    const unsigned char* a = (const unsigned char*)pg.c_str();
    int i;
    int n;
    int iNext;

    if( pgno>0 && pgno<=m_mxPage )
    {
        //page_usage_msg(pgno, "freelist trunk #%d child of %d", cnt, parent);
        iNext = decodeInt32(a);
        n = decodeInt32(a+4);
        sNextTrunkPageNo.m_startAddr = 0;
        sNextTrunkPageNo.m_len = 4;
        nNextTrunkPageNo = iNext;

        sLeafPageCounts.m_startAddr = 4;
        sLeafPageCounts.m_len = 4;
        nLeafPageCounts = n;

        for(i=0; i<n; i++){
            int child = decodeInt32(a + (i*4+8));
            //page_usage_msg(child, "freelist leaf, child %d of trunk page %d", i, pgno);

            ContentArea ca;
            ca.m_startAddr = i*4+8;
            ca.m_len = 4;
            sLeafPageNos.push_back(ca);
            nLeafPageNos.push_back(child);
        }

        sUnused.m_startAddr = n*4+8;
        sUnused.m_len = m_pagesize - sUnused.m_startAddr;
    }
}

void CSQLite3DB::LoadSqliteMaster()
{
    if (!m_bTableInfoHasLoad)
    {
        string sql = "SELECT * FROM SQLITE_MASTER";
        table_content tb;
        cell_content header;
        ExecuteCmd(sql, tb, header);

        while(!tb.empty())
        {
            cell_content cc = tb.front();
            tb.pop_front();

            string name = StrLower(cc[1]);
            TableSchema& tableInfo = m_mapTableSchema[name];
            tableInfo.type = cc[0];
            tableInfo.name = cc[1];
            tableInfo.tbl_name = cc[2];
            tableInfo.rootpage = StrToInt64(cc[3].c_str());
            tableInfo.sql = cc[4];
        }

        TableSchema& tableInfo = m_mapTableSchema["sqlite_master"];
        tableInfo.type = "table";
        tableInfo.name = "sqlite_master";
        tableInfo.tbl_name = "sqlite_master";
        tableInfo.rootpage = 1;
        tableInfo.sql = "CREATE TABLE IF NOT EXISTS sqlite_master (type TEXT, name TEXT, tbl_name TEXT, rootpage INTEGER, sql TEXT)";

        m_bTableInfoHasLoad = true;
    }
}

bool CSQLite3DB::OpenDatabase()
{
    try
    {
        open(m_path.c_str());
        return true;
    }
    catch(CppSQLite3Exception& e)
    {
        e.errorMessage();
        return false;
    }
//    int flags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_URI;
//    int rc = sqlite3_open_v2(m_path.c_str(), &mpDB, flags, 0);
//    if( rc!=SQLITE_OK ){
//        const char *zErr = sqlite3_errmsg(mpDB);
//        fprintf(stderr, "can't open %s (%s)\n", m_path.c_str(), zErr);
//        sqlite3_close(mpDB);
//        mpDB = NULL;
//    }
//    return (mpDB!=NULL);
}

bool CSQLite3DB::FileOpen()
{
    int rc;
    bool isOk = true;
    if (m_bRaw == 0)
    {
        void *pArg = (void *)(&m_pFd);
        if (!OpenDatabase())
        {
            return false;
        }
        rc = sqlite3_file_control(mpDB, "main", SQLITE_FCNTL_FILE_POINTER, pArg);
        if( rc!=SQLITE_OK ){
            fprintf(stderr, 
                "failed to obtain fd for %s (SQLite too old?)\n", m_path.c_str()
                );
            isOk = false;
        }
    }
    else
    {
        m_dbfd = ::open(m_path.c_str(), O_RDONLY);
        if( m_dbfd<0 ){
            fprintf(stderr,"can't open %s\n", m_path.c_str());
            isOk = false;
        }
    }
    return isOk;
}

void CSQLite3DB::FileClose()
{
    if( m_bRaw==0 ){
        sqlite3_close(mpDB);
        mpDB = 0;
        m_pFd = 0;
    }else{
        ::close(m_dbfd);
        m_dbfd = -1;
    }
}

unsigned char* CSQLite3DB::FileRead( int64_t ofst, int nByte )
{
    unsigned char *aData;
    int got;
    aData = (unsigned char *)sqlite3_malloc(nByte+32);
    if( aData==0 ) return aData; // out_of_memory();
    memset(aData, 0, nByte+32);

    if (m_bRaw == 0)
    {
        int rc = m_pFd->pMethods->xRead(m_pFd, (void*)aData, nByte, ofst);
        if( rc!=SQLITE_OK && rc!=SQLITE_IOERR_SHORT_READ ){
            fprintf(stderr, "error in xRead() - %d\n", rc);
            //exit(1);
        }
    }
    else
    {
        lseek(m_dbfd, (long)ofst, SEEK_SET);
        got = read(m_dbfd, aData, nByte);
        if( got>0 && got<nByte ) memset(aData+got, 0, nByte-got);
    }

    return aData;
}

int64_t CSQLite3DB::FileGetsize( void )
{
    int64_t res = 0;
    if( m_bRaw==0 ){
        int rc = m_pFd->pMethods->xFileSize(m_pFd, &res);
        if( rc!=SQLITE_OK ){
            fprintf(stderr, "error in xFileSize() - %d\n", rc);
            //exit(1);
        }
    }else{
        struct stat sbuf;
        fstat(m_dbfd, &sbuf);
        res = (int64_t)(sbuf.st_size);
    }
    return res;
}

void CSQLite3DB::PageUsageBtree( int pgno, /* Page to describe */ 
                                int parent, /* Parent of this page. 0 for root pages */ 
                                int idx, /* Which child of the parent */ 
                                const char *zName /* Name of the table */ )
{
    unsigned char *a;
    const char *zType = "corrupt node";
    int nCell;
    int i;
    int hdr = pgno==1 ? 100 : 0;
    PageUsageInfo info;
    static char zDesc[1000] = {0};

    if( pgno<=0 || pgno>m_mxPage ) return;
    a = FileRead((pgno-1)*m_pagesize, m_pagesize);
    switch( a[hdr] )
    {
    case 2:  zType = "interior node of index";
        info.type = PAGE_TYPE_INDEX_INTERIOR;
        break;
    case 5:  zType = "interior node of table";
        info.type = PAGE_TYPE_TABLE_INTERIOR;
        break;
    case 10: zType = "leaf of index";
        info.type = PAGE_TYPE_INDEX_LEAF;
        break;
    case 13: zType = "leaf of table";
        info.type = PAGE_TYPE_TABLE_LEAF;
        break;
    }

    info.parent = parent;
    info.pgno = pgno;

    if( parent ){
        sprintf(zDesc, "%d %s [%s], child %d of page %d",
            pgno, zType, zName, idx, parent);

    }else{
        sprintf(zDesc, "%d root %s [%s]", pgno, zType, zName);
    }
    info.desc = zDesc;
    nCell = a[hdr+3]*256 + a[hdr+4];
    info.ncell = nCell;
    m_pageUsageInfo.push_back(info);

    if( a[hdr]==2 || a[hdr]==5 ){
        int cellstart = hdr+12;
        unsigned int child;
        for(i=0; i<nCell; i++){
            int ofst;

            ofst = cellstart + i*2;
            ofst = a[ofst]*256 + a[ofst+1];
            child = decodeInt32(a+ofst);
            PageUsageBtree(child, pgno, i, zName);
        }
        child = decodeInt32(a+cellstart-4);
        PageUsageBtree(child, pgno, i, zName);
    }
    if( a[hdr]==2 || a[hdr]==10 || a[hdr]==13 ){
        int cellstart = hdr + 8 + 4*(a[hdr]<=5);
        for(i=0; i<nCell; i++){
            int ofst;
            ofst = cellstart + i*2;
            ofst = a[ofst]*256 + a[ofst+1];
            PageUsageCell(a[hdr], a+ofst, pgno, i);
        }
    }
    sqlite3_free(a);
}

void CSQLite3DB::PageUsageCell( unsigned char cType, /* Page type */ unsigned char *a, /* Cell content */ int pgno, /* page containing the cell */ int cellno /* Index of the cell on the page */ )
{
    int i;
    int n = 0;
    i64 nPayload;
    i64 rowid;
    i64 nLocal;
    static char zDesc[1000] = {0};

    i = 0;
    if( cType<=5 ){
        a += 4;
        n += 4;
    }
    if( cType!=5 ){
        i = decodeVarint(a, &nPayload);
        a += i;
        n += i;
        nLocal = LocalPayload(nPayload, cType);
    }else{
        nPayload = nLocal = 0;
    }
    if( cType==5 || cType==13 ){
        i = decodeVarint(a, &rowid);
        a += i;
        n += i;
    }
    if( nLocal<nPayload ){
        int ovfl = decodeInt32(a+nLocal);
        int cnt = 0;
        while( ovfl && (cnt++)<m_mxPage ){
            sprintf(zDesc, "%d overflow %d from cell %d of page %d",
                ovfl, cnt, cellno, pgno);

            PageUsageInfo info;
            info.type = PAGE_TYPE_OVERFLOW;
            info.pgno = ovfl;
            info.parent = pgno;
            info.overflow_page_idx = cnt;
            info.overflow_cell_idx = cellno;
            info.desc = zDesc;
            m_pageUsageInfo.push_back(info);

            a = FileRead((ovfl-1)*m_pagesize, 4);
            ovfl = decodeInt32(a);
            sqlite3_free(a);
        }
    }
}

i64 CSQLite3DB::LocalPayload( i64 nPayload, char cType )
{
    i64 maxLocal;
    i64 minLocal;
    i64 surplus;
    i64 nLocal;
    if( cType==13 ){
        /* Table leaf */
        maxLocal = m_pagesize-35;
        minLocal = (m_pagesize-12)*32/255-23;
    }else{
        maxLocal = (m_pagesize-12)*64/255-23;
        minLocal = (m_pagesize-12)*32/255-23;
    }
    if( nPayload>maxLocal ){
        surplus = minLocal + (nPayload-minLocal)%(m_pagesize-4);
        if( surplus<=maxLocal ){
            nLocal = surplus;
        }else{
            nLocal = minLocal;
        }
    }else{
        nLocal = nPayload;
    }
    return nLocal;
}

string CSQLite3DB::Pragma(const string &key)
{
    table_content tb;
    cell_content hdr;
    string sql = "PRAGMA " + key;
    string err = ExecuteCmd(sql, tb, hdr);
    if(err.empty() && tb.size())
    {
        cell_content cc = tb.front();
        if(cc.size())
        {
            return cc.front();
        }
    }

    return err;
}

int CSQLite3DB::GetCellCounts(int pgno)
{
    return m_pSqlite3Page->GetCellCounts(pgno);
}

string CSQLite3DB::LoadCell(int pgno, int idx)
{
    return m_pSqlite3Page->LoadCell(pgno, idx);
}

bool CSQLite3DB::DecodeCell(int pgno, int idx, vector<SQLite3Variant>& var)
{
    return m_pSqlite3Page->DecodeCell(pgno, idx, var);
}

bool CSQLite3DB::GetColumnNames(const string& tableName, vector<string>& colNames)
{
    bool bGetColNamesIsOk = false;
    if(tableName.empty())
    {
        return bGetColNamesIsOk;
    }

    table_content tb;
    if(GetTableInfo(tableName, tb))
    {
        while(!tb.empty())
        {
            cell_content cc = tb.front();
            tb.pop_front();

            if (cc.size() > 0)
            {
                colNames.push_back(cc[1]);
            }
        }

        bGetColNamesIsOk = true;
    }
    return bGetColNamesIsOk;
}

bool CSQLite3DB::GetIndexNames(const string &name, const string &tableName, vector<string> &colNames)
{
    bool bGetColNamesIsOk = false;
    if(tableName.empty())
    {
        return bGetColNamesIsOk;
    }

    string sql = "SELECT type,sql,rootpage FROM sqlite_master WHERE name='" + name + "' AND tbl_name='" + tableName + "'";
    try
    {
        CppSQLite3Query q = execQuery(sql.c_str());
        if(!q.eof())
        {
            string type = q.getStringField(0);
            string s = q.getStringField(1);
            int rootpage = q.getIntField(2);
            size_t start = s.find('(');
            size_t end = s.find(')');
            string sub = s.substr(start+1, end-start-1);
            string find = ",";
            vector<string> vs = StrSplit(sub, find);
            for(auto it=vs.begin(); it!=vs.end(); it++)
            {
                string col = *it;
                col = Trim(col, " ");
                colNames.push_back(col);
            }
            bGetColNamesIsOk = true;
        }
    }
    catch(CppSQLite3Exception& e)
    {

    }

    return bGetColNamesIsOk;
}




CSQLite3Page::CSQLite3Page(CSQLite3DB* parent)
: m_pParent(parent)
, m_pgno(0)
{

}

CSQLite3Page::~CSQLite3Page()
{

}

string CSQLite3Page::LoadPage(int pgno, bool decode)
{
    if (pgno == m_pgno)
    {
        return m_pageRawContent;
    }
    
    Clear();
    if(pgno > 0 && pgno <= m_pParent->m_mxPage)
    {
        int64_t ofst = (pgno-1)*m_pParent->m_pagesize;
        const char* data = (const char*)m_pParent->FileRead(ofst, m_pParent->m_pagesize);
        m_pageRawContent.assign(data, m_pParent->m_pagesize);
        sqlite3_free((void*)data);
        m_pgno = pgno;
        if(decode)
        {
            DecodePage();
        }
    }

    return m_pageRawContent;
}

void CSQLite3Page::DecodePage()
{
    unsigned char* a = (unsigned char*)m_pageRawContent.c_str();
    if (m_pageRawContent.size() != m_pParent->m_pagesize)
    {
        return;
    }

    if (m_pgno != 1) 
    {
        m_pageHeaderArea.m_startAddr = 0;
    }
    else 
    {
        m_pageHeaderArea.m_startAddr = 100;
        a += m_pageHeaderArea.m_startAddr;
    }

    // 读取page-header
    m_cType = a[0];
    m_firstFreeBlockAddr = decode_number(a, 1, 2);
    m_cellCounts = decode_number(a, 3, 2);
    m_startOfCellContentAddr = decode_number(a, 5, 2);
    m_fragmentBytes = decode_number(a, 7, 1);
    if (m_cType == 2 || m_cType == 5)
    {
        m_rightChildPageNumber = decode_number(a, 8, 4);
    }
    
    // 计算page-header区域
    int iCellPtr = (m_cType==2 || m_cType==5) ? 12 : 8; 
    m_pageHeaderArea.m_len = iCellPtr;

    // 计算cell-index区域
    m_cellIndexArea.m_startAddr = m_pageHeaderArea.m_startAddr + m_pageHeaderArea.m_len;
    m_cellIndexArea.m_len = 2*m_cellCounts;

    // 计算各个payload区域
    for(int i=0; i<m_cellCounts; i++){
        int cofst = iCellPtr + i*2;
        int64_t n;
        cofst = decode_number(a, cofst, 2);
        n = GetPayloadSize(m_cType, &a[cofst]);

        ContentArea area;
        area.m_startAddr = cofst;
        area.m_len = n;
        m_payloadArea.push_back(area);
    }

    // 计算unused区域
    m_unusedArea.m_startAddr = m_pageHeaderArea.m_startAddr + m_pageHeaderArea.m_len
        + m_cellIndexArea.m_len;
    int minStartAddr = m_pParent->m_pagesize;
    for ( vector<ContentArea>::iterator it = m_payloadArea.begin();
        it != m_payloadArea.end();
        ++it )
    {
        if (minStartAddr > it->m_startAddr)
        {
            minStartAddr = it->m_startAddr;
        }
    }
    m_unusedArea.m_len = minStartAddr - m_unusedArea.m_startAddr;

    // 计算空闲链表区域
    int next = m_firstFreeBlockAddr;
    while (next != 0)
    {
        ContentArea area;
        int ofst = next;
        area.m_startAddr = ofst;
        next = decode_number(a, ofst, 2);
        area.m_len = decode_number(a, ofst+2, 2);
        m_freeSpaceArea.push_back(area);
    }
}

i64 CSQLite3Page::GetPayloadSize(unsigned char cType, unsigned char* a)
{
    int i;
    int n = 0;
    int leftChild;
    i64 nPayload;
    i64 rowid;
    i64 nLocal;
    i = 0;
    if( cType<=5 ){
        leftChild = ((a[0]*256 + a[1])*256 + a[2])*256 + a[3];
        a += 4;
        n += 4;
    }
    if( cType!=5 ){
        i = decodeVarint(a, &nPayload);
        a += i;
        n += i;
        nLocal = LocalPayload(nPayload, cType);
    }else{
        nPayload = nLocal = 0;
    }
    if( cType==5 || cType==13 ){
        i = decodeVarint(a, &rowid);
        a += i;
        n += i;
    }
    if( nLocal<nPayload ){
        int ovfl;
        unsigned char *b = &a[nLocal];
        ovfl = ((b[0]*256 + b[1])*256 + b[2])*256 + b[3];
        //n += 4;
    }

    return nLocal+n;
}



void CSQLite3Page::Clear()
{
    m_pageRawContent.clear();
    m_pgno = 0;
    m_cType = m_firstFreeBlockAddr = m_cellCounts = m_startOfCellContentAddr = m_fragmentBytes = m_rightChildPageNumber = 0;
    m_pageHeaderArea.Clear();
    m_cellIndexArea.Clear();
    m_payloadArea.clear();
    m_unusedArea.Clear();
    m_freeSpaceArea.clear();
}

i64 CSQLite3Page::LocalPayload(i64 nPayload, char cType)
{
    return m_pParent->LocalPayload(nPayload, cType);
}

int CSQLite3Page::GetCellCounts(int pgno)
{
    LoadPage(pgno);
    return m_cellCounts;
}

string CSQLite3Page::LoadCell(int pgno, int idx)
{
    LoadPage(pgno);
    if (idx >= GetCellCounts(pgno) || idx >= m_payloadArea.size())
    {
        return "";
    }

    ContentArea& ca = m_payloadArea[idx];
    string cell = m_pageRawContent.substr(ca.m_startAddr);
    return cell;
}

int CSQLite3Page::GetPageType(int pgno)
{
    LoadPage(pgno);
    return m_cType;
}

bool CSQLite3Page::DecodeCell(int pgno, int idx, vector<SQLite3Variant>& vars)
{
    bool res = true;
    LoadPage(pgno);
   
    m_pParent->m_pSqlite3Payload->DescribeCell(m_cType, (unsigned char*)LoadCell(pgno, idx).c_str());
    vars = m_pParent->m_pSqlite3Payload->m_datas;
    return res;
}




CSQLite3Payload::CSQLite3Payload(CSQLite3Page* parent)
: m_pParent(parent)
{

}

CSQLite3Payload::~CSQLite3Payload()
{

}

void CSQLite3Payload::DescribeCell(unsigned char cType, /* Page type */ 
                                  unsigned char *a /* Cell content */ )
{
    int i;
    i64 nDesc = 0;
    int n = 0;

    //qDebug() << "DescribeCell cType =" << cType << " CellContent =" << a;
    m_leftChildLen = m_nPayloadLen = m_rowidLen = m_cellHeaderSizeLen = 0;
    m_leftChildStartAddr = m_nPayloadStartAddr = m_rowidStartAddr = m_cellHeaderSizeStartAddr = 0;

    m_datas.clear();
    i = 0;
    m_cType = cType;
    if( cType<=5 ){
        m_leftChild = ((a[0]*256 + a[1])*256 + a[2])*256 + a[3];
        m_leftChildStartAddr = n;
        m_leftChildLen = 4;
        a += 4;
        n += 4;
    }
    if( cType!=5 ){
        i = decodeVarint(a, &m_nPayload);
        m_nPayloadStartAddr = n;
        m_nPayloadLen = i;
        a += i;
        n += i;
        m_nLocal = m_pParent->LocalPayload(m_nPayload, cType);
    }else{
        m_nPayload = m_nLocal = 0;
    }
    if( cType==5 || cType==13 ){
        i = decodeVarint(a, &m_rowid);
        m_rowidStartAddr = n;
        m_rowidLen = i;
        a += i;
        n += i;
    }

    m_cellContent.assign((const char*)a-n, n);
    m_payloadContent.assign((const char*)a, m_nLocal);
    if( m_nLocal<m_nPayload ){
        int ovfl = decodeInt32(a + m_nLocal);
        int cnt = 0;
        uint64_t mxPage = m_pParent->m_pParent->m_mxPage;
        int pagesize = m_pParent->m_pParent->m_pagesize;
        string ovflContent;
        while (ovfl && (cnt++)<mxPage)
        {
            a = m_pParent->m_pParent->FileRead((ovfl-1)*pagesize, pagesize);
            ovfl = decodeInt32(a);
            ovflContent.assign((const char*)a+4, pagesize-4);
            m_payloadContent += ovflContent;
            sqlite3_free(a);
        }
        m_payloadContent = m_payloadContent.substr(0, m_nPayload);
        m_cellContent += m_payloadContent;

//         unsigned char *b = &a[m_nLocal];
//         ovfl = ((b[0]*256 + b[1])*256 + b[2])*256 + b[3];
//         //sprintf(&zDesc[nDesc], "ov: %d ", ovfl);
//         //nDesc += strlen(&zDesc[nDesc]);
//         n += 4;
    }
    if(cType!=5 ){
        nDesc += DescribeContent();
    }
    else
    {

    }
    //*pzDesc = zDesc;
    //return m_nLocal+n;
}

bool CSQLite3Payload::DescribeContent()
{
    //qDebug() << "m_ctype =" << m_cType << " DescribeContent =" << m_rawContent.c_str();
    i64 offset = m_cellContent.size() - m_payloadContent.size();
    int n;
    i64 i, x, v;
    const unsigned char *pData;
    const unsigned char *pLimit;
    unsigned char* a = (unsigned char*)m_payloadContent.c_str();
    const unsigned char* pStart = a;
    i64 nLocal = m_payloadContent.size();

    pLimit = &a[nLocal];
    n = decodeVarint(a, &x);
    m_cellHeaderSize = x;
    m_cellHeaderSizeStartAddr = offset + n;
    m_cellHeaderSizeLen = n;
    pData = &a[x];
    a += n;
    i = x - n;
    while( i>0 && pData<=pLimit )
    {
        SQLite3Variant var;
        n = decodeVarint(a, &x);
        var.tStartAddr = offset + a - pStart;
        var.tVal = x;
        var.tLen = n;

        a += n;
        i -= n;
        nLocal -= n;
        if (x == 0)
        {
            var.type = SQLITE_TYPE_NULL;
            var.blobStartAddr = 0;
        }
        else if( x>=1 && x<=6 )
        {
            v = (signed char)pData[0];
            pData++;
            switch( x )
            {
            case 6:  v = (v<<16) + (pData[0]<<8) + pData[1];  pData += 2;
            case 5:  v = (v<<16) + (pData[0]<<8) + pData[1];  pData += 2;
            case 4:  v = (v<<8) + pData[0];  pData++;
            case 3:  v = (v<<8) + pData[0];  pData++;
            case 2:  v = (v<<8) + pData[0];  pData++;
            }
            var.type = SQLITE_TYPE_INTEGER;
            var.iVal = v;
            
            //sprintf(zDesc, "%lld", v);
        }else if( x==7 )
        {
            //sprintf(zDesc, "real");
            var.type = SQLITE_TYPE_FLOAT;
            memcpy((void*)&var.lfVal, (void*)pData, 8); // FIX
            pData += 8;
        }else if( x==8 )
        {
            var.type = SQLITE_TYPE_INTEGER;
            var.iVal = 0;
            //sprintf(zDesc, "0");
        }else if( x==9 )
        {
            var.type = SQLITE_TYPE_INTEGER;
            var.iVal = 1;
            //sprintf(zDesc, "1");
        }else if( x>=12 )
        {
            i64 size = (x-12)/2;
            var.tVal = size;
            if( (x&1)==0 )
            {
                var.type = SQLITE_TYPE_BLOB;
                var.blob.assign((const char*)pData, size);
                //sprintf(zDesc, "blob(%lld)", size);
            }
            else
            {
                var.type = SQLITE_TYPE_TEXT;
                var.text.assign((const char*)pData, size);
                //sprintf(zDesc, "txt(%lld)", size);
            }
            pData += size;
        }
        //j = (int)strlen(zDesc);
        //zDesc += j;
        //nDesc += j;
        m_datas.push_back(var);
    }

    return true;
}
