# SQLiteExplorer
该项目旨在创建一个查看SQLite文件结构的工具软件(SQLite3 File Format Tools)，用以方便对SQLite文件格式的学习。
这里有需要的链接：

SQLite3 File Format:
https://sqlite.org/fileformat.html

GraphViz:
http://www.graphviz.org/


目前实现效果图如下：

## 1. Database界面
该界面主要显示数据库属性
![image](https://gitee.com/chuck_wilson/SQLiteExplorer/raw/master/art/1.png)

## 2. Data界面
该界面主要展示表数据。目前采用滚动加载方式，防止一次加载数据过多导致卡死。
![image](https://gitee.com/chuck_wilson/SQLiteExplorer/raw/master/art/2.png)

## 3. SQL界面
sql语句查询界面，查询结果也是采用Data界面方式滚动加载。
![image](https://gitee.com/chuck_wilson/SQLiteExplorer/raw/master/art/3.png)

## 4. Design界面
表设计界面，展示表含有的列编号，列名称，类型，非空，默认值以及主键情况。
![image](https://gitee.com/chuck_wilson/SQLiteExplorer/raw/master/art/4.png)

## 5. HexWindow界面
HexWindow界面目前支持IndexInterior，IndexLeaf，TableInterior，TableLeaf，FreelistTrunkPage，FreelistLeafPage，Overflow页面的解析：

### 5.1 索引内部页 IndexInterior
![image](https://gitee.com/chuck_wilson/SQLiteExplorer/raw/master/art/IndexInterior.jpg)

### 5.2 索引叶子页 IndexLeaf
![image](https://gitee.com/chuck_wilson/SQLiteExplorer/raw/master/art/IndexLeaf.jpg)

### 5.3 表内部页 TableInterior
![image](https://gitee.com/chuck_wilson/SQLiteExplorer/raw/master/art/TableInterior.jpg)

### 5.4 表叶子页 TableLeaf
![image](https://gitee.com/chuck_wilson/SQLiteExplorer/raw/master/art/TableLeaf.jpg)

### 5.5 自由页主干页 FreelistTrunkPage
![image](https://gitee.com/chuck_wilson/SQLiteExplorer/raw/master/art/FreeListTrunk.jpg)

### 5.6 自由页叶子页 FreelistLeafPage
![image](https://gitee.com/chuck_wilson/SQLiteExplorer/raw/master/art/FreelistLeaf.jpg)

## 6. DDL界面
展示建表语句。
![image](https://gitee.com/chuck_wilson/SQLiteExplorer/raw/master/art/6.png)

## 7. Graph界面
目前支持Index，Table，Freelist的页组织形式的显示。

### 7.1 索引图
![image](https://gitee.com/chuck_wilson/SQLiteExplorer/raw/master/art/IndexGraph.jpg)

### 7.2 表图
![image](https://gitee.com/chuck_wilson/SQLiteExplorer/raw/master/art/TableGraph.jpg)

### 7.3 自由页图
![image](https://gitee.com/chuck_wilson/SQLiteExplorer/raw/master/art/FreelistGraph.jpg)
