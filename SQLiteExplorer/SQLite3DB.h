#pragma once
#include <string>
#include <vector>
#include <deque>
#include <map>
using namespace std;

#include "sqlite3.h"
#include "utils.h"
#include "CppSQLite3.h"

typedef deque<string> cell_content;
typedef deque<cell_content> table_content;

struct TableSchema
{
    string   type;
    string   name;
    string   tbl_name;
    uint64_t rootpage;
    string   sql;
};

enum SQLite3DataType
{
    SQLITE_TYPE_INTEGER = 1,
    SQLITE_TYPE_FLOAT = 2,
    SQLITE_TYPE_TEXT = 3,
    SQLITE_TYPE_BLOB = 4,
    SQLITE_TYPE_NULL = 5
};
struct SQLite3Variant 
{
    SQLite3DataType type;

    i64 iVal;
    double lfVal;
    string blob;
    string text;

    // typeAndLen字段
    int typeAndLenBlob;
    int typeAndLenVal;
    int typeAndLenStartAddrInCell;
    int typeAndLenLen;
    int valStartAddrInCell;
    int valLen;

private:
    string desc;
};

struct ContentArea
{
    int m_startAddr;    // 当前页的相对地址
    i64 m_len;          // 长度

public:
    void Clear()
    {
        m_startAddr = m_len = 0;
    }
};

enum PageType
{
    PAGE_TYPE_UNKNOWN        = 0x00,
    PAGE_TYPE_INDEX_INTERIOR = 0x02,    // B-Tree Interior
    PAGE_TYPE_TABLE_INTERIOR = 0x05,    // B+Tree Interior
    PAGE_TYPE_INDEX_LEAF     = 0x0A,    // B-Tree Leaf
    PAGE_TYPE_TABLE_LEAF     = 0x0D,    // B+Tree Leaf
    PAGE_TYPE_OVERFLOW,         // 溢出页
    PAGE_TYPE_FREELIST_TRUNK,   // 自由页Trunk
    PAGE_TYPE_FREELIST_LEAF,    // 自由页叶子页
    PAGE_TYPE_PTR_MAP,          // auto_vacuum=incremental, page_2...
};

struct PageUsageInfo
{
    int pgno;       // 当前页页号
    int parent;     // 父页页号
    PageType type;  // 当前页类型
    int ncell;      // 当前页cell数量

    int overflow_page_idx;
    int overflow_cell_idx;
    string desc;

    PageUsageInfo()
        : pgno(0), parent(0), type(PAGE_TYPE_UNKNOWN), ncell(0)
        , overflow_page_idx(0), overflow_cell_idx(0)
    {}

    bool isRootPage(){

    }
};


class CSQLite3Page;
class CSQLite3Payload;

int decode_number(unsigned char *aData,             /* Content being decoded */
                              int ofst, int nByte   /* Start and size of decode */);

/*
** Convert the var-int format into i64.  Return the number of bytes
** in the var-int.  Write the var-int value into *pVal.
*/
int decodeVarint(const unsigned char *z, int64_t *pVal);
/*
** Extract a big-endian 32-bit integer
*/
unsigned int decodeInt32(const unsigned char *z);

class CSQLite3DB : public CppSQLite3DB
{
    friend class CSQLite3Page;
    friend class CSQLite3Payload;
public:
    CSQLite3DB(const string& path);
    ~CSQLite3DB(void);

    // 获取所有表名称
    vector<string> GetAllTableNames();

    // 获取所有叶子页id
    vector<pair<int, PageType> > GetAllPageIdsAndType(const string &name);

    // 获取指定页原始内容
    string LoadPage(int pgno, bool decode = true);

    // 获取指定页的记录数量
    int GetCellCounts(int pgno);

    // 获取指定页，指定索引的cell原始数据
    string LoadCell(int pgno, int idx);

    // 解码指定页，指定索引的数据
    bool DecodeCell(int pgno, int idx, vector<SQLite3Variant>& var);

    // 获取页大小
    int GetPageSize();

    // 获取指定表的列名称
    bool GetColumnNames(const string& tableName, vector<string>& colNames);

    bool GetIndexNames(const string& name, const string& tableName, vector<string>& colNames);

    // 执行sql查询,返回错误信息
    string ExecuteCmd(const string& sql, table_content& table, cell_content& headers);

    // 获取指定表的主键相关信息
    bool GetTablePrimaryKey(const string& tableName, vector<string>& pkFieldName, vector<string>& pkType, vector<int>& pkIdx, bool& withoutRowid);

    // 获取表字段信息
    bool GetTableInfo(const string& tableName, table_content& tb);

    // 获取Page信息
    vector<PageUsageInfo> GetPageUsageInfos(bool freelist);

    // 获取数据库信息
    map<string, string> GetDatabaseInfo();

    // 设置数据库信息
    void SetDatabaseInfo(const string& key, const string& val);

    // 获取自由页信息
    vector<PageUsageInfo> GetFreeList(bool useCache = false);

    // 解析自由页
    void DecodeFreeListTrunkPage(int pgno,
                                 ContentArea& sNextTrunkPageNo, int& nNextTrunkPageNo,
                                 ContentArea& sLeafPageCounts, int& nLeafPageCounts,
                                 vector<ContentArea>& sLeafPageNos, vector<int>& nLeafPageNos,
                                 ContentArea& sUnused);

private:
    bool OpenDatabase();
    bool FileOpen();
    void FileClose();
    unsigned char* FileRead(int64_t ofst, int nByte);
    int64_t FileGetsize(void);

    void LoadSqliteMaster();
    /*
    ** Describe the usages of a b-tree page
    */
    void PageUsageBtree(
        int pgno,             /* Page to describe */
        int parent,           /* Parent of this page.  0 for root pages */
        int idx,              /* Which child of the parent */
        const char *zName     /* Name of the table */
        );

    /*
    ** Find overflow pages of a cell and describe their usage.
    */
    void PageUsageCell(
        unsigned char cType,    /* Page type */
        unsigned char *a,       /* Cell content */
        int pgno,               /* page containing the cell */
        int cellno              /* Index of the cell on the page */
        );

    /*
    ** Compute the local payload size given the total payload size and
    ** the page size.
    */
    i64 LocalPayload(i64 nPayload, char cType);

private:
    string Pragma(const string& key);

private:
    string m_path;
    int    m_pagesize;      /* Size of a database page */
    uint64_t m_mxPage;      /* Last page number */
    int           m_bRaw;   /* True to access db file via OS APIs */
    int           m_dbfd;   /* File descriptor for reading the DB */
    sqlite3_file* m_pFd;    /* File descriptor for non-raw mode */

    map<string, TableSchema> m_mapTableSchema;
    bool m_bTableInfoHasLoad;

    vector<PageUsageInfo> m_pageUsageInfo;
    map<string, string> m_pragmaInfos;

public:
    CSQLite3Page* m_pSqlite3Page;
    CSQLite3Payload* m_pSqlite3Payload;
};



class CSQLite3Page
{
    friend class CSQLite3Payload;
public:
    CSQLite3Page(CSQLite3DB* parent);
    ~CSQLite3Page();

    // 获取指定页的原始数据
    string LoadPage(int pgno, bool decode = true);

    // 获取指定页的类型
    int GetPageType(int pgno);

    // 获取指定页的记录数
    int GetCellCounts(int pgno);

    // 获取指定页的指定索引的原始cell
    string LoadCell(int pgno, int idx);

    // 解码指定页，指定索引的数据
    bool DecodeCell(int pgno, int idx, vector<SQLite3Variant>& var);

private:
    void DecodePage();

    /*
    ** Create a description for a single cell.
    **
    ** The return value is the local cell size.
    */
    i64 GetPayloadSize(unsigned char cType, unsigned char* a);

    /*
    ** Compute the local payload size given the total payload size and
    ** the page size.
    */
    i64 LocalPayload(i64 nPayload, char cType);

    void Clear();

public:
    CSQLite3DB* m_pParent;

    string      m_pageRawContent;
    int         m_pgno;

    uint8_t  m_cType;
    uint16_t m_firstFreeBlockAddr;
    uint16_t m_cellCounts;
    uint16_t m_startOfCellContentAddr;
    uint8_t  m_fragmentBytes;
    int m_rightChildPageNumber;


    ContentArea m_pageHeaderArea;       // 页头区域
    ContentArea m_cellIndexArea;        // cellIndex区域
    vector<ContentArea> m_payloadArea;  // payload区域
    ContentArea m_unusedArea;           // 未使用区域
    vector<ContentArea> m_freeSpaceArea;// 空闲链表区域
};


class CSQLite3Payload
{
    friend class CSQLite3DB;
    friend class CSQLite3Page;
public:
    CSQLite3Payload(CSQLite3Page* parent);
    ~CSQLite3Payload();


    /*
    ** Create a description for a single cell.
    **
    ** The return value is the local cell size.
    */
    void DescribeCell(
        unsigned char cType,    /* Page type */
        unsigned char *a        /* Cell content */
        );

    /*
    ** Describe cell content.
    */
    bool DescribeContent();

    int GetLeftChild(){return m_leftChild;}
    i64 GetRowid(){return m_rowid;}

private:
    CSQLite3Page* m_pParent;
    string m_rawContent;

    i64 m_nPayload; // 整个cell大小 Not Contain itself
    i64 m_nLocal;   // cell在当前page大小

    unsigned char m_cType;

    // 
    int m_leftChild;
    i64 m_rowid;
    string m_pk;
    int m_cellHeaderSize; // Contain itself
    
    vector<int> m_typeAndLen;
    vector<SQLite3Variant> m_datas;
};
