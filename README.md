# SQLiteExplorer
一个查看SQLite文件结构的工具软件(SQLite3 File Format Tools)
目前效果图

## Database界面
该界面主要显示数据库属性
![image](https://github.com/hawkchch/SQLiteExplorer/blob/master/art/1.png)

## Data界面
该界面主要展示表数据。目前采用滚动加载方式，防止一次加载数据过多导致卡死。
![image](https://github.com/hawkchch/SQLiteExplorer/blob/master/art/2.png)

## SQL界面
sql语句查询界面，查询结果也是采用Data界面方式滚动加载。
![image](https://github.com/hawkchch/SQLiteExplorer/blob/master/art/3.png)

## Design界面
表设计界面，展示表含有的列编号，列名称，类型，非空，默认值以及主键情况。
![image](https://github.com/hawkchch/SQLiteExplorer/blob/master/art/4.png)

## HexWindow界面
HexWindow界面目前支持IndexInterior，IndexLeaf，TableInterior，TableLeaf，FreelistTrunkPage，FreelistLeafPage页面的解析：

### 索引内部页 IndexInterior
![image](https://github.com/hawkchch/SQLiteExplorer/blob/master/art/IndexInterior.jpg)

### 索引叶子页 IndexLeaf
![image](https://github.com/hawkchch/SQLiteExplorer/blob/master/art/IndexLeaf.jpg)

### 表内部页 TableInterior
![image](https://github.com/hawkchch/SQLiteExplorer/blob/master/art/TableInterior.jpg)

### 表叶子页 TableLeaf
![image](https://github.com/hawkchch/SQLiteExplorer/blob/master/art/TableLeaf.jpg)

### 自由页主干页 FreelistTrunkPage
![image](https://github.com/hawkchch/SQLiteExplorer/blob/master/art/FreeListTrunk.jpg)

### 自由页叶子页 FreelistLeafPage
![image](https://github.com/hawkchch/SQLiteExplorer/blob/master/art/FreelistLeaf.jpg)

## DDL界面
展示建表语句。
![image](https://github.com/hawkchch/SQLiteExplorer/blob/master/art/6.png)

## Graph界面
目前支持Index，Table，Freelist的页组织形式的显示。

### 索引图
![image](https://github.com/hawkchch/SQLiteExplorer/blob/master/art/IndexGraph.jpg)

### 表图
![image](https://github.com/hawkchch/SQLiteExplorer/blob/master/art/TableGraph.jpg)

### 自由页图
![image](https://github.com/hawkchch/SQLiteExplorer/blob/master/art/FreelistGraph.jpg)
