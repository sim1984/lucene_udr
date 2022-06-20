﻿# UDR full-text search based on Lucene++

There is no built-in full-text search subsystem in Firebird. The Lucene UDR library implements 
full-text search procedures and functions using the freely distributed Lucene library. The original 
Lucene search engine is written in Java. Unfortunately the FB Java plugin for writing external
stored procedures and functions are still in Beta stage. Therefore, Lucene UDP uses the Lucene port to 
the C++ language [Lucene++](https://github.com/luceneplusplus/LucenePlusPlus). Lucene++ is slightly 
faster than the original Lucene engine, but has slightly less features.

## Installing Lucene UDR

To install Lucene UDR, you need:

1. Unpack the zip archive with dynamic libraries into the `plugins/udr` directory
2. Execute the script [fts$install.sql](https://github.com/sim1984/lucene_udr/blob/main/sql/fts%24install.sql)  
to register procedures and functions in an indexed database.
For databases of the 1st SQL dialect, use the script [fts$install_1.sql](https://github.com/sim1984/lucene_udr/blob/main/sql/fts%24install_1.sql)

You can download ready-made builds for Windows OS using the links:
* [LuceneUdr_Win_x64.zip](https://github.com/sim1984/lucene_udr/releases/download/1.0/LuceneUdr_Win_x64.zip)
* [LuceneUdr_Win_x86.zip](https://github.com/sim1984/lucene_udr/releases/download/1.0/LuceneUdr_Win_x86.zip)

Under Linux, you can compile the library yourself.

Download the demo database, for which the examples are prepared, using the following links:
* [fts_demo_3.0.zip](https://github.com/sim1984/lucene_udr/releases/download/1.0/fts_demo_3.0.zip) - database for Firebird 3.0;
* [fts_demo_4.0.zip](https://github.com/sim1984/lucene_udr/releases/download/1.0/fts_demo_4.0.zip) - database for Firebird 4.0.

## Building and installing the library under Linux

Lucene UDR is based on [Lucene++](https://github.com/luceneplusplus/LucenePlusPlus). 
In some Linux distributions, you can install `lucene++` and `lucene++-contrib` from their repositories. 
If there are no libraries in the repositories, then you will need to download and build them from the source.

```
$ git clone https://github.com/luceneplusplus/LucenePlusPlus.git
$ cd LucenePlusPlus
$ mkdir build; cd build
$ cmake ..
$ make
$ sudo make install
```

In order for the `lucene++` library to be installed in `/usr/lib` and not in `/usr/local/lib`, run `cmake -DCMAKE_INSTALL_PREFIX=/usr..` instead of `cmake ..`

The building of the lucene++ library is described in more detail in [BUILDING.md](https://github.com/luceneplusplus/LucenePlusPlus/blob/master/doc/BUILDING.md).

Now you can start building UDR Lucene.

```
$ git clone https://github.com/sim1984/lucene_udr.git
$ cd lucene_udr
$ mkdir build; cd build
$ cmake ..
$ make
$ sudo make install
```

In the process of executing `cmake ..` the following error may occur

```
CMake Error at /usr/lib64/cmake/liblucene++/liblucene++Config.cmake:41 (message):
  File or directory /usr/lib64/usr/include/lucene++/ referenced by variable
  liblucene++_INCLUDE_DIRS does not exist !
Call Stack (most recent call first):
  /usr/lib64/cmake/liblucene++/liblucene++Config.cmake:47 (set_and_check)
  CMakeLists.txt:78 (find_package)
```

To fix it, you need to fix the files `liblucene++Config.cmake` and `liblucene++-contrib Config.cmake`, where
to replace the line

```
get_filename_component(PACKAGE_PREFIX_DIR "${CMAKE_CURRENT_LIST_DIR}/../../usr" ABSOLUTE)
```

with 

```
get_filename_component(PACKAGE_PREFIX_DIR "${CMAKE_CURRENT_LIST_DIR}/../../.." ABSOLUTE)
```

## Configuring Lucene UDR

Before using full-text search in your database, you need to make a preliminary configuration.
The Lucene UDR settings are in the file `$(root)\ftp.ini`. If this file does not exist, then create it yourself.

This file specifies the path to the directory where full-text indexes for the specified database will be created.

The full path to the database or alias must be set as the section name of the ini file 
(depending on the value of the `DatabaseAccess` parameter in `firebird.conf`). 
The path to the full-text index directory is specified in the `ftsDirectory` key.

```ini
[fts_demo]
ftsDirectory=f:\fbdata\3.0\fts\fts_demo

[f:\fbdata\3.0\fts_demo.fdb]
ftsDirectory=f:\fbdata\3.0\fts\fts_demo
```

Important: The user or group under which the Firebird service is running must have read and write permissions for the directory with full-text indexes.

You can get the directory location for full-text indexes using a query:

```sql
SELECT FTS$MANAGEMENT.FTS$GET_DIRECTORY() AS DIR_NAME
FROM RDB$DATABASE
```

## Creating full-text indexes

To create a full-text index, you need to perform three steps sequentially:
1. Creating a full-text index for a table using the procedure `FTS$MANAGEMENT.FTS$CREATE_INDEX`;
2. Adding indexed fields using the procedure `FTS$MANAGEMENT.FTS$ADD_INDEX_FIELD`;
3. Building an index using the procedure `FTS$MANAGEMENT.FTS$REBUILD_INDEX`.

### Creating a full-text index for a table

To create a full-text index for a table, call the procedure `FTS$MANAGEMENT.FTS$CREATE_INDEX`.

The first parameter specifies the name of the full-text index, the second - the name of the indexed table. The remaining parameters are optional.

The third parameter specifies the name of the analyzer. The analyzer specifies for which language the indexed fields will be analyzed. 
If the parameter is omitted, the STANDARD analyzer (for English) will be used. The list of available analyzers can be found 
using the procedure `FTS$MANAGEMENT.FTS$ANALYZERS`.

List of available analyzers:

* STANDARD - StandardAnalyzer (English);
* ARABIC - Arabic Analyzer;
* BRAZILIAN - BrazilianAnalyzer;
* CHINESE - ChineseAnalyzer;
* CJK - CJKAnalyzer (Chinese Letter);
* CZECH - CzechAnalyzer;
* DUTCH - DutchAnalyzer;
* ENGLISH - StandardAnalyzer (English);
* FRENCH - FrenchAnalyzer;
* GERMAN - GermanAnalyzer;
* GREEK - GreekAnalyzer;
* PERSIAN - PersianAnalyzer;
* RUSSIAN - RussianAnalyzer.

The fourth parameter specifies the name of the table field that will be returned as a search result. 
This is usually a primary or unique key field. Setting a special pseudo field `RDB$DB_KEY` is also supported. 
The value of only one field of one of the types can be returned:

* `SMALLINT`, `INTEGER`, `BIGINT` - fields of these types are often used as artificial primary key based on generators (sequences);

* `CHAR(16) CHARACTER SET OCTETS` or `BINARY(16)` - fields of these types are used as an artificial primary key based on GUID, that is, generated using `GEN_UUID()`;

* the `RDB$DB_KEY` field of type `CHAR(8) CHARACTER SET OCTETS`.

If this parameter is not set (NULL value), then an attempt will be made to find the field in the primary key for permanent tables and GTT. 
This attempt will be successful if the key is not composite and the field on which it is built has one of the data types described above. 
If the primary key does not exist, the pseudo field `RDB$DB_KEY` will be used.

The fifth parameter can be set to describe the field.

For examples, a table with the following structure is used:

```sql
CREATE TABLE PRODUCTS (
    PRODUCT_ID             BIGINT GENERATED BY DEFAULT AS IDENTITY,
    PRODUCT_UUID           CHAR(16) CHARACTER SET OCTETS NOT NULL,
    PRODUCT_NAME           VARCHAR(200) NOT NULL,
    UPC_EAN_CODE           VARCHAR(150),
    SELLING_PRICE          VARCHAR(400),
    MODEL_NUMBER           VARCHAR(45),
    ABOUT_PRODUCT          BLOB SUB_TYPE TEXT,
    PRODUCT_SPECIFICATION  BLOB SUB_TYPE TEXT,
    TECHNICAL_DETAILS      BLOB SUB_TYPE TEXT,
    SHIPPING_WEIGHT        VARCHAR(15),
    PRODUCT_DIMENSIONS     VARCHAR(50),
    VARIANTS               BLOB SUB_TYPE TEXT,
    PRODUCT_URL            VARCHAR(255) NOT NULL,
    IS_AMAZON_SELLER       BOOLEAN,
    CONSTRAINT PK_PRODUCT PRIMARY KEY (PRODUCT_ID),
    CONSTRAINT UNQ_PRODUCT_UUID UNIQUE (PRODUCT_UUID)
);
```

The example below creates an index `IDX_PRODUCT_NAME` for the `PRODUCTS` table using the `STANDARD` analyzer.
The `PRODUCT_ID` field is returned. Its name was automatically extracted from the primary key of the `PRODUCTS` table.

```sql
EXECUTE PROCEDURE FTS$MANAGEMENT.FTS$CREATE_INDEX('IDX_PRODUCT_NAME', 'PRODUCTS');

COMMIT;
```

The following example will create an index `IDX_PRODUCT_NAME_EN` using the analyzer `ENGLISH`.

```sql
EXECUTE PROCEDURE FTS$MANAGEMENT.FTS$CREATE_INDEX('IDX_PRODUCT_NAME_EN', 'PRODUCTS', 'ENGLISH');

COMMIT;
```

You can specify the name of the field that will be returned as a search result.

```sql
EXECUTE PROCEDURE FTS$MANAGEMENT.FTS$CREATE_INDEX('IDX_PRODUCT_ID_2_EN', 'PRODUCTS', 'ENGLISH', 'PRODUCT_ID');

EXECUTE PROCEDURE FTS$MANAGEMENT.FTS$CREATE_INDEX('IDX_PRODUCT_UUID_EN', 'PRODUCTS', 'ENGLISH', 'PRODUCT_UUID');

EXECUTE PROCEDURE FTS$MANAGEMENT.FTS$CREATE_INDEX('IDX_PRODUCT_DBKEY_EN', 'PRODUCTS', 'ENGLISH', 'RDB$DB_KEY');

COMMIT;
```

### Adding fields for indexing

After creating the index, you need to add fields that will be searched using the procedure `FTS$MANAGEMENT.FTS$ADD_INDEX_FIELD`. 
The first parameter specifies the index name, the second the name of the field to be added.
The third optional parameter can specify the significance multiplier for the field. 
By default, the significance of all index fields is the same and equal to 1.

```sql
EXECUTE PROCEDURE FTS$MANAGEMENT.FTS$ADD_INDEX_FIELD('IDX_PRODUCT_NAME_EN', 'PRODUCT_NAME');

EXECUTE PROCEDURE FTS$MANAGEMENT.FTS$ADD_INDEX_FIELD('IDX_PRODUCT_DBKEY_EN', 'PRODUCT_NAME');

EXECUTE PROCEDURE FTS$MANAGEMENT.FTS$ADD_INDEX_FIELD('IDX_PRODUCT_UUID_EN', 'PRODUCT_NAME');


EXECUTE PROCEDURE FTS$MANAGEMENT.FTS$ADD_INDEX_FIELD('IDX_PRODUCT_ID_2_EN', 'PRODUCT_NAME');
EXECUTE PROCEDURE FTS$MANAGEMENT.FTS$ADD_INDEX_FIELD('IDX_PRODUCT_ID_2_EN', 'ABOUT_PRODUCT');

COMMIT;
```

In the indexes `IDX_PRODUCT_NAME_EN`, `IDX_PRODUCT_DBKEY_EN` and `IDX_PRODUCT_UUID_EN` one field `PRODUCT_NAME` is processed, 
and in the index `IDX_PRODUCT_ID_2_EN` two fields `PRODUCT_NAME` and `ABOUT_PRODUCT` are processed.

The following example shows the creation of an index with two fields `PRODUCT_NAME` and `ABOUT_PRODUCT`. 
The significance of the `PRODUCT_NAME` field is 4 times higher than the significance of the `ABOUT_PRODUCT` field.

```sql
EXECUTE PROCEDURE FTS$MANAGEMENT.FTS$CREATE_INDEX('IDX_PRODUCT_ID_2X_EN', 'PRODUCTS', 'ENGLISH', 'PRODUCT_ID');

EXECUTE PROCEDURE FTS$MANAGEMENT.FTS$ADD_INDEX_FIELD('IDX_PRODUCT_ID_2X_EN', 'PRODUCT_NAME', 4);
EXECUTE PROCEDURE FTS$MANAGEMENT.FTS$ADD_INDEX_FIELD('IDX_PRODUCT_ID_2X_EN', 'ABOUT_PRODUCT');

COMMIT;
```

### Building an index

To build the index, the procedure `FTS$MANAGEMENT.FTS$REBUILD_INDEX` is used. The name of the full-text index must be specified as an input parameter.

```sql
EXECUTE PROCEDURE FTS$MANAGEMENT.FTS$REBUILD_INDEX('IDX_PRODUCT_NAME_EN');

EXECUTE PROCEDURE FTS$MANAGEMENT.FTS$REBUILD_INDEX('IDX_PRODUCT_DBKEY_EN');

EXECUTE PROCEDURE FTS$MANAGEMENT.FTS$REBUILD_INDEX('IDX_PRODUCT_UUID_EN');

EXECUTE PROCEDURE FTS$MANAGEMENT.FTS$REBUILD_INDEX('IDX_PRODUCT_ID_2_EN');

EXECUTE PROCEDURE FTS$MANAGEMENT.FTS$REBUILD_INDEX('IDX_PRODUCT_ID_2X_EN');

COMMIT;
```

At the stage of building for the index, a corresponding folder of the same name is created in the directory for full-text indexes.
These folders contain the Lucene index files. This part of the process happens outside of transaction control, so ROLLBACK will not remove the index files.

In addition, in case of a successful build, the status of the index changes to 'C' (Complete). Status changes occur in the current transaction.

## Index statuses

The description of the indexes is stored in the service table `FTS$INDEXES`.

The `FTS$INDEX_STATUS` field stores the index status. The index can have 4 statuses:

* *N* - New index. It is set when creating an index in which there is not a single segment yet.
* *U* - Updated metadata. It is set every time the index metadata changes, for example, when
an index segment is added or deleted. If the index has such a status, then it needs to be rebuilt so that the search for it
works correctly.
* *I* - Inactive. Inactive index. Inactive indexes are not updated by the `FTS$UPDATE_INDEXES` procedure.
* *C* - Complete. Active index. Such indexes are updated by the procedure `FTS$UPDATE_INDEXES`.
The index enters this state only after a complete build or rebuild.

## Search using full-text indexes

The `FTS$SEARCH` procedure is used to search the full-text index.

The first parameter specifies the name of the index with which the search will be performed, and the second parameter specifies the search phrase.
The third optional parameter sets a limit on the number of records returned, by default 1000.
The fourth parameter allows you to enable the search results explanation mode, FALSE by default.

Search example:

```sql
SELECT
    FTS$RELATION_NAME
  , FTS$KEY_FIELD_NAME
  , FTS$DB_KEY
  , FTS$ID
  , FTS$UUID
  , FTS$SCORE
  , FTS$EXPLANATION
FROM FTS$SEARCH('IDX_PRODUCT_NAME_EN', 'Transformers Bumblebee')
```

Output parameters:

- FTS$RELATION_NAME - the name of the table in which the document was found;
- FTS$KEY_FIELD_NAME - the name of the key field in the table;
- FTS$DB_KEY - the value of the key field in the format `RDB$DB_KEY`;
- FTS$ID - value of a key field of type `BIGINT` or `INTEGER`;
- FTS$UUID - value of a key field of type `BINARY(16)`. This type is used to store the GUID;
- FTS$SCORE - the degree of compliance with the search query;
- FTS$EXPLANATION - explanation of search results.

The query result will be available in one of the fields `FTS$DB_KEY`, `FTS$ID`, `FTS$UUID`, depending on which resulting field was specified when creating the index.

To extract data from the target table, it is enough to simply make a join with it, the condition of which depends on how the index was created.

Here are examples of different join options:

```sql
SELECT
  FTS.FTS$SCORE,
  P.PRODUCT_ID,
  P.PRODUCT_NAME
FROM FTS$SEARCH('IDX_PRODUCT_NAME_EN', 'Transformers Bumblebee') FTS
JOIN PRODUCTS P ON P.PRODUCT_ID = FTS.FTS$ID;

SELECT
  FTS.FTS$SCORE,
  P.PRODUCT_UUID,
  P.PRODUCT_NAME
FROM FTS$SEARCH('IDX_PRODUCT_UUID_EN', 'Transformers Bumblebee') FTS
JOIN PRODUCTS P ON P.PRODUCT_UUID = FTS.FTS$UUID;

SELECT
  FTS.FTS$SCORE,
  P.RDB$DB_KEY,
  P.PRODUCT_ID,
  P.PRODUCT_NAME
FROM FTS$SEARCH('IDX_PRODUCT_DBKEY_EN', 'Transformers Bumblebee') FTS
JOIN PRODUCTS P ON P.RDB$DB_KEY = FTS.FTS$DB_KEY;
```

To search for two fields at once, we use the index `IDX_PRODUCT_ID_2_EN`, in which the fields `REMARK` and `RUNTOTAL` were specified during creation.

```sql
SELECT
  FTS.FTS$SCORE,
  P.PRODUCT_ID,
  P.PRODUCT_NAME,
  P.ABOUT_PRODUCT
FROM FTS$SEARCH('IDX_PRODUCT_ID_2_EN', 'Transformers Bumblebee') FTS
JOIN PRODUCTS P ON P.PRODUCT_ID = FTS.FTS$ID;
```

To explain the search results, set the last parameter to TRUE.

```sql
SELECT
  FTS.FTS$SCORE,
  P.PRODUCT_ID,
  P.PRODUCT_NAME,
  P.ABOUT_PRODUCT,
  FTS.FTS$EXPLANATION
FROM FTS$SEARCH('IDX_PRODUCT_ID_2_EN', 'Transformers Bumblebee', 5, TRUE) FTS
JOIN PRODUCTS P ON P.PRODUCT_ID = FTS.FTS$ID;
```

The `FTS$EXPLANATION` field will contain an explanation of the result.

```
4.12074 = (MATCH) sum of:
  1.7817 = (MATCH) sum of:
    1.16911 = (MATCH) weight(PRODUCT_NAME:transformers in 3329), product of:
      0.455576 = queryWeight(PRODUCT_NAME:transformers), product of:
        6.84324 = idf(docFreq=28, maxDocs=10002)
        0.0665732 = queryNorm
      2.56622 = (MATCH) fieldWeight(PRODUCT_NAME:transformers in 3329), product of:
        1 = tf(termFreq(PRODUCT_NAME:transformers)=1)
        6.84324 = idf(docFreq=28, maxDocs=10002)
        0.375 = fieldNorm(field=PRODUCT_NAME, doc=3329)
    0.612596 = (MATCH) weight(ABOUT_PRODUCT:transformers in 3329), product of:
      0.480313 = queryWeight(ABOUT_PRODUCT:transformers), product of:
        7.21481 = idf(docFreq=19, maxDocs=10002)
        0.0665732 = queryNorm
      1.27541 = (MATCH) fieldWeight(ABOUT_PRODUCT:transformers in 3329), product of:
        1.41421 = tf(termFreq(ABOUT_PRODUCT:transformers)=2)
        7.21481 = idf(docFreq=19, maxDocs=10002)
        0.125 = fieldNorm(field=ABOUT_PRODUCT, doc=3329)
  2.33904 = (MATCH) sum of:
    1.60308 = (MATCH) weight(PRODUCT_NAME:bumblebee in 3329), product of:
      0.533472 = queryWeight(PRODUCT_NAME:bumblebee), product of:
        8.01332 = idf(docFreq=8, maxDocs=10002)
        0.0665732 = queryNorm
      3.00499 = (MATCH) fieldWeight(PRODUCT_NAME:bumblebee in 3329), product of:
        1 = tf(termFreq(PRODUCT_NAME:bumblebee)=1)
        8.01332 = idf(docFreq=8, maxDocs=10002)
        0.375 = fieldNorm(field=PRODUCT_NAME, doc=3329)
    0.735957 = (MATCH) weight(ABOUT_PRODUCT:bumblebee in 3329), product of:
      0.526458 = queryWeight(ABOUT_PRODUCT:bumblebee), product of:
        7.90796 = idf(docFreq=9, maxDocs=10002)
        0.0665732 = queryNorm
      1.39794 = (MATCH) fieldWeight(ABOUT_PRODUCT:bumblebee in 3329), product of:
        1.41421 = tf(termFreq(ABOUT_PRODUCT:bumblebee)=2)
        7.90796 = idf(docFreq=9, maxDocs=10002)
        0.125 = fieldNorm(field=ABOUT_PRODUCT, doc=3329)
```

For comparison, an explanation of the index search results with fields that have a different significance coefficient is shown.

```sql
SELECT
  FTS.FTS$SCORE,
  P.PRODUCT_ID,
  P.PRODUCT_NAME,
  P.ABOUT_PRODUCT,
  FTS.FTS$EXPLANATION
FROM FTS$SEARCH('IDX_PRODUCT_ID_2X_EN', 'Transformers Bumblebee', 5, TRUE) FTS
JOIN PRODUCTS P ON P.PRODUCT_ID = FTS.FTS$ID;
```

```
13.7448 = (MATCH) sum of:
  4.67643 = (MATCH) sum of:
    4.67643 = (MATCH) weight(PRODUCT_NAME:transformers in 166), product of:
      0.455576 = queryWeight(PRODUCT_NAME:transformers), product of:
        6.84324 = idf(docFreq=28, maxDocs=10002)
        0.0665732 = queryNorm
      10.2649 = (MATCH) fieldWeight(PRODUCT_NAME:transformers in 166), product of:
        1 = tf(termFreq(PRODUCT_NAME:transformers)=1)
        6.84324 = idf(docFreq=28, maxDocs=10002)
        1.5 = fieldNorm(field=PRODUCT_NAME, doc=166)
  9.06839 = (MATCH) sum of:
    9.06839 = (MATCH) weight(PRODUCT_NAME:bumblebee in 166), product of:
      0.533472 = queryWeight(PRODUCT_NAME:bumblebee), product of:
        8.01332 = idf(docFreq=8, maxDocs=10002)
        0.0665732 = queryNorm
      16.9988 = (MATCH) fieldWeight(PRODUCT_NAME:bumblebee in 166), product of:
        1.41421 = tf(termFreq(PRODUCT_NAME:bumblebee)=2)
        8.01332 = idf(docFreq=8, maxDocs=10002)
        1.5 = fieldNorm(field=PRODUCT_NAME, doc=166)
```

## Syntax of search queries

### Terms

Search queries (search phrases) consist of terms and operators. Lucene supports simple and complex terms.
Simple terms consist of one word, complex terms consist of several. The first of them are ordinary words,
for example, "Hello", "world". The second type of terms is a group of words, for example, "Hello world".
Several terms can be linked together using logical operators.

### Fields

Lucene supports multi-field search. By default, the search is performed in all fields of the full-text index,
the expression for each field is repeated and connected by the `OR` operator. For example, if you have an index containing
the fields `PRODUCT_NAME` and `ABOUT_PRODUCT`, then the query

```
Transformers Bumblebee
```

will be equivalent to the query

```
(PRODUCT_NAME: Transformers Bumblebee) OR (ABOUT_PRODUCT: Transformers Bumblebee)
```

You can specify which field you want to search by, to do this, specify the field name, the colon symbol ":" in the request,
and then the search phrase for this field.

Example of searching for the word "Polyester" in the `ABOUT_PRODUCT` field and the words "Transformers Bumblebee" in the `PRODUCT_NAME` field:

```sql
SELECT
  FTS.FTS$SCORE,
  P.PRODUCT_ID,
  P.PRODUCT_NAME,
  P.ABOUT_PRODUCT,
  FTS.FTS$EXPLANATION
FROM FTS$SEARCH('IDX_PRODUCT_ID_2_EN', '(PRODUCT_NAME: Transformers Bumblebee) AND (ABOUT_PRODUCT: Polyester)', 5, TRUE) FTS
JOIN PRODUCTS P ON P.PRODUCT_ID = FTS.FTS$ID;
```

Note: Lucene, like Firebird, supports delimited fields. It is strongly discouraged to use spaces and other special characters in field names, 
as this will make it much more difficult to write search queries. If your field contains a space or other special character, 
it must be escaped using the "\\" character.

For example, if you have an index for two fields "Product Name" and "Product Specification" and you want to find the word "Weight" in the specification, 
then the query should look like this:

```
Product\ Specification: Weight
```

### Mask

Lucene allows you to search for documents by mask, using the symbols "?" and "\*" in terms. In this case, the character "?" 
replaces any one character, and "\*" replaces any number of characters, for example

```
"te?t" "test*" "tes*t"
```

You cannot start a search query with the characters "?" or "\*".

### Fuzzy search

To perform a fuzzy search, add the tilde "~" to the end of the term. In this case, all
similar words will be searched, for example, when searching for "roam\~", the words "foam" and "roams" will also be found.

### Amplification of terms

Lucene allows you to change the meaning of terms in a search phrase. For example, you are looking for the phrase "Hello world" and
want the word "world" to be more meaningful. The significance of the term in the search phrase can be increased by using the symbol "ˆ",
after which the gain is indicated. In the following example, the significance of the word "world" is four times greater
than the significance of the word "Hello", which is equal to one by default.

```
"Hello worldˆ4"
```

### Logical operators

Logical operators allow you to use logical constructions when setting
search conditions, and allow you to combine several terms.
Lucene supports the following logical operators: `AND`, `+`, `OR`, `NOT`, `-`.

Logical operators must be specified in capital letters.

#### OR operator

`OR` is the default logical operator, which means that if
no other logical operator is specified between the two terms of the search phrase, then the `OR` operator is substituted. In this case, the search system finds
the document if one of the terms specified in the search phrase is present in it.
An alternative notation for the `OR` operator is `||`.

```
"Hello world" "world"
```

Equivalent to:

```
"Hello world" OR "world"
```

#### AND operator

The `AND` operator indicates that all search terms combined by the operator must be present in the text.
An alternative notation of the operator is `&&`.

```
"Hello" AND "world"
```

#### Operator +

The `+` operator indicates that the word following it must necessarily be present in the text.
For example, to search for records that must contain the word "hello" and may
contain the word "world", the search phrase may look like:

```
+Hello world
```

#### NOT operator

The `NOT` operator allows you to exclude from the search results those in which the term following the operator occurs. Instead of the word `NOT`, 
the symbol "!" can be used. For example, to search for records that should contain the word "hello" and should not contain the word "world", 
the search phrase may look like:

```
"Hello" NOT "world"
```

Note: The `NOT` operator cannot be used with only one term. For example, a search with this condition will not return results:

```
NOT "world"
```

#### Operator –

This operator is analogous to the `NOT` operator. Usage example:

```
"Hello" -"world"
```

#### Grouping of logical operators

The Lucene query analyzer supports grouping of logical operators. Let's say you need to find either the word "word"
or the word "dolly" and necessarily the word "hello", this query is used for this:

```
"Hello" && ("world" || "dolly")
```

### Escaping special characters

To include special characters in the search phrase, they are escaped using the "\\" character.
Below is a list of special characters used in Lucene at the moment:

```
+ - && || ! ( ) { } [ ] ˆ " ˜ * ? : \
```

The search phrase for the expression "(1 + 1) : 2" will have the form:

```
\( 1 \+ 1 \) \: 2
```

To escape special characters, you can use the `FTS$ESCAPE_QUERY` function.

```sql
FTS$ESCAPE_QUERY('(1 + 1) : 2')
```

A more detailed English-language description of the syntax is available on the official website
Lucene: [https://lucene.apache.org](https://lucene.apache.org).

## Indexing views

You can index not only permanent tables, but also complex views.

In order to index a view, one requirement must be met:
there must be a field in the view by which you can uniquely identify the record.

Let's say you have a view `V_PRODUCT_CATEGORIES`, where `PRODUCT_UUID` is the unique identifier of the `PRODUCTS` table:

```sql
CREATE TABLE CATEGORIES (
    ID             BIGINT GENERATED BY DEFAULT AS IDENTITY,
    CATEGORY_NAME  VARCHAR(80) NOT NULL,
    CONSTRAINT PK_CATEGORY PRIMARY KEY (ID),
    CONSTRAINT UNQ_CATEGORY_NAME UNIQUE (CATEGORY_NAME)
);

CREATE TABLE PRODUCT_CATEGORIES (
    ID            BIGINT GENERATED BY DEFAULT AS IDENTITY,
    PRODUCT_UUID  CHAR(16) CHARACTER SET OCTETS NOT NULL,
    CATEGORY_ID   BIGINT NOT NULL,
    CONSTRAINT PK_PRODUCT_CATEGORIES PRIMARY KEY (ID),
    CONSTRAINT UNQ_PRODUCT_CATEGORIES UNIQUE (PRODUCT_UUID, CATEGORY_ID),
    CONSTRAINT FK_PRODUCT_CAT_REF_CATEGORY FOREIGN KEY (CATEGORY_ID) REFERENCES CATEGORIES (ID),
    CONSTRAINT FK_PRODUCT_CAT_REF_PRODUCT FOREIGN KEY (PRODUCT_UUID) REFERENCES PRODUCTS (PRODUCT_UUID)
);

CREATE OR ALTER VIEW V_PRODUCT_CATEGORIES(
    PRODUCT_UUID,
    CATEGORIES)
AS
SELECT
    PC.PRODUCT_UUID
  , LIST(C.CATEGORY_NAME, ' | ') AS CATEGORIES
FROM PRODUCT_CATEGORIES PC
JOIN CATEGORIES C
     ON C.ID = PC.CATEGORY_ID
GROUP BY 1
;
```

You want to search for products of a category, but the name of the category is in the reference table and one product can have several categories. 
In this case, you can create the following full-text index:

```sql
EXECUTE PROCEDURE FTS$MANAGEMENT.FTS$CREATE_INDEX('IDX_PRODUCT_CATEGORIES', 'V_PRODUCT_CATEGORIES', 'ENGLISH', 'PRODUCT_UUID');

EXECUTE PROCEDURE FTS$MANAGEMENT.FTS$ADD_INDEX_FIELD('IDX_PRODUCT_CATEGORIES', 'CATEGORIES');

COMMIT;

EXECUTE PROCEDURE FTS$MANAGEMENT.FTS$REBUILD_INDEX('IDX_PRODUCT_CATEGORIES');

COMMIT;
```

The search for a product by its category looks like this:

```sql
SELECT
  FTS.FTS$SCORE,
  P.PRODUCT_UUID,
  P.PRODUCT_NAME,
  PC.CATEGORIES,
  FTS.FTS$EXPLANATION
FROM FTS$SEARCH('IDX_PRODUCT_CATEGORIES', '"Toys & Games"') FTS
JOIN V_PRODUCT_CATEGORIES PC ON PC.PRODUCT_UUID = FTS.FTS$UUID
JOIN PRODUCTS P ON P.PRODUCT_UUID = PC.PRODUCT_UUID;
```

## Highlighting found terms in a text fragment

It is often necessary not only to find documents on request, but also to highlight what was found.

To highlight the found terms in a text fragment, the package `FTS$HIGHLIGHTER` is used. The package contains:

- function `FTS$HIGHLIGHTER.FTS$BEST_FRAGMENT` to highlight the found terms in a text fragment;
- procedure `FTS$HIGHLIGHTER.FTS$BEST_FRAGMENTS` returns several fragments of text with the highlight of terms in the fragment.

### Highlighting found terms using the FTS$HIGHLIGHTER.FTS$BEST_FRAGMENT function

The function `FTS$HIGHLIGHTER.FTS$BEST_FRAGMENT` returns the best text fragment in which the found terms are highlighted with tags.

The function is described as

```sql
  FUNCTION FTS$BEST_FRAGMENT (
      FTS$TEXT BLOB SUB_TYPE TEXT CHARACTER SET UTF8,
      FTS$QUERY VARCHAR(8191) CHARACTER SET UTF8,
      FTS$ANALYZER VARCHAR(63) CHARACTER SET UTF8 NOT NULL DEFAULT 'STANDARD',
      FTS$FIELD_NAME VARCHAR(63) CHARACTER SET UTF8 DEFAULT NULL,
      FTS$FRAGMENT_SIZE SMALLINT NOT NULL DEFAULT 512,
      FTS$LEFT_TAG VARCHAR(50) CHARACTER SET UTF8 NOT NULL DEFAULT '<b>',
      FTS$RIGHT_TAG VARCHAR(50) CHARACTER SET UTF8 NOT NULL DEFAULT '</b>')
  RETURNS VARCHAR(8191) CHARACTER SET UTF8;
```

The `FTS$TEXT` parameter specifies the text in which fragments are searched and selected.

The `FTS$QUERY` parameter specifies the search phrase.

The third optional parameter `FTS$ANALYZER` specifies the name of the analyzer with which the terms are allocated.

The `FTS$FIELD_NAME` parameter specifies the name of the field being searched for. It must be specified if the search query explicitly contains several fields,
otherwise the parameter can be omitted or set as NULL.

The `FTS$FRAGMENT_SIZE` parameter specifies a limit on the length of the returned fragment.
Please note that the actual length of the returned text may be longer. The returned fragment usually does not break the words,
in addition, it does not take into account the length of the tags themselves for selection.

The `FTS$LEFT_TAG` parameter specifies the tag that is added to the found term on the left.

The `FTS$RIGHT_TAG` parameter specifies the tag that is added to the found fragment on the right.

The simplest example of use:

```sql
SELECT
  FTS$HIGHLIGHTER.FTS$BEST_FRAGMENT(
    q'!!Go to your orders and start the return Select the ship method Ship it! |
    Go to your orders and start the return Select the ship method Ship it! |
    show up to 2 reviews by default A shiny Pewter key ring with a 3D element
    of a rotating golf ball made of a PVC material. This makes a great accessory
    for your sports bag. | 1.12 ounces (View shipping rates and policies)!!',
    'A shiny Pewter',
    'English',
    NULL
  ) AS TEXT_FRAGMENT
FROM RDB$DATABASE
```

Now let's combine the search itself and the selection of the found terms:

```sql
EXECUTE BLOCK (
  FTS$QUERY VARCHAR(8191) CHARACTER SET UTF8 = :FTS_QUERY
)
RETURNS (
  FTS$SCORE DOUBLE PRECISION,
  PRODUCT_ID TYPE OF COLUMN PRODUCTS.PRODUCT_ID,
  PRODUCT_NAME TYPE OF COLUMN PRODUCTS.PRODUCT_NAME,
  ABOUT_PRODUCT TYPE OF COLUMN PRODUCTS.ABOUT_PRODUCT,
  HIGHTLIGHT_PRODUCT_NAME VARCHAR(8191) CHARACTER SET UTF8,
  HIGHTLIGHT_ABOUT_PRODUCT VARCHAR(8191) CHARACTER SET UTF8
)
AS
BEGIN
  FOR
    SELECT
      FTS.FTS$SCORE,
      PRODUCTS.PRODUCT_ID,
      PRODUCTS.PRODUCT_NAME,
      PRODUCTS.ABOUT_PRODUCT,
      FTS$HIGHLIGHTER.FTS$BEST_FRAGMENT(PRODUCTS.PRODUCT_NAME, :FTS$QUERY, 'ENGLISH', 'PRODUCT_NAME') AS HIGHTLIGHT_PRODUCT_NAME,
      FTS$HIGHLIGHTER.FTS$BEST_FRAGMENT(PRODUCTS.ABOUT_PRODUCT, :FTS$QUERY, 'ENGLISH', 'ABOUT_PRODUCT') AS HIGHTLIGHT_ABOUT_PRODUCT
    FROM FTS$SEARCH('IDX_PRODUCT_ID_2_EN', :FTS$QUERY, 25) FTS
    JOIN PRODUCTS ON PRODUCTS.PRODUCT_ID = FTS.FTS$ID
  INTO
    FTS$SCORE,
    PRODUCT_ID,
    PRODUCT_NAME,
    ABOUT_PRODUCT,
    HIGHTLIGHT_PRODUCT_NAME,
    HIGHTLIGHT_ABOUT_PRODUCT
  DO
    SUSPEND;
END
```

### Highlighting the found terms using the FTS$HIGHLIGHTER.FTS$BEST_FRAGMENTS procedure

The procedure `FTS$HIGHLIGHTER.FTS$BEST_FRAGMENTS` returns several fragments of text in which the found terms are marked with tags.

The procedure is described as

```sql
  PROCEDURE FTS$BEST_FRAGMENTS (
      FTS$TEXT BLOB SUB_TYPE TEXT CHARACTER SET UTF8,
      FTS$QUERY VARCHAR(8191) CHARACTER SET UTF8,
      FTS$ANALYZER VARCHAR(63) CHARACTER SET UTF8 NOT NULL DEFAULT 'STANDARD',
      FTS$FIELD_NAME VARCHAR(63) CHARACTER SET UTF8 DEFAULT NULL,
      FTS$FRAGMENT_SIZE SMALLINT NOT NULL DEFAULT 512,
      FTS$MAX_NUM_FRAGMENTS INTEGER NOT NULL DEFAULT 10,
      FTS$LEFT_TAG VARCHAR(50) CHARACTER SET UTF8 NOT NULL DEFAULT '<b>',
      FTS$RIGHT_TAG VARCHAR(50) CHARACTER SET UTF8 NOT NULL DEFAULT '</b>')
  RETURNS (
      FTS$FRAGMENT VARCHAR(8191) CHARACTER SET UTF8);
```

The input parameters of the procedure `FTS$HIGHLIGHTER.FTS$BEST_FRAGMENTS` are identical to the parameters of 
the function `FTS$HIGHLIGHTER.FTS$BEST_FRAGMENT`, but there is one additional parameter `FTS$MAX_NUM_FRAGMENTS`, 
which limits the number of fragments returned.

The text of the found fragments with selected occurrences of terms is returned in the output parameter `FTS$FRAGMENT`. 
This procedure should be applied in one document already found.

Usage example:

```sql
SELECT
    BOOKS.TITLE
  , BOOKS.CONTENT
  , F.FTS$FRAGMENT
FROM BOOKS
LEFT JOIN FTS$HIGHLIGHTER.FTS$BEST_FRAGMENTS(
  BOOKS.CONTENT,
  'friendly',
  'ENGLISH'
) F ON TRUE
WHERE BOOKS.ID = 8
```

## Keeping data up-to-date in full-text indexes

There are several ways to keep full-text indexes up-to- date:

1. Periodically call the procedure `FTS$MANAGEMENT.FTS$REBUILD_INDEX` for the specified index.
This method completely rebuilds the full-text index. In this case, all records of the table or view are read
for which the index was created.

2. You can maintain full-text indexes using triggers and calling one of the `FTS$LOG_BY_ID` procedures inside them,
`FTS$LOG_BY_UUID` or `FTS$LOG_BY_DBKEY`. Which of the procedures to call
depends on which type of field is selected as the key (integer, UUID (GIUD) or `RDB$DB_KEY`).
When calling these procedures, the change record is added to a special table `FTS$LOG` (change log).
Changes from the log are transferred to full-text indexes by calling the procedure `FTS$UPDATE_INDEXES'.
The call to this procedure must be done in a separate script, which can be placed in the task scheduler (Windows)
or cron (Linux) with some frequency, for example 5 minutes.

3. Delayed updating of full-text indexes, using FirebirdStreaming technology. In this case, a special
service reads the replication logs and extracts from them the information necessary to update the full-text indexes.
(under development).

### Triggers to keep full-text indexes up-to-date

To maintain the relevance of full-text indexes, it is necessary to create triggers that, when changing
any of the fields included in the full-text index, writes information about the record change to a special table
`FTS$LOG` (log).

Rules for writing triggers to support full-text indexes:

1. In the trigger, it is necessary to check all fields that participate in the full-text index.
The field validation conditions must be combined via `OR`.

2. For the `INSERT` operation, it is necessary to check all fields included in full-text indexes whose value is different
from `NULL`. If this condition is met, then one of the procedures must be performed
`FTS$LOG_BY_DBKEY('<table name>', NEW.RDB$DB_KEY, 'I');` or `FTS$LOG_BY_ID('<table name>', NEW.<key field>, 'I')`
or `FTS$LOG_BY_UUID('<table name>', NEW.<key field>, 'I')`.

3. For the `UPDATE` operation, it is necessary to check all fields included in full-text indexes whose value has changed.
If this condition is met, then the procedure `FTS$LOG_BY_DBKEY('<table name>', OLD.RDB$DB_KEY, 'U');`
or `FTS$LOG_BY_ID('<table name>', OLD.<key field>, 'U')`or `FTS$LOG_BY_UUID('<table name>', OLD.<key field>, 'U')`.

4. For the `DELETE` operation, it is necessary to check all fields included in full-text indexes whose value is different
from `NULL`. If this condition is met, then it is necessary to perform the procedure
`FTS$LOG_CHANGE('<table name>', OLD.RDB$DB_KEY, 'D');`.

To facilitate the task of writing such triggers, there is a special package `FTS$TRIGGER_HELPER`, which
contains procedures for generating trigger source texts. So for example, in order to generate triggers
to support full-text indexes created for the `PRODUCTS` table, you need to run the following query:

```sql
SELECT
    FTS$TRIGGER_SCRIPT
FROM FTS$TRIGGER_HELPER.FTS$MAKE_TRIGGERS('PRODUCTS', TRUE)
```

This query will return the following trigger text for all created FTS indexes on the `PRODUCTS` table:

```sql
CREATE OR ALTER TRIGGER "FTS$PRODUCTS_AIUD" FOR "PRODUCTS"
ACTIVE AFTER INSERT OR UPDATE OR DELETE
POSITION 100
AS
BEGIN
  /* Block for key PRODUCT_ID */
  IF (INSERTING AND (NEW."ABOUT_PRODUCT" IS NOT NULL
      OR NEW."PRODUCT_NAME" IS NOT NULL)) THEN
    EXECUTE PROCEDURE FTS$LOG_BY_ID('PRODUCTS', NEW."PRODUCT_ID", 'I');
  IF (UPDATING AND (NEW."ABOUT_PRODUCT" IS DISTINCT FROM OLD."ABOUT_PRODUCT"
      OR NEW."PRODUCT_NAME" IS DISTINCT FROM OLD."PRODUCT_NAME")) THEN
    EXECUTE PROCEDURE FTS$LOG_BY_ID('PRODUCTS', OLD."PRODUCT_ID", 'U');
  IF (DELETING AND (OLD."ABOUT_PRODUCT" IS NOT NULL
      OR OLD."PRODUCT_NAME" IS NOT NULL)) THEN
    EXECUTE PROCEDURE FTS$LOG_BY_ID('PRODUCTS', OLD."PRODUCT_ID", 'D');
  /* Block for key PRODUCT_UUID */
  IF (INSERTING AND (NEW."PRODUCT_NAME" IS NOT NULL)) THEN
    EXECUTE PROCEDURE FTS$LOG_BY_UUID('PRODUCTS', NEW."PRODUCT_UUID", 'I');
  IF (UPDATING AND (NEW."PRODUCT_NAME" IS DISTINCT FROM OLD."PRODUCT_NAME")) THEN
    EXECUTE PROCEDURE FTS$LOG_BY_UUID('PRODUCTS', OLD."PRODUCT_UUID", 'U');
  IF (DELETING AND (OLD."PRODUCT_NAME" IS NOT NULL)) THEN
    EXECUTE PROCEDURE FTS$LOG_BY_UUID('PRODUCTS', OLD."PRODUCT_UUID", 'D');
  /* Block for key RDB$DB_KEY */
  IF (INSERTING AND (NEW."PRODUCT_NAME" IS NOT NULL)) THEN
    EXECUTE PROCEDURE FTS$LOG_BY_DBKEY('PRODUCTS', NEW.RDB$DB_KEY, 'I');
  IF (UPDATING AND (NEW."PRODUCT_NAME" IS DISTINCT FROM OLD."PRODUCT_NAME")) THEN
    EXECUTE PROCEDURE FTS$LOG_BY_DBKEY('PRODUCTS', OLD.RDB$DB_KEY, 'U');
  IF (DELETING AND (OLD."PRODUCT_NAME" IS NOT NULL)) THEN
    EXECUTE PROCEDURE FTS$LOG_BY_DBKEY('PRODUCTS', OLD.RDB$DB_KEY, 'D');
END
```

Updating all full-text indexes, you need to create an SQL script `fts$update.sql`

```sql
EXECUTE PROCEDURE FTS$UPDATE_INDEXES;
```

Then a script to call the SQL script via ISQL, something like the following

```bash
isql -user SYSDBA -pas masterkey -i fts$update.sql inet://localhost/mydatabase
```

Pay attention! The package `FTS$TRIGGER_HELPER` helps to generate triggers to support full-text indexes
only for regular tables. If you want to maintain a full-text index on the view, then you need
to develop such triggers for the base tables of the view yourself.
Below is an example that supports a full-text index of triggers for a view `V_PRODUCT_CATEGORIES`.

```sql
SET TERM ^;

-- Field PRODUCT_UUID and CATEGORY_ID from table PRODUCT_CATEGORIES
CREATE OR ALTER TRIGGER FTS$PRODUCT_CATEGORIES_AIUD FOR PRODUCT_CATEGORIES
ACTIVE AFTER INSERT OR UPDATE OR DELETE
POSITION 100
AS
BEGIN
  IF (INSERTING) THEN
    EXECUTE PROCEDURE FTS$LOG_BY_UUID('V_PRODUCT_CATEGORIES', NEW.PRODUCT_UUID, 'I');

  IF (UPDATING AND (NEW.PRODUCT_UUID <> OLD.PRODUCT_UUID
      OR NEW.CATEGORY_ID <> OLD.CATEGORY_ID)) THEN
  BEGIN
    EXECUTE PROCEDURE FTS$LOG_BY_UUID('V_PRODUCT_CATEGORIES', OLD.PRODUCT_UUID, 'D');
    EXECUTE PROCEDURE FTS$LOG_BY_UUID('V_PRODUCT_CATEGORIES', NEW.PRODUCT_UUID, 'I');
  END

  IF (DELETING) THEN
    EXECUTE PROCEDURE FTS$LOG_BY_UUID('V_PRODUCT_CATEGORIES', OLD.PRODUCT_UUID, 'D');
END
^

-- Change from table CATEGORIES
CREATE OR ALTER TRIGGER FTS$CATEGORIES_AU FOR CATEGORIES
ACTIVE AFTER UPDATE
POSITION 100
AS
DECLARE PRODUCT_UUID TYPE OF COLUMN PRODUCT_CATEGORIES.PRODUCT_UUID;
BEGIN
  IF (NEW.CATEGORY_NAME <> OLD.CATEGORY_NAME) THEN
  BEGIN
    SELECT MAX(PRODUCT_CATEGORIES.PRODUCT_UUID)
    FROM PRODUCT_CATEGORIES
    JOIN CATEGORIES ON CATEGORIES.ID = PRODUCT_CATEGORIES.CATEGORY_ID
    WHERE CATEGORIES.CATEGORY_NAME = OLD.CATEGORY_NAME
    INTO PRODUCT_UUID;

    EXECUTE PROCEDURE FTS$LOG_BY_UUID('V_PRODUCT_CATEGORIES', :PRODUCT_UUID, 'U');
  END
END
END
^

SET TERM ;^
```
